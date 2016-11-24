#include <iostream>
#include <asio.hpp>
#include <thread>
#include <type_traits>

struct Job
{
  Job() = default;

  Job(const Job&) = delete;
  void operator=(const Job&) = delete;

  void operator()() {
    std::cout << "performing job. prev = " << completed_ << '\n';
    completed_++;
  }

  static size_t completed_;
};

size_t Job::completed_ = 0;

class JobExecutor
{
public:
  JobExecutor(asio::io_service& ios): serial_executor_(ios)
  {}

  template <typename T,
            typename = typename std::enable_if_t<std::is_same<std::decay_t<T>, Job>::value>>
  void execute(T&& job)
  {
    serial_executor_.post( [&]() { std::forward<T>(job)(); } );
  }

private:
  asio::io_service::strand serial_executor_;
};

int main() {
  asio::io_service ios;
  auto wrk (std::make_unique<asio::io_service::work>(ios));

  std::thread ios_thrd( [&]() { ios.run(); } );
  JobExecutor jexec{ios};

  for (int i = 0; i < 100; i++) {
    jexec.execute(Job{});
  }

  wrk.reset();

  ios_thrd.join();
  return 0;
}
