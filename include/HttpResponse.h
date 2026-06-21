#pragma once

#include <string>

class HttpResponse
{
public:
    int statusCode = 200;

    std::string body;

    std::string contentType = "text/plain";

    std::string toString() const;
};