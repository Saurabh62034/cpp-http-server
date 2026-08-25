#pragma once

#include <string>
#include <unordered_map>

class HttpResponse
{
public:
    int statusCode = 200;
    std::string body;
    std::string contentType = "text/plain";
    std::string statusMessage;
    std::unordered_map<std::string, std::string> headers;

    std::string toString() const;
};
