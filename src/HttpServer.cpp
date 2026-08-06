#include "HttpServer.h"
#include "FormParser.h"
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

HttpRequest HttpServer::parseRequest(const string& buffer)
{
    HttpRequest request;

    std::stringstream ss(buffer);

    ss >> request.method;
    ss >> request.path;
    ss >> request.version;
    // ss>> request.content_length;
    // ss>> request.body_message;
    // ss>> request.next_message;
    // ss>> request.next_message2;
    // ss>> request.next_message3;
    // ss>> request.next_message4;
    // ss>> request.next_message5;
    // ss>> request.next_message6;


    return request;
}


string ReceiveMessage(int client_fd, string& buffer_leftover){
    cout<<"Trying to receive the message"<<endl;
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
            cout<<"message received"<<endl;
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
    std::cout << "handleClient start\n";
    string buffer_leftover = "";
    int body_length = 0;

        while(true){
            string complete_message = ReceiveMessage(client_fd, buffer_leftover);

            std::cout << "Request received\n";
            if(complete_message.empty()) break;
            HttpRequest request = parseRequest(complete_message);
            string left_buffer = complete_message;
            // cout<<"complete_message = "<<complete_message<<endl;
            // cout<<"buffer_leftover= "<<buffer_leftover<<endl;
            int length  = 0;
            size_t first_line = left_buffer.find("\r\n");
            left_buffer = left_buffer.substr(first_line+2);

            while (!left_buffer.empty()) {
                size_t line = left_buffer.find("\r\n");
                if (line == string::npos) {
                    std::cout << left_buffer << endl;
                    break;
                }
                
                string message = left_buffer.substr(0, line);
                size_t colon = message.find(":");

                request.headers[message.substr(0,colon)] = message.substr(colon+1, message.size());
                left_buffer = left_buffer.substr(line + 2); 
            }

            char buffer[1024];
            if(request.headers.count("Content-Length")){

                cout<<"content length== : "<<stoi(request.headers["Content-Length"])<<endl;
                length = stoi(request.headers["Content-Length"]);

                while(buffer_leftover.size()<length){
                    cout<<"Body was incomplete so calling recv again"<<endl;
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
                cout<<"int length = "<<length<<endl;
            }
            cout<<"body content = "<<request.body<<endl;
            std::cout<<"path requested = "<<request.method<<endl;
            FormParser parser;
            if(request.headers["Content-Type"] == " application/x-www-form-urlencoded"){
                
                request.form = parser.parse(request.body);
            }
            for(auto &p:request.form){
                cout<<p.first<<": "<<p.second<<endl;
            }
            
            Router router;
            HttpResponse response = router.route(request);
            std::string responseText = response.toString();
            

            std::cout<< "Response size = "<< responseText.size()<< '\n';
            send(client_fd,
                responseText.c_str(),
                responseText.size(),
            0);
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