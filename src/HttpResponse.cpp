#include "HttpResponse.h"


std::string HttpResponse::toString() const
{
    std::string statusText;

    switch(statusCode)
    {
        case 200: statusText = "OK"; break;
        case 303: statusText = "See Other"; break; // Required for redirects
        case 400: statusText = "Bad Request"; break;
        case 404: statusText = "Not Found"; break;
        case 405: statusText = "Method Not Allowed"; break;
        case 411: statusText = "Length Required"; break;
        case 413: statusText = "Content Too Large"; break;
        case 500: statusText = "Internal Server Error"; break;
        case 505: statusText = "HTTP Version not supported"; break;
        default:  statusText = "Unknown";
    }

    std::string responseString = "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n";

    responseString += "Content-Type: " + contentType + "\r\n";
    responseString += "Content-Length: " + std::to_string(body.size()) + "\r\n";

    for (const auto& header : headers) {
        responseString += header.first + ": " + header.second + "\r\n";
    }

    responseString += "\r\n" + body;

    return responseString;
}
