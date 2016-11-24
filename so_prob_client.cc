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
 
class Connection: public boost::enable_shared_from_this<Connection>
{
public:
    Connection(boost::asio::io_service& io) : _socket(io){}
   
    void connect(tcp::resolver::iterator& point)
    {
        boost::asio::async_connect(_socket, point, boost::bind(&Connection::onConnected, this, boost::asio::placeholders::error));
    }
   
    void onConnected(const boost::system::error_code& ErrorCode)
    {
        std::cout << "You are connected" << std::endl;
       
        // receive first message on onReceive
        boost::asio::async_read_until(_socket, 
            _buffer, 
            '\n',
            boost::bind(&Connection::onReceive, 
              this, boost::asio::placeholders::error));
    }
   
    void onSend(const boost::system::error_code& ErrorCode, std::size_t bytes_transferred)
    {
        std::cout << "Sending..." << std::endl;
    }
   
    void onReceive(const boost::system::error_code& ErrorCode)
    {
        std::cout << "You received the following message from the server:" << std::endl;
        //std::cout.write(_buffer.data(), bytes_transferred);
       
        // send first message on onSend
        m_message = make_daytime_string() + '\n';
	std::cout << "Sending " << m_message << std::endl;
        boost::asio::async_write(_socket, boost::asio::buffer(m_message), boost::bind(&Connection::onSend, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
   
    tcp::socket& getSocket()
    {
        return _socket;
    }
private:
    tcp::socket _socket;
    boost::asio::streambuf _buffer;
    std::string m_message;
};
 
int main()
{
    try
    {
        boost::asio::io_service io_service;
       
        tcp::resolver resolver(io_service);
 
        tcp::resolver::query query("127.0.0.1", "7171");
       
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
       
        Connection conn(io_service);
        conn.connect(endpoint_iterator);

        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
   
    return 0;
}
