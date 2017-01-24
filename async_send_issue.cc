//http://stackoverflow.com/questions/41650607/async-send-data-not-sent

//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2016 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ctime>
#include <iostream>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>

using boost::asio::ip::tcp;

std::string make_daytime_string()
{
        using namespace std; // For time_t, time and ctime;
        time_t now = time(0);
        //char now_string[128];
        return ctime(&now);
}

class tcp_connection
        : public boost::enable_shared_from_this<tcp_connection>
{
public:
        typedef boost::shared_ptr<tcp_connection> pointer;

        static pointer create(boost::asio::io_service& io_service)
        {
                return pointer(new tcp_connection(io_service));
        }

        tcp::socket& socket()
        {
                return socket_;
        }

        void start()
        {
                
                boost::asio::async_read(socket_, boost::asio::buffer(inputBuffer_),
                        boost::bind(&tcp_connection::handle_read, shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
                /* the snippet below works here - but not in handle_read 
                outputBuffer_ = make_daytime_string();
                
                boost::asio::async_write(socket_, boost::asio::buffer(outputBuffer_),
                        boost::bind(&tcp_connection::handle_write, shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));*/
        }

private:
        tcp_connection(boost::asio::io_service& io_service)
                : socket_(io_service)
        {
        }

        void handle_read(const boost::system::error_code& error, size_t bytes_transferred)
        {
                outputBuffer_ = make_daytime_string();

                if (strcmp(inputBuffer_, "time"))
                {                       
                        boost::asio::async_write(socket_, boost::asio::buffer(outputBuffer_),
                                boost::bind(&tcp_connection::handle_write, shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred)); 
                }
                else
                {
                        outputBuffer_ = "Something else was requested";//, 128);
                        boost::asio::async_write(socket_, boost::asio::buffer(outputBuffer_),
                                boost::bind(&tcp_connection::handle_write, shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
                }
        }

        void handle_write(const boost::system::error_code& error,
                size_t bytes_transferred)
        {
                if (!error)
                {
                        std::cout << "Bytes transferred: " << bytes_transferred;
                        std::cout << "Message sent: " << outputBuffer_;
                }
                else
                {
                        std::cout << "Error in writing: " << error.message();
                }
        }

        tcp::socket socket_;
        std::string message_;
        char inputBuffer_[128];
        std::string outputBuffer_;
};

class tcp_server
{
public:
        tcp_server(boost::asio::io_service& io_service)
                : acceptor_(io_service, tcp::endpoint(tcp::v4(), 13))
        {
                start_accept();
        }

private:
        void start_accept()
        {
                using std::cout;
                tcp_connection::pointer new_connection =
                        tcp_connection::create(acceptor_.get_io_service());

                acceptor_.async_accept(new_connection->socket(),
                        boost::bind(&tcp_server::handle_accept, this, new_connection,
                                boost::asio::placeholders::error));
                cout << "Done";
        }

        void handle_accept(tcp_connection::pointer new_connection,
                const boost::system::error_code& error)
        {
                if (!error)
                {
                        new_connection->start();
                }

                start_accept();
        }

        tcp::acceptor acceptor_;
};

int main()
{
        try
        {
                boost::asio::io_service io_service;
                tcp_server server(io_service);
                io_service.run();
        }
        catch (std::exception& e)
        {
                std::cerr << e.what() << std::endl;
        }

        return 0;
}
