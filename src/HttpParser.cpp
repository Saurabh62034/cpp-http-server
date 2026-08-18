#include "HttpParser.h"
#include "UrlDecoder.h"
#include <iostream>
#include <sstream>
#include <cctype>
#include <sys/socket.h>
#include <iostream>
using namespace std;
ParseResult HttpParser::parse(
    int client_fd,
    const std::string& data,
    std::string& bufferLeftover)
{
    HttpRequest request;

    if (!parseRequestLine(data, request))
    {
        return {
            ParseStatus::BAD_REQUEST,
            request
        };
    }

    parseHeaders(data, request);
    parseQueryParams(request);
    parseBody(client_fd, request, bufferLeftover);

    return {
        ParseStatus::SUCCESS,
        request
    };
}

bool HttpParser::parseRequestLine(
    const std::string& data,
    HttpRequest& request)
{
    // 1. Request line cannot be empty
    if(data.empty())
    {
        return false;
    }

    std::stringstream ss(data);

    // 2. Method must exist
    if(!(ss >> request.method))
    {
        return false;
    }

    // 3. Request target/path must exist
    if(!(ss >> request.path))
    {
        return false;
    }

    // 4. HTTP version must exist
    if(!(ss >> request.version))
    {
        return false;
    }

    // 5. There must not be anything after HTTP version
    std::string extra;

    if(ss >> extra)
    {
        return false;
    }

    // 6. Validate HTTP version
    if(request.version != "HTTP/1.1" &&
       request.version != "HTTP/1.0")
    {
        return false;
    }

    // 7. Validate method
    if(request.method != "GET" &&
       request.method != "POST")
    {
        return false;
    }

    // 8. Request target cannot be empty
    if(request.path.empty())
    {
        return false;
    }

    // 9. Extract query parameters
    size_t queryPos = request.path.find("?");

    if(queryPos != std::string::npos)
    {
        request.query =
            request.path.substr(queryPos + 1);

        request.path =
            request.path.substr(0, queryPos);
    }

    // 10. Path cannot become empty
    if(request.path.empty())
    {
        return false;
    }

    // 11. URL decode
    request.path =
        UrlDecoder::decode(request.path);

    return true;
}

static std::string trim(const std::string& value)
{
    size_t start =
        value.find_first_not_of(" \t");

    size_t end =
        value.find_last_not_of(" \t");

    if(start == std::string::npos)
    {
        return "";
    }

    return value.substr(
        start,
        end - start + 1
    );
}

void HttpParser::parseHeaders(
    const std::string& data,
    HttpRequest& request)
{
    std::string leftBuffer = data;

    size_t firstLine = leftBuffer.find("\r\n");

    if(firstLine == std::string::npos)
    {
        return;
    }

    leftBuffer =
        leftBuffer.substr(firstLine + 2);

    while(!leftBuffer.empty())
    {
        size_t line = leftBuffer.find("\r\n");

        std::string message;

        if(line == std::string::npos)
        {
            // Last header has no trailing \r\n
            message = leftBuffer;
            leftBuffer.clear();
        }
        else
        {
            message =
                leftBuffer.substr(0, line);

            leftBuffer =
                leftBuffer.substr(line + 2);
        }

        size_t colon = message.find(":");

        // Invalid header
        if(colon == std::string::npos)
        {
            continue;
        }

        std::string key =
            trim(message.substr(0, colon));

        std::string value =
            trim(message.substr(colon + 1));

        request.headers[key] = value;

        
    }
}

void HttpParser::parseQueryParams(
    HttpRequest& request)
{
    std::string query = request.query;

    while(!query.empty())
    {
        size_t ampPos = query.find("&");

        std::string params;

        if(ampPos == std::string::npos)
        {
            params = query;
            query.clear();
        }
        else
        {
            params =
                query.substr(0, ampPos);

            query =
                query.substr(ampPos + 1);
        }

        size_t keySize = params.find("=");

        if(keySize == std::string::npos)
        {
            std::string key =
                UrlDecoder::decodeQueryComponent(params);

            request.queryParams[key] = "";

            continue;
        }

        std::string key =
            params.substr(0, keySize);

        std::string value =
            params.substr(keySize + 1);

        key =
            UrlDecoder::decodeQueryComponent(key);

        value =
            UrlDecoder::decodeQueryComponent(value);

        request.queryParams[key] = value;
    }

    for(auto &p:request.queryParams){
        cout<<p.first<<":"<<p.second<<endl;
    }
}

void HttpParser::parseBody(
    int client_fd,
    HttpRequest& request,
    std::string& bufferLeftover)
{
    auto it = request.headers.find("Content-Length");

    if(it == request.headers.end())
    {
        return;
    }

    size_t length = 0;

    try
    {
        length = std::stoul(it->second);
    }
    catch(const std::exception&)
    {
        return;
    }

    char buffer[1024];

    while(bufferLeftover.size() < length)
    {
        ssize_t bytes =
            recv(client_fd, buffer, sizeof(buffer), 0);

        if(bytes > 0)
        {
            bufferLeftover.append(buffer, bytes);
        }
        else if(bytes == 0)
        {
            std::cout
                << "Client disconnected while receiving body."
                << std::endl;

            return;
        }
        else
        {
            std::cout
                << "Error while receiving body."
                << std::endl;

            return;
        }
    }

    request.body =
        bufferLeftover.substr(0, length);

    bufferLeftover =
        bufferLeftover.substr(length);
}