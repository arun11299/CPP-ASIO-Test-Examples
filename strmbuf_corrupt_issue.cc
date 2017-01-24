 typedef std::vector  NetworkBuffer;

    class CSession : public boost::enable_shared_from_this
    {
    private:
        boost::asio::io_service&        m_ioService;
        CSessionSocket                  m_socket;   // derived from boost::asio::ip::tcp::socket; just want to capture destruction
        INetworkHandler*                m_pNetworkHandler;
        NetworkBufferQueue              m_writeQueue;

        long                            m_readPayloadLength;

        boost::asio::streambuf          m_buffer;

        boost::interprocess::interprocess_mutex     m_readMutex;

    public:
        CSession(boost::asio::io_service& io, INetworkHandler* pNetworkHandler);
        ~CSession();

        boost::asio::ip::tcp::socket& socket() { return m_socket; }
        INetworkHandler* getNetworkHandler() { return m_pNetworkHandler; }

        void handleConnect(const boost::system::error_code& error);

        void read_header();
        void post_write(CProtocolMessage_ptr& messagePtr);

        void close();

    private:
        void handle_read_header(const boost::system::error_code& error, size_t bytesRead);
        void handle_read_body(const boost::system::error_code& error, size_t bytesRead);

        bool decode_header();
        bool decode_body();

        void write(CProtocolMessage_ptr& messagePtr);
        void handle_write(const boost::system::error_code& error);

        void do_close();
    };

    typedef boost::shared_ptr CSession_ptr;


    CSession::CSession(boost::asio::io_service& io, INetworkHandler* pNetworkHandler)
        : m_ioService(io),
          m_socket(io),
          m_pNetworkHandler(pNetworkHandler)
    {
    }

    void CSession::read_header() 
    {
        boost::asio::async_read(m_socket,
            m_buffer,
            boost::asio::transfer_exactly(sizeof(m_readPayloadLength)),
            boost::bind(&CSession::handle_read_header, shared_from_this(),
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void CSession::handle_read_header(const boost::system::error_code& error, size_t bytesRead)
    {
        scoped_lock lock(m_readMutex);

        if (!error && decode_header()) {        
            boost::asio::async_read(m_socket,
                m_buffer,
                boost::asio::transfer_exactly(m_readPayloadLength),
                boost::bind(&CSession::handle_read_body, shared_from_this(),
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        } else {
            do_close();
        }
    }

    bool CSession::decode_header()
    {
        std::streamsize sa = m_buffer.in_avail();

        NetworkBuffer buffer;

        boost::asio::streambuf::const_buffers_type bufs = m_buffer.data();
        buffer.assign(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + sizeof(m_readPayloadLength));
        m_readPayloadLength = *((long*)buffer.data());
        m_buffer.consume(sizeof(m_readPayloadLength));

        sa = m_buffer.in_avail();

        int tmp = ntohl(m_readPayloadLength);
        if (tmp > 0 && tmp  lock(m_readMutex);

        if (!error && decode_body()) {
            read_header();  // read the next packet
        } else {
            do_close();
        }
    }

    bool CSession::decode_body()
    {
        std::streamsize sa = m_buffer.in_avail();

        NetworkBuffer buffer;

        boost::asio::streambuf::const_buffers_type bufs = m_buffer.data();
        buffer.assign(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + m_readPayloadLength);
        m_buffer.consume(m_readPayloadLength);

        sa = m_buffer.in_avail();

        CProtocolMessage_ptr* pMessage = CProtocolMessage::deserialize(buffer, this);
        if (pMessage) {
            return m_pNetworkHandler->onCommand(pMessage);
        }
        return false;
    }

    void CSession::post_write(CProtocolMessage_ptr& msgPtr)
    {
        m_ioService.post(boost::bind(&CSession::write, this->shared_from_this(), msgPtr));
    }

    void CSession::write(CProtocolMessage_ptr& msgPtr)
    {
        auto self(shared_from_this());

        // write to the buffer which will be sent
        msgPtr->serialize();

        bool write_in_progress = !m_writeQueue.empty();
        m_writeQueue.push_back(msgPtr->shared_from_this());
        if (!write_in_progress) {
            boost::asio::async_write(m_socket,
                boost::asio::buffer(m_writeQueue.front()->getBuffer().data(),
                    m_writeQueue.front()->getBuffer().size()),
                boost::bind(&CSession::handle_write, shared_from_this(),
                    boost::asio::placeholders::error));
        }
    }

    void CSession::handle_write(const boost::system::error_code& error)
    {
        auto self(shared_from_this());

        CProtocolMessage_ptr& msgPtr = m_writeQueue.front();
        msgPtr.reset();

        if (!error) {
            m_writeQueue.pop_front();
            if (!m_writeQueue.empty()) {
                boost::asio::async_write(m_socket,
                    boost::asio::buffer(m_writeQueue.front()->getBuffer().data(),
                        m_writeQueue.front()->getBuffer().size()),
                    boost::bind(&CSession::handle_write, shared_from_this(),
                        boost::asio::placeholders::error));
            }
        } else {
            do_close();
        }
    }

    void CSession::close()
    {
        m_ioService.post([this]() { m_socket.close(); });
    }

    void CSession::do_close()
    {
        //m_socket.close();
        this->m_pNetworkHandler->onDisconnect(this);
    }

