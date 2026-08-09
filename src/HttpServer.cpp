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
using namespace std;

HttpServer::HttpServer(int port, int threadCount)
    : port_(port),
      pool(threadCount)
{
}

void parseQueryParams(HttpRequest &request){
    string query = request.query;
    string params;
    while(!query.empty()){
        size_t amp_pos = query.find("&");
        if(amp_pos == string::npos){
            params = query;
            query.clear();
        }
        else{
            params = query.substr(0,amp_pos);
            query = query.substr(amp_pos+1);
        }
        
        size_t key_size = params.find("=");
        if(key_size == string::npos){
            string key = UrlDecoder::decodeQueryComponent(params);
            request.queryParams[key]="";
            continue;
        }
        string key = params.substr(0,key_size);
        string value = params.substr(key_size+1);
        key = UrlDecoder::decodeQueryComponent(key);
        value = UrlDecoder::decodeQueryComponent(value);
        request.queryParams[key] = value;
    }
}

HttpRequest HttpServer::parseRequest(const string& buffer)
{
    HttpRequest request;

    std::stringstream ss(buffer);
    ss >> request.method;
    ss >> request.path;
    ss >> request.version;

    size_t query_pos = request.path.find("?");
    string query = "";
    if(query_pos != string::npos){
        query = request.path.substr(query_pos+1);
        request.path = request.path.substr(0,query_pos);
    }

    request.path = UrlDecoder::decode(request.path);
    request.query = query;
    parseQueryParams(request);

    cout << MimeTypes::get("index.html") << endl;
cout << MimeTypes::get("style.css") << endl;
cout << MimeTypes::get("script.js") << endl;
cout << MimeTypes::get("image.png") << endl;
cout << MimeTypes::get("photo.jpg") << endl;
cout << MimeTypes::get("data.json") << endl;
cout << MimeTypes::get("something.xyz") << endl;
    return request;
}

std::string trim(const std::string& value)
{
    size_t start = value.find_first_not_of(" \t");
    size_t end = value.find_last_not_of(" \t");

    if (start == std::string::npos) {
        return "";
    }

    return value.substr(start, end - start + 1);
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
            buffer[bytes]  = '\0';
            buffer_leftover = buffer_leftover.append(buffer);
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
            HttpRequest request = parseRequest(headers_data);
            cout<<"decoded path = "<<request.path<<endl;
            

            string left_buffer = headers_data;
            int length  = 0;
            size_t first_line = left_buffer.find("\r\n");
            left_buffer = left_buffer.substr(first_line+2);

            while (!left_buffer.empty()) {
                size_t line = left_buffer.find("\r\n");
                if (line == string::npos) {
                    break;
                }
                string message = left_buffer.substr(0, line);
                size_t colon = message.find(":");
                 // Invalid header line
                if (colon == string::npos)
                {
                    left_buffer = left_buffer.substr(line + 2);
                    continue;
                }

                std::string key = trim(message.substr(0, colon));
                std::string value = trim(message.substr(colon + 1));

                request.headers[key] = value;
                left_buffer = left_buffer.substr(line + 2); 
            }

            char buffer[1024];
            if(request.headers.count("Content-Length")){

                length = stoi(request.headers["Content-Length"]);

                while(buffer_leftover.size()<length){
                    ssize_t bytes = recv(client_fd, buffer, sizeof(buffer)-1, 0);
                    if(bytes>0){
                        buffer[bytes] = '\0';
                        buffer_leftover = buffer_leftover.append(buffer,bytes);
                    }
                    else if(bytes == 0){
                        cout<<"user disconnects gracefully.";
                        break;
                    }
                    else{
                        cout<<"something went wrong.";
                        break;
                    }
                }
                request.body = buffer_leftover.substr(0, length);
                buffer_leftover = buffer_leftover.substr(length);
            }
            FormParser parser;
            if(request.headers["Content-Type"] == "application/x-www-form-urlencoded"){
                request.form = parser.parse(request.body);
            }

            bool keep_alive = false;
            if (request.version == "HTTP/1.1") {
                keep_alive = true;
                if (request.headers.count("Connection") &&
                    request.headers["Connection"] == "close") {
                    keep_alive = false;
                }
            }
            else if (request.version == "HTTP/1.0") {
                if (request.headers.count("Connection") &&
                    request.headers["Connection"] == "Keep-Alive") {
                    keep_alive = true;
                }
            }
            Router router;
            HttpResponse response = router.route(request);
            std::string responseText = response.toString();
            send(client_fd,
                responseText.c_str(),
                responseText.size(),
            0);
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