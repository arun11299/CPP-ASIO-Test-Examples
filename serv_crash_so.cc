#include <iostream>
#include <string>
#include <memory>
#include <array>
#include <boost/asio.hpp>

using namespace boost::asio;

//Connection Class

class Connection : public std::enable_shared_from_this<Connection>{

    ip::tcp::socket m_socket;
    std::array<char, 2056> m_acceptMessage;
    std::string m_acceptMessageWrapper;
    std::string m_buffer;
public:
    Connection(io_service& ioService): m_socket(ioService) {    }

    virtual ~Connection() { }

    static std::shared_ptr<Connection> create(io_service& ioService){
        return std::shared_ptr<Connection>(new Connection(ioService));
    }


    std::string& receiveMessage() {
         size_t len = boost::asio::read(m_socket, boost::asio::buffer(m_acceptMessage));
         m_acceptMessageWrapper = std::string(m_acceptMessage.begin(), m_acceptMessage.begin() + len);
         return m_acceptMessageWrapper;
    }

    void sendMessage(const std::string& message) {
         boost::asio::write(m_socket, boost::asio::buffer(message));
    }

    ip::tcp::socket& getSocket(){
        return m_socket;
    }

};



//Server Class

class Server {
  io_service m_ioService ;
  ip::tcp::acceptor m_acceptor;


public:
    Server(int port):
        m_acceptor(m_ioService, ip::tcp::endpoint(ip::tcp::v4(), port)){    }

    virtual ~Server() { }

    std::shared_ptr<Connection> createConnection(){
        std::shared_ptr<Connection> newConnection = Connection::create(m_ioService);
        m_acceptor.accept(newConnection->getSocket());
        return newConnection;
    }

    void runService() {
        m_ioService.run();
    }


};


int main(int argc, char* argv[]) {
    Server s(5000);
    auto c1 = s.createConnection();
    //do soething
    s.runService();
    return 0;
}
