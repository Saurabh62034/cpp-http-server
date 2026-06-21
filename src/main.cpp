#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd < 0)
    {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    std::cout << server_fd << '\n';
    std::cout << "Socket created successfully\n";

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    int result = bind(server_fd, (struct sockaddr*)&address, sizeof(address));

    if(result < 0)
    {
        std::cerr << "Bind failed\n";
        return 1;
    }
    std::cout << "Bind successful On Port\n"<< result << '\n';
    close(server_fd);

    return 0;
}