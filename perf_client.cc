#include <thread>
#include <chrono>
#include <vector>
#include <signal.h>
#include <asio.hpp>
#include <system_error>

namespace
{
bool keepGoing = true;
void shutdown(int) { keepGoing = false; }
}

int main(int, char **)
{
        signal(SIGINT, shutdown);

        asio::io_service io;
        asio::io_service::work work(io);

        std::thread t1([&]() { io.run(); });
        std::thread t2([&]() { io.run(); });
        std::thread t3([&]() { io.run(); });
        std::thread t4([&]() { io.run(); });

        asio::ip::tcp::socket socket(io);
        auto endpoint = asio::ip::tcp::resolver(io).resolve({ 
            "127.0.0.1", "1234" });
        asio::connect(socket, endpoint);

        // options to test
        //socket.set_option(asio::ip::tcp::no_delay(true)); 
        socket.set_option(asio::socket_base::receive_buffer_size(131072 * 4));
        socket.set_option(asio::socket_base::send_buffer_size(131072 * 4));

        std::vector<unsigned char> buffer(131072, 0);
        buffer.back() = 'e';

        std::chrono::time_point<std::chrono::system_clock> last = 
            std::chrono::system_clock::now();
        
        std::chrono::duration<double> delta = std::chrono::seconds(0);

        std::size_t bytesSent = 0;

        while (keepGoing)
        {
                // blocks during send
                asio::write(socket, asio::buffer(buffer));
                //socket.send(asio::buffer(buffer));

                // accumulate bytes sent
                bytesSent += buffer.size();

                // accumulate time spent sending
                delta += std::chrono::system_clock::now() - last;
                last = std::chrono::system_clock::now();

                // print information periodically
                if (delta.count() >= 5.0) 
                {
                        std::printf("Mbytes/sec: %f, Gbytes/sec: %f, Mbits/sec: %f, Gbits/sec: %f\n",
                                    bytesSent / 1.0e6 / delta.count(),
                                    bytesSent / 1.0e9 / delta.count(),
                                    8 * bytesSent / 1.0e6 / delta.count(),
                                    8 * bytesSent / 1.0e9 / delta.count());

                        // reset accumulators
                        bytesSent = 0;
                        delta = std::chrono::seconds(0);
                }
        }

        io.stop();

        t1.join();
        t2.join();
        t3.join();
        t4.join();

        std::printf("client: goodbyte\n");
}
