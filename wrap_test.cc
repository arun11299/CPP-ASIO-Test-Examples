#include <asio.hpp>
#include <iostream>
#include <cassert>

int main()
{
    asio::io_service ios1, ios2;
    asio::io_service::strand s2(ios2);
    auto test_func = s2.wrap([&]() {
      assert(s2.running_in_this_thread());
    });


    auto wrap_test_func = ios1.wrap(test_func);
    wrap_test_func();

    ios1.run_one();
    ios2.run_one();
}

