#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <cstdlib>
#include <limits>

void handler( const boost::system::error_code& error , int signal_number )
{
    // Not safe to use stream here...
    std::cout << "handling signal " << signal_number << std::endl;
    exit (1);
}

int main( int argc , char** argv )
{
    boost::asio::io_service io_service;

    // Construct a signal set registered for process termination.
    boost::asio::signal_set signals(io_service, SIGINT );

    // Start an asynchronous wait for one of the signals to occur.
    signals.async_wait( handler );

    char choice;
    while( true )
    {
        std::cout << "Press a key: " << std::endl;
        std::cin >> choice; 
        io_service.run_one();
    }
}
