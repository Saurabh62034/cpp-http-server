#include <iostream>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main()
{
    // 1. Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    // 2. Configure address
    sockaddr_in server_addr{};

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 3. Bind socket to port
    if (bind(server_fd,
             (sockaddr*)&server_addr,
             sizeof(server_addr)) < 0)
    {
        std::cerr << "Bind failed\n";
        return 1;
    }

    // 4. Listen
    if (listen(server_fd, 5) < 0)
    {
        std::cerr << "Listen failed\n";
        return 1;
    }

    std::cout << "Server listening on port 8080\n";

    while (true)
    {
        // 5. Accept client
        int client_fd = accept(server_fd, nullptr, nullptr);

        if (client_fd < 0)
        {
            std::cerr << "Accept failed\n";
            continue;
        }

        char buffer[4096] = {0};

        // 6. Read request
        ssize_t bytes = recv(
            client_fd,
            buffer,
            sizeof(buffer) - 1,
            0
        );

        if (bytes > 0)
        {
            std::cout << "Request Received:\n";
            std::cout << buffer << "\n";
        }

        // 7. HTTP Response
        const char* response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 22\r\n"
            "\r\n"
            "<h1>Hello Saurabh</h1>";

        send(
            client_fd,
            response,
            strlen(response),
            0
        );

        // 8. Close client connection
        close(client_fd);
    }

    close(server_fd);

    return 0;
}