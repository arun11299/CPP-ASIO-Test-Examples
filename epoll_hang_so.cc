//http://stackoverflow.com/questions/41804866/asio-on-linux-stalls-in-epoll

#define ASIO_STANDALONE
#define ASIO_HEADER_ONLY
#define ASIO_NO_EXCEPTIONS
#define ASIO_NO_TYPEID
#include "asio.hpp"

#include <chrono>
#include <iostream>
#include <list>
#include <map>
#include <thread>
#include <string>
#include <mutex>

static bool s_freeInboundSocket = false;
static bool s_freeOutboundSocket = false;

static std::mutex lck;

class Tester
{
public:
    Tester(asio::io_service& i_ioService, unsigned i_n, unsigned port_base)
        : m_inboundStrand(i_ioService)
        , m_outboundStrand(i_ioService)
        , m_resolver(i_ioService)
        , m_n(i_n)
        , m_traceStart(std::chrono::high_resolution_clock::now())
        , m_listener(i_ioService, asio::ip::tcp::endpoint(asio::ip::address_v4::from_string("127.0.0.1"), port_base+m_n))
    {}

    ~Tester()
    {}

    void TraceIn(unsigned i_line)
    {
        m_inboundTrace.emplace_back(i_line, std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_traceStart));
    }

    void AbortIn(std::string err_msg)
    {
        //TraceIn(i_line);
      lck.lock();
      std::cout << err_msg << std::endl;
      lck.unlock();
        abort();
    }

    void AbortIn(const asio::error_code ec)
    {
      lck.lock();
      std::cout << ec.message() << std::endl;
      lck.unlock();
      abort();
    }

    void TraceOut(unsigned i_line)
    {
        m_outboundTrace.emplace_back(i_line, std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_traceStart));
    }

    template <typename T>
    void AbortOut(T arg)
    {
        //TraceOut(i_line);
        AbortIn(arg);
    }

    void DumpTrace(std::map<unsigned, unsigned>& o_counts)
    {
        std::cout << "## " << m_n << " ##\n";
        std::cout << "-- " << m_traceStart.time_since_epoch().count() << "\n";
        std::cout << "- in -             - out -\n";

        auto in = m_inboundTrace.begin();
        auto out = m_outboundTrace.begin();

        while ((in != m_inboundTrace.end()) || (out != m_outboundTrace.end()))
        {
            if (in == m_inboundTrace.end()) 
            {
                ++o_counts[out->first];

                std::cout << "                  " << out->first << " : " << out->second.count() << "\n";
                ++out;
            }
            else if (out == m_outboundTrace.end())  
            {
                ++o_counts[in->first];

                std::cout << in->first << " : " << in->second.count() << "\n";
                ++in;
            }
            else if (out->second < in->second)
            {
                ++o_counts[out->first];

                std::cout << "                  " << out->first << " : " << out->second.count() << "\n";
                ++out;
            }
            else
            {
                ++o_counts[in->first];

                std::cout << in->first << " : " << in->second.count() << "\n";
                ++in;
            }
        }
        std::cout << std::endl;
    }

    //////////////
    // Inbound

    void Listen(uint16_t i_portBase)
    {
        m_inboundSocket.reset(new asio::ip::tcp::socket(m_inboundStrand.get_io_service()));

        asio::error_code ec;
        TraceIn(__LINE__);

        m_listener.async_accept(*m_inboundSocket,
            m_inboundStrand.wrap([this](const asio::error_code& i_error)
        {
            OnInboundAccepted(i_error);
        }));
    }

    void OnInboundAccepted(const asio::error_code& i_error)
    {
        TraceIn(__LINE__);

        if (i_error) { AbortIn(i_error); return; }

        asio::async_read_until(*m_inboundSocket, m_inboundRxBuf, '\n',
            m_inboundStrand.wrap([this](const asio::error_code& i_err, size_t i_nRd)
        {
            OnInboundReadCompleted(i_err, i_nRd);
        }));
    }


    void OnInboundReadCompleted(const asio::error_code& i_error, size_t i_nRead)
    {
        TraceIn(__LINE__);

        if (i_error) { AbortIn(i_error); return; }
        if (i_nRead != 4) { AbortIn("More data read"); return; }  // "msg\n"

        std::istream is(&m_inboundRxBuf);
        std::string s;
        if (!std::getline(is, s)) { AbortIn("getline failed"); return; }
        if (s != "msg") { AbortIn("incorrect data read"); return; }
        if (m_inboundRxBuf.in_avail() != 0) { AbortIn("protocol error"); return; }

        asio::async_read_until(*m_inboundSocket, m_inboundRxBuf, '\n',
            m_inboundStrand.wrap([this](const asio::error_code& i_err, size_t i_nRd)
        {
            OnInboundWaitCompleted(i_err, i_nRd);
        }));

    }

    void OnInboundWaitCompleted(const asio::error_code& i_error, size_t i_nRead)
    {
        TraceIn(__LINE__);

        if (i_error != asio::error::eof) { AbortIn("expected eof"); return; }
        if (i_nRead != 0) { AbortIn("more data read"); return; }

        if (s_freeInboundSocket)
        {
            m_inboundSocket.reset();
        }
    }

    //////////////
    // Outbound

    void Connect(std::string i_host, uint16_t i_portBase)
    {
        asio::error_code ec;
        auto endpoint = m_resolver.resolve(asio::ip::tcp::resolver::query(i_host, std::to_string(i_portBase+m_n)), ec);
        if (ec) { AbortOut(ec); return; }

        m_outboundSocket.reset(new asio::ip::tcp::socket(m_outboundStrand.get_io_service()));

        TraceOut(__LINE__);

        asio::async_connect(*m_outboundSocket, endpoint, [this](auto ec, auto s) {
              this->OnOutboundConnected(ec, s);
            });

    }

    template <typename T>
    void OnOutboundConnected(const asio::error_code& i_error, T s)
    {
        TraceOut(__LINE__);

        if (i_error) { AbortOut(i_error); return; }

        std::ostream(&m_outboundTxBuf) << "msg" << '\n';

        asio::async_write(*m_outboundSocket, m_outboundTxBuf.data(),
            m_outboundStrand.wrap([this](const asio::error_code& i_error, size_t i_nWritten)
        {
            OnOutboundWriteCompleted(i_error, i_nWritten);
        }));
    }

    void OnOutboundWriteCompleted(const asio::error_code& i_error, size_t i_nWritten)
    {
        TraceOut(__LINE__);

        if (i_error) { AbortOut(i_error); return; }
        if (i_nWritten != 4) { AbortOut("out protocol error"); return; } // "msg\n"

        TraceOut(__LINE__);
        m_outboundSocket->shutdown(asio::socket_base::shutdown_both);

        asio::async_read_until(*m_outboundSocket, m_outboundRxBuf, '\n',
            m_outboundStrand.wrap([this](const asio::error_code& i_error, size_t i_nRead)
        {
            OnOutboundWaitCompleted(i_error, i_nRead);
        }));
    }

    void OnOutboundWaitCompleted(const asio::error_code& i_error, size_t i_nRead)
    {
        TraceOut(__LINE__);

        if (i_error != asio::error::eof) { AbortOut("out not eof"); return; }
        if (i_nRead != 0) { AbortOut("out protocol error"); return; }

        if (s_freeOutboundSocket)
        {
            m_outboundSocket.reset();
        }
    }

