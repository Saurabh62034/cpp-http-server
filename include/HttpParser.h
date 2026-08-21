#pragma once

#include "HttpRequest.h"

#include <string>

enum class ParseStatus
{
    SUCCESS,
    BAD_REQUEST,
    METHOD_NOT_ALLOWED,
    HTTP_VERSION_NOT_SUPPORTED
};

struct ParseResult
{
    ParseStatus status;
    HttpRequest request;
};

class HttpParser
{
public:
    ParseResult parse(int client_fd,
        const std::string& data,
        std::string& bufferLeftover);

private:
    bool parseRequestLine(
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


