#include <iostream>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
 
using boost::asio::ip::tcp;
 
std::string make_daytime_string()
{
    using namespace std; // For time_t, time and ctime;
    time_t now = time(0);
    return ctime(&now);
}

class Connection : public boost::enable_shared_from_this<Connection>
{
public:
    typedef boost::shared_ptr<Connection> pointer;

    static pointer create(boost::asio::io_service& io_service)
    {
        return pointer(new Connection(io_service));
    }


    tcp::socket& socket()
    {
        return _socket;
    }

    void connect(tcp::resolver::iterator& point)
    {
        boost::asio::async_connect(_socket, point, boost::bind(&Connection::connect_handler, shared_from_this(), boost::asio::placeholders::error));
    }

    void connect_handler(const boost::system::error_code& error)
    {
        if(error)
        {
            std::cout << "Error on connect_handler: " << error.message() << std::endl;
            return;
        }

        std::cout << "You are connected to the server." << std::endl;
        start();

    }

    void start()
    {
        start_write();
        start_read();
    }

private:
    // private ctor
    Connection(boost::asio::io_service& io) : _socket(io){}

    void start_write()
    {
        _daymessage = make_daytime_string();

        boost::asio::async_write(_socket, boost::asio::buffer(_daymessage),
                                 boost::bind(&Connection::handle_write, shared_from_this(),
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred));
    }

    void handle_write(const boost::system::error_code& error,
                      size_t bytes)
    {
        if(error)
        {
            std::cout << "Error on handle write: " << error.message() << std::endl;
            return;
        }

        std::cout << "Message has been sent!" << std::endl;
        start_write();
    }

    void start_read()
    {
        // Start an asynchronous operation to read a newline-delimited message.
        boost::asio::async_read_until(_socket, _buffer, '\n',
                                      boost::bind(&Connection::handle_read, shared_from_this(),
                                                  boost::asio::placeholders::error,
                                                  boost::asio::placeholders::bytes_transferred));
    }

    void handle_read(const boost::system::error_code& error, size_t bytes)
    {
        if(error)
        {
            std::cout << "Error on handle read: " << error.message() << std::endl;
            return;
        }

        // Extract the newline-delimited message from the buffer.
        std::string line;
        std::istream is(&_buffer);
        std::getline(is, line);

        if (!line.empty())
        {
            std::cout << "Received: " << line << "\n";
        }

        start_read();
    }

    tcp::socket _socket;
    std::string _daymessage;
    boost::asio::streambuf _buffer;
};

int main()
{
    std::cout << "Client is running!" << std::endl;

    try
    {
        boost::asio::io_service io_service;

        tcp::resolver resolver(io_service);

        tcp::resolver::query query("127.0.0.1", "7171");

        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        auto connection = Connection::create(io_service);

        connection->connect(endpoint_iterator);

        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
