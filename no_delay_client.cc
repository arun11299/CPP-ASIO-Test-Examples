#include <asio.hpp>
#include <thread>
#include <chrono>
using namespace asio;
struct Client
{
   io_service svc;
   ip::tcp::socket sock;

   Client() : svc(), sock(svc)
   {
     ip::tcp::endpoint endpoint( ip::address::from_string("127.0.0.1"), 32323);
     sock.open(ip::tcp::v4());
     sock.set_option(ip::tcp::no_delay(true));
     sock.connect(endpoint);
   }

   void send(std::string const& message) {
       sock.send(buffer(message));
   }
};

int main()
{
   std::thread t([]() {
        Client client;
        client.send("hello world\n");
        client.send("bye world\n");
       });

        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        t.join();
}
