#include <iostream>
#include <asio.hpp>

int main() {
  asio::io_service io;
  asio::io_service::work w{io};
  io.post([](){ std::cout << "simple work" << std::endl; });
  io.run();
  return 0;
}
