#include <iostream>
#include <array>
#include <string>
#include <system_error>
#include <asio.hpp>


void send_something(asio::io_service& ios, 
                    std::string host, int port, std::string message)
{
    asio::ip::tcp::endpoint
    endpoint(asio::ip::address::from_string(host), port);
    asio::generic::stream_protocol::socket socket{ios};
    //asio::ip::tcp::socket socket(ios);
    socket.connect(endpoint);

    std::error_code error;
    size_t len = 0;
    // Write the complete message
    while (len < message.length()) {
      len += socket.write_some(asio::buffer(message), error);
    }

    //std::array<char, 128> buf;
    //socket.read_some(asio::buffer(buf), error);
    std::cout << "B4" << std::endl;
    socket.async_read_some(asio::null_buffers(), 
        [socket = std::move(socket), error](std::error_code err, size_t) mutable {
          std::array<char, 128> buf;
          socket.read_some(asio::buffer(buf), error);
          std::cout << buf.data() << std::endl;
        });
    std::cout << "After" << std::endl;

    if (error == asio::error::eof)
    {
      std::cout << "Connection closed by server\n";
    }
    else if (error)
    {
      std::cout << "ERROR in connection" << std::endl;
      return;
    }

    //std::cout << "Received: " << buf.data() << std::endl;
}  


int main() {
  asio::io_service ios;
  send_something(ios, "127.0.0.1", 7869, "Hello!");
  ios.run();
  return 0;
}
