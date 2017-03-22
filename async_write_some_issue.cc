#include <asio.hpp>
#include <iostream>
#include <system_error>

int main()
{
  asio::io_service io;

  asio::ip::tcp::socket sock(io);
  sock.open(asio::ip::tcp::v4());
  auto const fd = sock.native_handle();

  int ret;

  // make the socket non-blocking
  unsigned long on = 1;
  ret = fcntl(fd, F_SETFD, O_NONBLOCK);
  if (ret == -1)
    throw std::runtime_error("ioctl");

  // connect to 127.0.0.1:1 which should be a closed port
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(6969);
  ret = inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
  if (ret <= 0)
    throw std::runtime_error("pton error");

  // do the connect and ignore "connection in progress" error
  ret = connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
  if (ret < 0)
  {
    throw std::runtime_error(std::strerror(errno));
  }

  bool finished = false;
#if 1 // this doesn't work
  sock.async_write_some(
#else // this works
  asio::async_write(sock,
#endif
      asio::null_buffers(),
      [&](std::error_code ec, std::size_t s)
      {
        std::cout << ec << std::endl;
        finished = true;
      });

  while (!finished)
    io.run_one();

  std::cout << "FINISHED" << std::endl;
}
