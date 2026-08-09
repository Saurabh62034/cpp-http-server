#pragma once

#include "HttpRequest.h"

#include <string>

class HttpParser
{
public:
    HttpRequest parse(int client_fd,
        const std::string& data,
        std::string& bufferLeftover);

private:
    void parseRequestLine(
        const std::string& data,
        HttpRequest& request
    );

    void parseHeaders(
        const std::string& data,
        HttpRequest& request
    );

    void parseQueryParams(
        HttpRequest& request
    );

    void parseBody(
        int client_fd,
        HttpRequest& request,
        std::string& bufferLeftover
    );
};