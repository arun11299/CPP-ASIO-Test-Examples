#include <iostream>
#include <thread>
#include <asio.hpp>
#include <asio/use_future.hpp>

int get_sum(int a, int b)
{
  return a + b;
}

template <typename Func, typename CompletionFunction>
auto perform_asyncly(asio::io_service& ios, Func f, CompletionFunction cfun)
{

  using handler_type = typename asio::handler_type
                         <CompletionFunction, void(asio::error_code, int)>::type;

  handler_type handler{cfun};
  asio::async_result<handler_type> result(handler);

  ios.post([handler, f]() mutable {
        handler(asio::error_code{}, f(2, 2));
      });

  return result.get();
}

int main() {
  asio::io_service ios;
  asio::io_service::work wrk{ios};
  std::thread t{[&]{ ios.run(); }};
  auto res = perform_asyncly(ios, get_sum, asio::use_future);
  std::cout << res.get() << std::endl;

  t.join();

  return 0;
}
