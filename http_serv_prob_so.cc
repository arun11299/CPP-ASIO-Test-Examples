#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>

using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;

int main(int argc, const char * argv[]) {

    static asio::io_service ioService;
    static asio::io_service ioService1;
    static tcp::acceptor tcpAcceptor(ioService, tcp::endpoint(tcp::v4(), 2080));
    //tcpAcceptor.listen(20);

    while (true) {

        // creates a socket
        tcp::socket* socket = new tcp::socket(ioService1);

        // wait and listen
        tcpAcceptor.accept(*socket);

        asio::streambuf inBuffer;
        istream headerLineStream(&inBuffer);

        char buffer[1];
	std::cout << "Start Read" << std::endl;
        auto read_bytes = asio::read(*socket, asio::buffer(buffer, 1));  // <--- Yuck!
	std::cout << "Read bytes = " << read_bytes << std::endl;

        asio::write(*socket, asio::buffer((string) "HTTP/1.1 200 OK\r\n\r\nYup!"));

        socket->shutdown(asio::ip::tcp::socket::shutdown_both);
        socket->close();
        delete socket;
	std::cout << "End Connection" << std::endl;

    }

    return 0;
}

