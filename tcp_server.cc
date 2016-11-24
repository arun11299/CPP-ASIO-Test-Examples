#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

std::string _coordinates = "Hello\r\n";

using namespace std;
using boost::asio::ip::tcp;

std::string inline make_daytime_string() {
    std:: time_t now = std::time(0);
    return std::ctime(&now);
}

class tcp_connection
    // Using shared_ptr and enable_shared_from_this
    // because we want to keep the tcp_connection object alive
    // as long as there is an operation that refers to it.
    : public boost::enable_shared_from_this<tcp_connection>
    {
    public:
        typedef boost::shared_ptr<tcp_connection> pointer;

        static pointer create(boost::asio::io_service& io_service) {
            cout << "Creates a pointer for the tcp connection" <<endl;
            return pointer(new tcp_connection(io_service));
        }

        tcp::socket& socket() {
            return socket_;
        }

        // Call boost::asio::async_write() to serve the data to the client.
        // We are using boost::asio::async_write(),
        // rather than ip::tcp::socket::async_write_some(),
        // to ensure that the entire block of data is sent.

        void start() {
          start_read();

          // This is going to read after every 1ms the _coordinates variable
          usleep(1000);
          m_message = _coordinates;
        }

private:
    tcp_connection(boost::asio::io_service& io_service)
        : socket_(io_service)
        {
        }

    void start_read() {
        // Start an asynchronous operation to read a newline-delimited message.
        // When read, handle_read should kick in
        boost::asio::async_read_until(
            socket_,
            input_buffer_,
            '\n',
            boost::bind(
                &tcp_connection::handle_read,
                shared_from_this(),
                boost::asio::placeholders::error
            )
        );
    }

    // When stream is received, handle the message from the client
    void handle_read(const boost::system::error_code& ec) {

        std::cout << "HANDLE_READ - line 101" << "\n";
        messageFromClient_ = "";
        if (!ec) {
            // Extract the newline-delimited message from the buffer.
            std::string line;
            std::istream is(&input_buffer_);
            std::getline(is, line);

            // Empty messages are heartbeats and so ignored.
            if (!line.empty()) {
                messageFromClient_ += line;
                std::cout << "Received: " << line << "\n";
            }
            start_read();
        }
        else {
            std::cout << "Error on receive: " << ec.message() << "\n";
        }

        start_read();

	boost::asio::async_write(
                    socket_,
                    boost::asio::buffer(m_message),
                    boost::bind(
                        &tcp_connection::handle_write,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred
                    )
                );
    }

    // handle_write() is responsible for any further actions
    // for this client connection.
    void handle_write(const boost::system::error_code& /*error*/, size_t /*bytes_transferred*/) {
        m_message += "helloo\n";
    }

    tcp::socket socket_;
    std::string m_message;
    boost::asio::streambuf input_buffer_;
    std::string messageFromClient_;
};

class tcp_server {
    public:
        // Constructor: initialises an acceptor to listen on TCP port 14002.
        tcp_server(boost::asio::io_service& io_service)
        : acceptor_(io_service, tcp::endpoint(tcp::v4(), 14002))
        {
            // start_accept() creates a socket and
            // initiates an asynchronous accept operation
            // to wait for a new connection.
            start_accept();
        }

private:
    void start_accept() {
        // creates a socket
        cout << "creating a new socket for the communication" <<endl;
        tcp_connection::pointer new_connection = tcp_connection::create(acceptor_.get_io_service());

        // initiates an asynchronous accept operation
        // to wait for a new connection.
        acceptor_.async_accept(
            new_connection->socket(),
            boost::bind(
                &tcp_server::handle_accept,
                this,
                new_connection,
                boost::asio::placeholders::error
            )
        );
    }

    // handle_accept() is called when the asynchronous accept operation
    // initiated by start_accept() finishes. It services the client request
    void handle_accept(tcp_connection::pointer new_connection, const boost::system::error_code& error) {

        if (!error) {
            cout << "Starting the new tcp connection" <<endl;
            new_connection->start();
        }

        // Call start_accept() to initiate the next accept operation.
        start_accept();
    }

    tcp::acceptor acceptor_;
};


int main() {
    try {
        boost::asio::io_service io_service;
        tcp_server server(io_service);

        // Run the io_service object to perform asynchronous operations.
        io_service.run();
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
