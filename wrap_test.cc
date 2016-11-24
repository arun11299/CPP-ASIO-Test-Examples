#include <asio.hpp>
#include <iostream>
#include <cassert>

int main()
{
    asio::io_service ios1, ios2;
    asio::io_service::strand s2(ios2);
    auto test_func2 = wrap(s2, [&] {
    	        assert(s2.running_in_this_thread());
    	            });


    auto test_func = s2.wrap([&] {
	std::cout << "Executed" << std::endl;
        assert(s2.running_in_this_thread());
    });

    auto wrap_test_func = ios1.wrap(test_func);
    std::cout << "Here" << std::endl;

    wrap_test_func();
    std::cout << "Here 2" << std::endl;

    ios1.run_one();
    ios2.run_one();

}

