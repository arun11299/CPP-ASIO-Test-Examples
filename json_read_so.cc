#include <asio.hpp>
#include <iostream>
#include <sstream>
#include <istream>

int main(int argc, const char * argv[]) {

    static asio::io_service ios;
    asio::ip::tcp::endpoint
    endpoint(asio::ip::address::from_string("127.0.0.1"), 8000);
    asio::generic::stream_protocol::socket socket{ios};
    socket.connect(endpoint);

    std::string req = "GET /api\n\r\n\r\n";
    std::error_code ec;
    socket.write_some(asio::buffer(req, req.size()), ec);
    std::cout << "Request sent" << std::endl;

    if (ec == asio::error::eof) {
      std::cerr << "Premature termination\n";
      return 1;
    }
    //while (true) {
    {
          asio::streambuf response;
          // Get till all the headers
          asio::read_until(socket, response, "\r\n\r\n");

          // Check that response is OK. 
          std::istream response_stream(&response);
          std::string http_version;
          response_stream >> http_version;
          std::cout << "Version : " << http_version << std::endl;

          unsigned int status_code;
          response_stream >> status_code;

          std::string status_message;
          std::getline(response_stream, status_message);

          if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
            std::cerr << "invalid response";
            return -1; 
          }

          if (status_code != 200) {
            std::cerr << "response did not returned 200 but " << status_code;
            return -1; 
          }

          //read the headers.
          std::string header;
          while (std::getline(response_stream, header) && header != "\r") {
            std::cout << "H: " << header << std::endl;
          }

          bool chunk_size_present = false;
          std::string chunk_siz_str;

          // Ignore the remaining additional '\r\n' after the header
          std::getline(response_stream, header);

          // Read the Chunk size
          asio::read_until(socket, response, "\r\n");
          std::getline(response_stream, chunk_siz_str);
          std::cout << "CS : " << chunk_siz_str << std::endl;
          size_t chunk_size = (int)strtol(chunk_siz_str.c_str(), nullptr, 16);


          // Now how many bytes yet to read from the socket ?
          // response might have some additional data still with it
          // after the last `read_until`
          auto chunk_bytes_to_read = chunk_size - response.size();

          std::cout << "Chunk Length = " << chunk_size << std::endl;
          std::cout << "Additional bytes to read: " << response_stream.gcount() << std::endl;

          std::error_code error;
          size_t n = asio::read(socket, response, asio::transfer_exactly(chunk_bytes_to_read), error);

          if (error) {
            return -1; //throw boost::system::system_error(error);
          }

          std::ostringstream ostringstream_content;
          ostringstream_content << &response;

          auto str_response = ostringstream_content.str();
          std::cout << str_response << std::endl;
    }

    //}

    return 0;
}

