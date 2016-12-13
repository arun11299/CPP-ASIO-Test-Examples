#include <iostream>
#include <type_traits>
#include <thread>
#include <memory>
#include <asio.hpp>

template <typename Handler>
class GenHandler
{
public:
  GenHandler(Handler&& h): hndler_(std::forward<Handler>(h))
  {}

  template <typename... Args>
  void operator()(Args&&... args)
  {
    std::cout << "GenHandler called" << std::endl;
    hndler_();
  }
private:
  Handler hndler_;
};

template<typename HandlerType>
GenHandler<std::decay_t<HandlerType>> create_handler(HandlerType&& h)
{
  return {std::forward<HandlerType>(h)};
}

template <typename Handler>
void SomeEvent(asio::io_service& ios, Handler& h)
{
  ios.post([=] ()mutable { h(); });
}

int main() {
  asio::io_service ios;
  asio::io_service::strand strand{ios};
  auto work = std::make_unique<asio::io_service::work>(ios);
  std::thread t([&]() { ios.run(); });

  auto hndl = create_handler([] { std::cout << "Regular Handle" << std::endl; });
  SomeEvent(ios, hndl);

  auto hndl2 = create_handler(strand.wrap([] { std::cout << "Strand handler-depth 2" << std::endl; }));
  auto str_handler = strand.wrap([=]() mutable { std::cout <<"strand\n"; hndl2();  });
  SomeEvent(ios, str_handler);
  work.reset();

  t.join();
  return 0;
}
