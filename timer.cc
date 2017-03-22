#include <iostream>
#include <asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

int main() {
  asio::io_service io_s;
  asio::deadline_timer timer(io_s);
  timer.expires_from_now(boost::posix_time::seconds(5));
  timer.async_wait([](auto err_c) { std::cout << "After 5 seconds" << std::endl; } );

  io_s.reset();

  io_s.run();

  return 0;
}
