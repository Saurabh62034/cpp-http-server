#pragma once

#include "HttpRequest.h"

#include <string>

enum class ParseStatus
{
    SUCCESS,
    BAD_REQUEST,
    METHOD_NOT_ALLOWED,
    HTTP_VERSION_NOT_SUPPORTED,
    PAYLOAD_TOO_LARGE,
    LENGTH_REQUIRED
};
constexpr size_t MAX_BODY_SIZE = 10 * 1024 * 1024;
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
    ParseStatus parseRequestLine(
        const std::string& data,
        HttpRequest& request
    );

    bool parseHeaders(
        const std::string& data,
        HttpRequest& request
    );

    void parseQueryParams(
        HttpRequest& request
    );

    ParseStatus parseBody(
        int client_fd,
        HttpRequest& request,
        std::string& bufferLeftover
    );

};


