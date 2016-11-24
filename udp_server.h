#pragma once

#include <boost/asio.hpp>

#include <memory>
#include <string>
#include <queue>

class udp_server {
private:
    class udp_session : public std::enable_shared_from_this<udp_session> {
    public:
        udp_server &_udps;
        boost::asio::ip::udp::endpoint _endpoint_in;
        boost::asio::ip::udp::socket *_socket_out;
        char _buffer[8800];

        udp_session(udp_server &udp_server);

        ~udp_session();

        void process(size_t bytes_transferred);

        void receiveFromNext(const boost::system::error_code &err, 
                             size_t bytes_transferred);

        void sendToPrev(const boost::system::error_code &err, 
                        size_t bytes_transferred);
    };

    boost::asio::io_service &_ios;
    boost::asio::ip::udp::socket _socket_in;
    std::queue<boost::asio::ip::udp::socket*> _sockets;
    boost::asio::ip::udp::endpoint _endpoint_out;

public:
    udp_server(boost::asio::io_service &ios,
               std::string host, std::string port);

    ~udp_server();

    void receive();

    void handle_receive(std::shared_ptr<udp_session> session, 
                        const boost::system::error_code &err,
                        size_t bytes_transferred);
};
