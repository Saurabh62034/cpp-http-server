#include "Router.h"
#include "HandlePost.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>


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
    else if(request.path == "/create-user"){
        response.statusCode = 200;
        response.contentType = "text/html";
        response.body = readFile("../public/create-user.html");
    }
    else if(request.path == "/saveData" && request.method == "POST"){
        SaveData save_data(request.body);
        save_data.Store();
        
        response.statusCode = 303; 
        response.statusMessage = "See Other";
        response.headers["Location"] = "/save-after"; 
    }
    else if(request.path == "/save-after" && request.method == "GET"){
        response.statusCode = 200;
        response.contentType = "text/html";
        response.body = readFile("../public/after-save.html");
        size_t pos = response.body.find("{{BODY}}");
        std::string name = "";
        if(request.form.count("name")){
            name = request.form.at("name");
        }
        if (pos != std::string::npos)
        {
            response.body.replace(
                pos,
                8,               // length of "{{BODY}}"
                name
            );
        }

        size_t email_pos = response.body.find("{{EMAIL}}");

        std::string email = "";
        if(request.form.count("email")){
            email = request.form.at("email");
        }

        if(email_pos!=std::string::npos){
            response.body.replace(
                email_pos,
                9,
                email
            );
        }
    }
   
    else
    {
        response.statusCode = 404;
        response.body = "Page Not Found";
    }

    return response;
}