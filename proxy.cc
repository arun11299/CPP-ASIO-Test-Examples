#include <sys/types.h>  // socket, bind, listen, accept, send, recv, getaddrinfo
#include <sys/socket.h> // socket, bind, listen, accept, send, recv, getaddrinfo
#include <arpa/inet.h>  // htons
#include <sys/select.h> // select
#include <unistd.h>     // close, exit
#include <string.h>     // bzero
#include <stdlib.h>     // atoi
#include <netdb.h>      // getaddrinfo
#include <netinet/in.h> // struct sockaddr, sockaddr_in, 
#include <iostream>     // cout
#include <thread>
#include <string>       // find, substr

#define debug(...) std::cout << __VA_ARGS__ << '\n' << std::flush;

#define GET_THREAD_ID   std::this_thread::get_id()
#define CONNECT_200_OK  "HTTP/1.1 200 Connection established\r\nProxy-agent: myProxy\r\n\r\n"

class Proxy {
    public:
        Proxy() {
            listenSocket = -1;
            acceptSocket = -1;
        }
        ~Proxy() {
            debug("Proxy object destroyed");
        }
        bool startListening(int port) {
            struct sockaddr_in addr = { 0 };
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
            addr.sin_port = htons(port);

            listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (listenSocket < 0) {
                perror("socket()");
                return false;
            }
            if (bind(listenSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                perror("bind()");
                return false;
            }
            if (listen(listenSocket, 50) < 0) {
                perror("listen()");
                return false;
            }
            return true;
        }

        int acceptConnection(void) {
            struct sockaddr_in clientAddress = { 0 };
            unsigned int clientLen = sizeof(clientAddress);
            acceptSocket = accept(listenSocket, (struct sockaddr*)&clientAddress, &clientLen);
            return acceptSocket;
        }

        bool manageClient(int clientSocket) {
            char requestBuffer[16384];
            int requestSize;
            int serverSocket = -1;

            memset(requestBuffer, 0, sizeof(requestBuffer));
            if (getRequestFromClient(clientSocket, requestBuffer, requestSize)) {
                std::string hostToResolve;
                std::string requestBufferCPP(requestBuffer);
                if (parseRequest(hostToResolve, requestBufferCPP)) {
                    serverSocket = connectToRemoteServer(hostToResolve);
                    if (serverSocket < 0) {
                        debug(GET_THREAD_ID << "Failed to connect to server");
                        return false;
                    }
                    debug("Connected");
                    int send200okToClient = send(clientSocket, CONNECT_200_OK, strlen(CONNECT_200_OK), 0);
                    if (send200okToClient <= 0) {
                        perror("send");
                        return false;
                    }
                    else {
                        debug("200OK done");
                        if (!tunnelHTTPS(clientSocket, serverSocket)) {
                            debug("Failed HTTPS tunnel");
                            return false;
                        }
                        else {
                            debug("Connection went fine, quitting");
                            return true;
                        }
                    }
                }
            }
            else {
                debug("Failed to receive from client");
                close(clientSocket);
                close(serverSocket);
                return false;
            }
        }

    private:
        bool parseRequest(std::string &hostname, std::string &parseThis) {
            // CONNECT www.youtube.com:443 HTTP/1.1
            //        ^               ^
            std::size_t firstBlankPos = 8; // CONNECT + blank space
            std::size_t colonPos = parseThis.find(":");
            std::size_t hostnameSize = colonPos - firstBlankPos;
            hostname = parseThis.substr(firstBlankPos, hostnameSize);
            debug("I PARSED " << hostname);
            return true;
        }

        bool getRequestFromClient(int clientSocket, char *reqBuf, int &reqSize) {
            if (acceptSocket < 0)
                return false;
            int received = recv(clientSocket, reqBuf, 16384, 0);
            if (received < 0) {
                perror("recv()");
                return false;
            }
            else if (received == 0) {
                debug("Connection closed by client");
                return false;
            }
            else {
                // NO GET REQUESTS
                if (strstr(reqBuf, "GET")) {
                    debug("IGNORING GET REQUESTS, quitting");
                    return false;
                }
                reqSize = received;
                return true;
            }
        }

        // returns connected socket to remote server
        int connectToRemoteServer(std::string host) {
            struct addrinfo* result;
            struct addrinfo* res;
            int error, connectedSocket;
            struct sockaddr_in addr = { 0 };

            addr.sin_family = AF_INET;
            addr.sin_port = htons(443);

            int remoteSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (remoteSocket < 0) {
                perror("socket");
                return -1;
            }
            else {
                error = getaddrinfo(host.c_str(), NULL, NULL, &result);
                if (error != 0) {
                    if (error == EAI_SYSTEM) {
                        perror("getaddrinfo");
                    } else {
                        fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(error));
                    }
                    return -1;
                }
                // try to connect to remote server 
                for (res = result; res != NULL; res = res->ai_next) {
                    if (res->ai_family != AF_INET) continue;
                    struct sockaddr_in *addr_in = (struct sockaddr_in *)(res->ai_addr);
                    addr.sin_addr.s_addr = (addr_in->sin_addr.s_addr);
                    std::cout << inet_ntoa( addr_in->sin_addr ) << std::endl;
                    debug("Attempting to connect to " << host << " (" << addr.sin_addr.s_addr << ")");
                    connectedSocket = connect(remoteSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr));
                    std::cout << "Connected" << std::endl;
                    if (connectedSocket < 0) {
                        debug("Failed to connect to " << inet_ntoa(addr.sin_addr));
                        continue;
                    }
                    else {
                        debug("Connection established with " << inet_ntoa(addr.sin_addr));
                        break;
                    }
                }
                freeaddrinfo(result);
            }
            return connectedSocket;
        }

        // actual data exchange between client and server
        bool tunnelHTTPS(int client, int server) {
            int r;
            bool ret;
            int readFromServer = 0, sendToClient = 0;
            int readFromClient = 0, sendToServer = 0;
            struct timeval timeout;
            char buffer[1500];
            int bufferSize = sizeof(buffer);
            fd_set fdset;
            int maxfd = server >= client ? server+1 : client+1;
            debug("STARTING TUNNEL " << client << " and " << server);
            while(true) {
                bzero(buffer, bufferSize);
                FD_ZERO(&fdset);
                FD_SET(client, &fdset);
                FD_SET(server, &fdset);
                timeout.tv_sec = 5;
                timeout.tv_usec = 0;
                r = select(maxfd, &fdset, NULL, NULL, &timeout);
                // ERROR OR TIMEOUT
                if (r <= 0) {
                    close(client);
                    close(server);
                    ret = false;
                    break;
                }
                if (r == 0) {
                    continue;
                }
                // OK CLIENT ---> SERVER
                if (FD_ISSET(client, &fdset)) {
                    if ((readFromClient = recv(client, buffer, bufferSize, 0)) <= 0) {
                        close(client);
                        close(server);
                        ret = false;
                        break;
                    }
                    if ((sendToServer = send(server, buffer, readFromClient, 0)) <= 0) {
                        close(client);
                        close(server);
                        ret = false;
                        break;                      
                    }
                }
                // OK SERVER ---> CLIENT
                if (FD_ISSET(server, &fdset)) {
                    if ((readFromServer = recv(server, buffer, bufferSize, 0)) <= 0) {
                        close(client);
                        close(server);
                        ret = false;
                        break;
                    }
                    if ((sendToClient = send(client, buffer, readFromServer, 0)) <= 0) {
                        close(client);
                        close(server);
                        ret = false;
                        break;                      
                    }
                }
            }
        }

    private:
        int listenSocket;
        int acceptSocket;
};

int main(int argc, char *argv[]) {

    int port = atoi(argv[1]);
    Proxy p;

    if (p.startListening(port)) {
        debug(GET_THREAD_ID << " Listening on port " << port);
        while (true) {
            int newSocket = p.acceptConnection();
            if (newSocket > 0) {
                debug("Got new connection on clientfd " << newSocket <<" launching thread");
                std::thread handleConnThread(&Proxy::manageClient, &p, newSocket);
                handleConnThread.detach();
            }
            else {
                debug("Accept failed");
            }
        }
    }
    else {
        debug("Listen failed");
    }
    return 0;
}
