#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <iostream>
#include <sstream>
#include <thread>
#include <asio.hpp>

void process()
{
    std::cerr << "start"<< std::endl;
    std::istringstream is("<t>1</t>");
    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(is, pt); // <<< seg fault here
    std::cerr << std::endl << "end" << std::endl;
}

int main()
{
    asio::io_service io_service;
    io_service.post([](auto y) {process();});
    io_service.run();
    return 0;
}