private:
    //////////////
    // Inbound

    asio::io_service::strand m_inboundStrand;

    std::unique_ptr<asio::ip::tcp::socket> m_inboundSocket;

    asio::streambuf m_inboundRxBuf;
    asio::streambuf m_inboundTxBuf;

    //////////////
    // Outbound

    asio::io_service::strand m_outboundStrand;

    asio::ip::tcp::resolver m_resolver;
    std::unique_ptr<asio::ip::tcp::socket> m_outboundSocket;

    asio::streambuf m_outboundRxBuf;
    asio::streambuf m_outboundTxBuf;

    //////////////
    // Common

    unsigned m_n;

    const std::chrono::high_resolution_clock::time_point m_traceStart;
    std::vector<std::pair<unsigned, std::chrono::nanoseconds>> m_inboundTrace;
    std::vector<std::pair<unsigned, std::chrono::nanoseconds>> m_outboundTrace;


    asio::ip::tcp::acceptor m_listener;
};

static int Usage(int i_ret)
{
    std::cout << "[" << i_ret << "]" << "Usage: example <nThreads> <nConnections> <inboundFree> <outboundFree>" << std::endl;
    return i_ret;
}

int main(int argc, char* argv[])
{
    if (argc < 5)
        return Usage(__LINE__);

    const unsigned nThreads = unsigned(std::stoul(argv[1]));
    if (nThreads == 0)
        return Usage(__LINE__);
    const unsigned nConnections = unsigned(std::stoul(argv[2]));
    if (nConnections == 0)
        return Usage(__LINE__);

    s_freeInboundSocket = (*argv[3] == 'y');
    s_freeOutboundSocket = (*argv[4] == 'y');

    const uint16_t listenPortBase = 15000;
    const uint16_t connectPortBase = 15000;
    const std::string connectHost = "127.0.0.1";

    for (int i = 0; i < 100; i++)
    {
    asio::io_service ioService;

    std::cout << "Creating." << std::endl;

    std::list<Tester> testers;

    for (unsigned i = 0; i < nConnections; ++i)
    {
        testers.emplace_back(ioService, i, listenPortBase);
        testers.back().Listen(listenPortBase);
        testers.back().Connect(connectHost, connectPortBase);
    }

    std::cout << "Starting." << std::endl;

    std::vector<std::thread> threads;

    for (unsigned i = 0; i < nThreads; ++i)
    {   
        threads.emplace_back([&]()
        {
            ioService.run();
        });
    }

    std::cout << "Waiting." << std::endl;

    for (auto& thread : threads)
    {
        thread.join();
    }

    std::cout << "Stopped." << std::endl;
    }

    return 0;
}

void DumpAllTraces(std::list<Tester>& i_testers)
{
    std::map<unsigned, unsigned> counts;

    for (auto& tester : i_testers)
    {
        tester.DumpTrace(counts);
    }   

    std::cout << "##############################\n";
    for (const auto& count : counts)
    {
        std::cout << count.first << " : " << count.second << "\n";
    }
    std::cout << std::endl;
}

#if defined(ASIO_NO_EXCEPTIONS)
namespace asio
{
        namespace detail
        {

                template <typename Exception>
                void throw_exception(const Exception& e)
                {
                  abort();
                }

        } // namespace detail
} // namespace asio
#endif
