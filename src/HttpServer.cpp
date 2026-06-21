#include "HttpServer.h"

#include <iostream>
#include <sstream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>

HttpServer::HttpServer(int port, int threadCount)
    : port_(port),
      pool(threadCount)
{
}

HttpRequest HttpServer::parseRequest(const char* buffer)
{
    HttpRequest request;

    std::stringstream ss(buffer);

    ss >> request.method;
    ss >> request.path;
    ss >> request.version;

    return request;
}

void HttpServer::handleClient(int client_fd)
{
    std::cout << "handleClient start\n";
    while(true)
    {
        char buffer[4096];

        ssize_t bytes =
            recv(client_fd,
                 buffer,
                 sizeof(buffer)-1,
                 0);

        if(bytes <= 0)
        {
            break;
        }

        buffer[bytes] = '\0';
        std::cout << "Request received\n";
        

        HttpRequest request =
            parseRequest(buffer);

        Router router;

        HttpResponse response =
            router.route(request);

        std::string responseText =
            response.toString();

            std::cout
    << "Response size = "
    << responseText.size()
    << '\n';
        send(client_fd,
             responseText.c_str(),
             responseText.size(),
             0);
    }

    close(client_fd);
}

void HttpServer::start()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd < 0)
    {
        std::cerr << "Socket creation failed\n";
        return;
    }

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if(bind(server_fd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) < 0)
    {
        std::cerr << "Bind failed\n";
        return;
    }

    if(listen(server_fd, SOMAXCONN) < 0)
    {
        std::cerr << "Listen failed\n";
        return;
    }

    std::cout
        << "Server listening on port "
        << port_
        << '\n';

    while(true)
    {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);

        int client_fd =
            accept(server_fd,
                   reinterpret_cast<sockaddr*>(&clientAddr),
                   &clientLen);

        if(client_fd < 0)
        {
            continue;
        }

        pool.enqueue(
            [this, client_fd]()
            {
                handleClient(client_fd);
            });
    }

    close(server_fd);
}