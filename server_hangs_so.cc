#include <iostream>
#include <functional>
#include <string>
#include <asio.hpp>
#include <thread>
#include <memory>
#include <system_error>
#include <chrono>

//global variable for service and acceptor
asio::io_service ioService;
asio::ip::tcp::acceptor accp(ioService); 

const char* response = "HTTP/1.1 200 OK\r\n\r\n\r\n";

//callback for accept 
void onAccept(const std::error_code& ec, std::shared_ptr<asio::ip::tcp::socket> soc)
{
    using asio::ip::tcp;
    soc->set_option(asio::ip::tcp::no_delay(true));
    auto buf = new asio::streambuf;
    asio::async_read_until(*soc, *buf, "\r\n\r\n",
        [=](auto ec, auto siz) {
          asio::write(*soc, asio::buffer(response, std::strlen(response)));
          soc->shutdown(asio::ip::tcp::socket::shutdown_send);
          delete buf;
          soc->close();
        });
    auto nsoc = std::make_shared<tcp::socket>(ioService);
    //soc.reset(new tcp::socket(ioService));
    accp.async_accept(*nsoc, [=](const std::error_code& ec){
      onAccept(ec, nsoc);
    });

}

int main( int argc, char * argv[] )
{
    using asio::ip::tcp;
    asio::ip::tcp::resolver resolver(ioService);

    try{
        asio::ip::tcp::resolver::query query( 
            "127.0.0.1", 
            std::to_string(9999)
        );

     asio::ip::tcp::endpoint endpoint = *resolver.resolve( query );
     accp.open( endpoint.protocol() );
     accp.set_option( asio::ip::tcp::acceptor::reuse_address( true ) );
     accp.bind( endpoint );

     std::cout << "Ready to accept @ 9999" << std::endl;

     auto t1 = std::thread([&]() { ioService.run(); });
     auto t2 = std::thread([&]() { ioService.run(); });

     accp.listen( 1000 );

     std::shared_ptr<tcp::socket> soc = std::make_shared<tcp::socket>(ioService);

     accp.async_accept(*soc, [=](const std::error_code& ec) {
                                onAccept(ec, soc);
                              });

    t1.join();
    t2.join();
    } catch(const std::exception & ex){
      std::cout << "[" << std::this_thread::get_id()
        << "] Exception: " << ex.what() << std::endl;
    } catch (...) {
      std::cerr << "Caught unknown exception" << std::endl;
    }
}
