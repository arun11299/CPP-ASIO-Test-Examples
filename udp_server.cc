#include <boost/bind.hpp>

#include "udp_server.h"

udp_server::udp_session::udp_session(udp_server &udp_server) : _udps(udp_server) { }

udp_server::udp_session::~udp_session() { }

void udp_server::udp_session::process(size_t bytes_transferred) {
        if(!_udps._sockets.empty()){
            _socket_out = _udps._sockets.back();
            _udps._sockets.pop();
        }else{
            _socket_out = new boost::asio::ip::udp::socket(_udps._ios,
                                   boost::asio::ip::udp::endpoint(
                                           boost::asio::ip::udp::v4(), 0));
        }
        _socket_out->async_send_to(boost::asio::buffer(_buffer, bytes_transferred), 
                                   _udps._endpoint_out,
                                   boost::bind(&udp_session::receiveFromNext, 
                                               shared_from_this(),           
                                               boost::asio::placeholders::error,                                
                                               boost::asio::placeholders::bytes_transferred));

}

void udp_server::udp_session::receiveFromNext(const boost::system::error_code &err,
                                              size_t bytes_transferred) {
    if (!err) {
        _socket_out->async_receive_from(boost::asio::buffer(_buffer, 8800),
                                        _udps._endpoint_out,
                                        boost::bind(&udp_session::sendToPrev,
                                                    shared_from_this(),
                                                    boost::asio::placeholders::error,
                                                    boost::asio::placeholders::bytes_transferred));
    }
}

void udp_server::udp_session::sendToPrev(const boost::system::error_code &err, 
                                         size_t bytes_transferred) {
    if (!err) {
            _udps._socket_in.send_to(boost::asio::buffer(_buffer, bytes_transferred),
                                     _endpoint_in);
            _udps._sockets.emplace(_socket_out);
    }
}

udp_server::udp_server(boost::asio::io_service &ios, cs_cache &cs,
                       std::string host, int port)
                      : _ios(ios)
                      , _socket_in(ios, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 3000)) {
    _endpoint_out = *boost::asio::ip::udp::resolver(ios).resolve({boost::asio::ip::udp::v4(), host, port}); 
    receive();
}

udp_server::~udp_server() { }

void udp_server::receive() {
    auto session = std::make_shared<udp_session>(*this);
    _socket_in.async_receive_from(boost::asio::buffer(session->_buffer, 8800),
                                  session->_endpoint_in,
                                  boost::bind(&udp_server::handle_receive,
                                              this,
                                              session,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
}

void udp_server::handle_receive(std::shared_ptr<udp_session> session, 
                                const boost::system::error_code &err,
                                size_t bytes_transferred) {
    if (!err) {
        _ios.dispatch(boost::bind(&udp_session::process, 
                                  session, 
                                  bytes_transferred));
    }
    receive();
}
