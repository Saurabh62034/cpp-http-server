#include "HttpResponse.h"


std::string HttpResponse::toString() const
{
    std::string statusText;

    switch(statusCode)
    {
        case 200: statusText = "OK"; break;
        case 303: statusText = "See Other"; break; // Required for redirects
        case 404: statusText = "Not Found"; break;
        case 500: statusText = "Internal Server Error"; break;
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
