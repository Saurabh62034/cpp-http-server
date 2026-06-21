#include "HttpResponse.h"

std::string HttpResponse::toString() const
{
    std::string statusText;

    switch(statusCode)
    {
        case 200:
            statusText = "OK";
            break;

        case 404:
            statusText = "Not Found";
            break;

        case 500:
            statusText = "Internal Server Error";
            break;

        default:
            statusText = "Unknown";
    }

    return
        "HTTP/1.1 " +
        std::to_string(statusCode) +
        " " +
        statusText +
        "\r\n"
        "Content-Type: " +
        contentType +
        "\r\n"
        "Content-Length: " +
        std::to_string(body.size()) +
        "\r\n\r\n" +
        body;
}