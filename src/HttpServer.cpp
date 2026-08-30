#include "HttpServer.h"
#include "FormParser.h"
#include "UrlDecoder.h"
#include "MimeTypes.h"
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <cctype>
using namespace std;

HttpServer::HttpServer(int port, int threadCount)
    : port_(port),
      pool(threadCount)
{
}

bool equalsIgnoreCase(const std::string& a,
                      const std::string& b)
{
    if(a.size() != b.size())
    {
        return false;
    }

    for(size_t i = 0; i < a.size(); ++i)
    {
        if(std::tolower(
               static_cast<unsigned char>(a[i])) !=
           std::tolower(
               static_cast<unsigned char>(b[i])))
        {
            return false;
        }
    }

    return true;
}

bool sendAll(int client_fd, const std::string& response)
{
    size_t total_sent = 0;

    while(total_sent < response.size())
    {
        ssize_t bytes_sent = send(
            client_fd,
            response.data() + total_sent,
            response.size() - total_sent,
            0
        );

        if(bytes_sent <= 0)
        {
            return false;
        }

        total_sent += bytes_sent;
    }

    return true;
}

string ReceiveMessage(int client_fd, string& buffer_leftover){
    char buffer[1024];

    while(true){
        size_t leftover_pos = buffer_leftover.find("\r\n\r\n");
        if(leftover_pos != string::npos){
            string header_message = buffer_leftover.substr(0,leftover_pos);
            buffer_leftover = buffer_leftover.substr(leftover_pos+4);
            return header_message;
        }

        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer)-1, 0);
        if(bytes>0){
            buffer_leftover.append(buffer, bytes);
        }
        if(bytes == 0){
            cout<<"gracefully client disconnects."<<endl;
            return "";
        }
        if(bytes<1){
            cout<<"hardware or network error occurred."<<endl;
            return "";
        }
    }
}

void HttpServer::handleClient(int client_fd)
{
    string buffer_leftover = "";
    int body_length = 0;

        while(true){
            string headers_data = ReceiveMessage(client_fd, buffer_leftover);

            if(headers_data.empty()) break;
            cout << "===== RAW HEADERS =====" << endl;
            cout << headers_data << endl;
            cout << "=======================" << endl;
            ParseResult result =
                parser.parse(
                    client_fd,
                    headers_data,
                    buffer_leftover
                );
            if (result.status != ParseStatus::SUCCESS)
            {
                cout<<"Parsing failed"<<endl;
                HttpResponse response;
                if (result.status == ParseStatus::BAD_REQUEST)
                {
                    response.statusCode = 400;
                }
                else if(result.status == ParseStatus::METHOD_NOT_ALLOWED){
                    response.statusCode = 405;
                }
                else if(result.status == ParseStatus::HTTP_VERSION_NOT_SUPPORTED){
                    response.statusCode = 505;
                }
                else if(result.status == ParseStatus::PAYLOAD_TOO_LARGE){
                    response.statusCode = 413;
                }
                else if(result.status == ParseStatus::LENGTH_REQUIRED){
                    response.statusCode = 411;
                }
                
                response.headers["Connection"] = "close";
                std::string responseText = response.toString();
                sendAll(client_fd, responseText);
                break;
            }
            HttpRequest request = result.request;
            FormParser formparser;
            if(request.headers["Content-Type"] == "application/x-www-form-urlencoded"){
                request.form = formparser.parse(request.body);
            }

            bool keep_alive = false;
            if(request.version == "HTTP/1.1")
            {
                keep_alive = true;

                auto it = request.headers.find("Connection");

                if(it != request.headers.end() &&
                equalsIgnoreCase(it->second, "close"))
                {
                    keep_alive = false;
                }
            }
            else if(request.version == "HTTP/1.0")
            {
                auto it = request.headers.find("Connection");

                if(it != request.headers.end() &&
                equalsIgnoreCase(it->second, "keep-alive"))
                {
                    keep_alive = true;
                }
            }

            Router router;
            HttpResponse response = router.route(request);

            if(keep_alive)
            {
                response.headers["Connection"] = "keep-alive";
            }
            else
            {
                response.headers["Connection"] = "close";
            }

            std::string responseText = response.toString();
            if(!sendAll(client_fd, responseText))
            {
                std::cerr << "Failed to send response\n";
                break;
            }
            if (!keep_alive) {
                break;
            }
        }
    close(client_fd);
}

void HttpServer::start(){
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