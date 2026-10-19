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
#include <cerrno>
#include <cstring>
#include <utility>
using namespace std;

HttpServer::HttpServer(int port, int threadCount)
    : port_(port),
      pool(threadCount)
{
}
namespace{
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

bool hasHeaderToken(const std::string& value,
                    const std::string& expected)
{
    std::size_t start = 0;

    while (start < value.size())
    {
        const std::size_t comma = value.find(',', start);
        const std::size_t end =
            (comma == std::string::npos) ? value.size() : comma;

        std::size_t first = value.find_first_not_of(" \t", start);

        if (first != std::string::npos && first < end)
        {
            std::size_t last = value.find_last_not_of(" \t", end - 1);

            const std::string token =
                value.substr(first, last - first + 1);

            if (equalsIgnoreCase(token, expected))
            {
                return true;
            }
        }

        if (comma == std::string::npos)
        {
            break;
        }

        start = comma + 1;
    }

    return false;
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

}
void HttpServer::handleClient(int client_fd)
{
    std::string input_buffer;

    while (true)
    {
        // The parser examines only the bytes already received.
        ParseResult result = parser.parse(input_buffer);

        // If the request is incomplete, receive more bytes.
        // Socket I/O belongs here, not inside HttpParser.
        while (result.status == ParseStatus::NEED_MORE_DATA)
        {
            char buffer[4096];

            const ssize_t bytes = recv(
                client_fd,
                buffer,
                sizeof(buffer),
                0
            );

            if (bytes > 0)
            {
                input_buffer.append(
                    buffer,
                    static_cast<std::size_t>(bytes)
                );

                result = parser.parse(input_buffer);
                continue;
            }

            if (bytes == 0)
            {
                // EOF: the peer closed the connection.
                // Any incomplete request is discarded.
                if (!input_buffer.empty())
                {
                    std::cerr
                        << "Client disconnected with an "
                        << "incomplete HTTP request.\n";
                }

                close(client_fd);
                return;
            }

            if (errno == EINTR)
            {
                // The receive was interrupted by a signal.
                // Retry without losing buffered input.
                continue;
            }

            std::cerr
                << "Failed to receive request: "
                << strerror(errno)
                << '\n';

            close(client_fd);
            return;
        }

        // A complete request was not produced.
        if (result.status != ParseStatus::SUCCESS)
        {
            std::cerr << "HTTP request parsing failed.\n";

            HttpResponse response;

            switch (result.status)
            {
                case ParseStatus::BAD_REQUEST:
                    response.statusCode = 400;
                    break;

                case ParseStatus::METHOD_NOT_ALLOWED:
                    response.statusCode = 405;
                    break;

                case ParseStatus::HTTP_VERSION_NOT_SUPPORTED:
                    response.statusCode = 505;
                    break;

                case ParseStatus::PAYLOAD_TOO_LARGE:
                    response.statusCode = 413;
                    break;

                case ParseStatus::LENGTH_REQUIRED:
                    response.statusCode = 411;
                    break;

                case ParseStatus::SUCCESS:
                case ParseStatus::NEED_MORE_DATA:
                    // These cases are handled before this switch.
                    break;
            }

            response.headers["Connection"] = "close";

            const std::string responseText = response.toString();

            sendAll(client_fd, responseText);

            break;
        }

        // SUCCESS must identify a non-empty, valid request
        // boundary. Never erase bytes without a valid boundary.
        if (result.bytesConsumed == 0 ||
            result.bytesConsumed > input_buffer.size())
        {
            std::cerr
                << "Invalid parser consumption result.\n";

            break;
        }

        HttpRequest request = std::move(result.request);

        // Remove only the completed request.
        // Bytes belonging to the next request remain buffered.
        input_buffer.erase(0, result.bytesConsumed);

        // Preserve existing form-parsing behavior.
        FormParser formparser;

        const auto contentTypeIt = request.headers.find("content-type");

        if (contentTypeIt != request.headers.end() &&
            equalsIgnoreCase(contentTypeIt->second, "application/x-www-form-urlencoded"))
        {
            request.form = formparser.parse(request.body);
        }

        // Preserve the current HTTP/1.0 and HTTP/1.1
        // keep-alive policy for this refactoring step.
        bool keep_alive = false;

        const auto connectionIt = request.headers.find("connection");
        const bool hasConnectionHeader =
            connectionIt != request.headers.end();

        const bool hasClose =
            hasConnectionHeader &&
            hasHeaderToken(connectionIt->second, "close");

        const bool hasKeepAlive =
            hasConnectionHeader &&
            hasHeaderToken(connectionIt->second, "keep-alive");

        if (request.version == "HTTP/1.1")
        {
            // Persistent connections are the default in HTTP/1.1.
            keep_alive = !hasClose;
        }
        else if (request.version == "HTTP/1.0")
        {
            // HTTP/1.0 requires an explicit keep-alive token.
            // "close" takes precedence if both tokens are present.
            keep_alive = hasKeepAlive && !hasClose;
        }

        // Existing application boundary remains unchanged.
        Router router;
        HttpResponse response = router.route(request);

        if (keep_alive)
        {
            response.headers["Connection"] = "keep-alive";
        }
        else
        {
            response.headers["Connection"] = "close";
        }

        const std::string responseText = response.toString();

        if (!sendAll(client_fd, responseText))
        {
            std::cerr << "Failed to send response.\n";
            break;
        }

        if (!keep_alive)
        {
            break;
        }

        // Continue the loop. If another complete request is
        // already buffered, it can be parsed without recv().
        // Otherwise, the next iteration waits for more bytes.
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