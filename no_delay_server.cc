#include <asio.hpp>
#include <iostream>
#include <system_error>

int main() {
    asio::io_service        io_service;
    asio::ip::tcp::acceptor acceptor(io_service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 32323));
    asio::ip::tcp::socket   socket(io_service);

    acceptor.accept(socket);
    socket.set_option(asio::ip::tcp::no_delay(true));

    asio::streambuf sb;
    std::error_code ec;
    while (asio::read(socket, sb, ec)) {
        std::cout << "received:\n" << &sb;
    }
}
