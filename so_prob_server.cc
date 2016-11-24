#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
 
using boost::asio::ip::tcp;
 
std::string make_daytime_string()
{
    using namespace std; // For time_t, time and ctime;
    time_t now = time(0);
    return ctime(&now);
}
 
 
class tcp_connection: public boost::enable_shared_from_this<tcp_connection>
{
public:
    typedef boost::shared_ptr<tcp_connection> pointer;
   
    static pointer create(boost::asio::io_service& io_service)
    {
        return pointer(new tcp_connection(io_service));
    }
   
    tcp::socket& socket()
    {
        return socket_;
    }
   
    // Call boost::asio::async_write() to serve the data to the client.
    // We are using boost::asio::async_write(),
    // rather than ip::tcp::socket::async_write_some(),
    // to ensure that the entire block of data is sent.
   
    void start()
    {
        // The data to be sent is stored in the class member m_message
        // as we need to keep the data valid
        // until the asynchronous operation is complete.
       
        m_message = make_daytime_string();
       
        // When initiating the asynchronous operation,
        // and if using boost::bind(),
        // we must specify only the arguments
        // that match the handler's parameter list.
        // In this code, both of the argument placeholders
        // (boost::asio::placeholders::error
        // and boost::asio::placeholders::bytes_transferred)
        // could potentially have been removed,
        // since they are not being used in handle_write().
       
        std::cout << socket_.remote_endpoint().address().to_string() << ":" << socket_.remote_endpoint().port() << " connected!" << std::endl;
       
        boost::asio::async_write(socket_, boost::asio::buffer(m_message),
                                 boost::bind(&tcp_connection::handle_write, shared_from_this()));
	boost::asio::async_read_until(socket_, 
	    _buffer, 
	    '\n',
	    boost::bind(&tcp_connection::handle_receive, 
	      shared_from_this(), 
	      boost::asio::placeholders::error));
    }
 
   
private:
    tcp_connection(boost::asio::io_service& io_service)
    : socket_(io_service)
    {
    }
    // handle_write() is responsible for any further actions
    // for this client connection.
   
    void handle_write() // call back.. when it finishes sending, we come here
    {
        std::cout << "Client has received the messaged. " << std::endl;
    }
   
    void handle_receive(const boost::system::error_code& ErrorCode)
    {
        std::cout << "You received the following message from the server: "<< std::endl;
        if (ErrorCode) {
          std::cout << "Error occured: " << ErrorCode.message() << std::endl;
          return;
	}
	std::string line;
	std::istream is(&_buffer);
	std::getline(is, line);
	std::cout << line << std::endl;
    }
   
    tcp::socket socket_;
    std::string m_message;
    boost::asio::streambuf _buffer;
};
 
class tcp_server
{
public:
    tcp_server(boost::asio::io_service& io_service) : acceptor_(io_service, tcp::endpoint(tcp::v4(), 7171))
    {
        // start_accept() creates a socket and
        // initiates an asynchronous accept operation
        // to wait for a new connection.
        start_accept();
    }
   
private:
    void start_accept()
    {
        // creates a socket
        tcp_connection::pointer new_connection = tcp_connection::create(acceptor_.get_io_service());
       
        // initiates an asynchronous accept operation
        // to wait for a new connection.
        acceptor_.async_accept(new_connection->socket(),
                               boost::bind(&tcp_server::handle_accept, this, new_connection,
                                           boost::asio::placeholders::error));
    }
   
    // handle_accept() is called when the asynchronous accept operation
    // initiated by start_accept() finishes. It services the client request
    void handle_accept(tcp_connection::pointer new_connection,
                       const boost::system::error_code& error)
    {
        if (!error)
        {
            new_connection->start();
        }
       
        // Call start_accept() to initiate the next accept operation.
        start_accept();
    }
   
    tcp::acceptor acceptor_;
};
 
int main()
{
    std::cout << "Server is online!" << std::endl;
    try
    {
        boost::asio::io_service io_service;
        tcp_server server(io_service);
        io_service.run();
       
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
   
    return 0;
}
