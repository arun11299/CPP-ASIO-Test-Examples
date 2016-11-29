#include <thread>
#include <chrono>
#include <vector>
#include <signal.h>
#include <asio.hpp>
#include <system_error>

namespace
{
bool keepGoing = true;
void shutdown(int)
{
        keepGoing = false;
}

std::size_t bytesAccum = 0;
void justReceive(std::error_code ec, std::size_t bytesReceived,
    asio::ip::tcp::socket &socket, std::vector<unsigned char> &buffer)
{
        bytesAccum += bytesReceived;
/*
        auto end = buffer.begin() + bytesReceived;
        for (auto it = buffer.begin(); it != end; ++it)
        {
                if (*it == 'e')
                {
                        std::printf("server got: %lu\n", bytesAccum);
                        bytesAccum = 0;
                }
        }
*/
        socket.async_receive(
            asio::buffer(buffer),
            0,
            [&] (auto ec, auto bytes) {
              justReceive(ec, bytes, socket, buffer);
            });
}
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

        asio::ip::tcp::acceptor acceptor(io,
            asio::ip::tcp::endpoint(
                asio::ip::address::from_string("127.0.0.1"), 1234));
        asio::ip::tcp::socket socket(io);

        // accept 1 client
        std::vector<unsigned char> buffer(131072, 0);
        acceptor.async_accept(socket, [&socket, &buffer](std::error_code ec)
        {
            // options
            //socket.set_option(asio::ip::tcp::no_delay(true)); 
            socket.set_option(asio::socket_base::receive_buffer_size(131072  * 4));
            socket.set_option(asio::socket_base::send_buffer_size(131072 * 4));

            socket.async_receive(
                asio::buffer(buffer),
                0,
                [&](auto ec, auto bytes) {
                  justReceive(ec, bytes, socket, buffer);
                });
        });

        while (keepGoing)
        {
                std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        io.stop();

        t1.join();
        t2.join();
        t3.join();
        t4.join();

        std::printf("server: goodbye\n");
}
