#include "Router.h"
#include <iostream>
#include <fstream>
#include <sstream>


std::string readFile(const std::string& path)
{
    std::ifstream file(path);

    if(!file)
    {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

HttpResponse Router::route(const HttpRequest& request)
{
    HttpResponse response;

    if(request.path == "/")
    {
        response.statusCode = 200;
        response.contentType = "text/html";
        response.body = readFile("../public/index.html");
    }
    else if(request.path == "/about")
    {
        response.statusCode = 200;
        response.contentType = "text/html";
        response.body = readFile("../public/about.html");
    }
    else
    {
        response.statusCode = 404;
        response.body = "Page Not Found";
    }

    return response;
}