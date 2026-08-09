#include "Router.h"
#include "HandlePost.h"
#include "MimeTypes.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_map>


std::string readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);

    if(!file)
    {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

std::string getStaticFilePath(const std::string& requestPath)
{
    namespace fs = std::filesystem;

    fs::path publicRoot = fs::weakly_canonical("../public");

    fs::path relativePath = requestPath.substr(1);

    fs::path requestedPath =
        fs::weakly_canonical(publicRoot / relativePath);

    if(!fs::exists(requestedPath) ||
       !fs::is_regular_file(requestedPath))
    {
        return "";
    }

    std::string root = publicRoot.string();
    std::string file = requestedPath.string();

    if(file.compare(0, root.size(), root) != 0)
    {
        return "";
    }

    return requestedPath.string();
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
        SaveData save_data(request.form);
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
        // Static file serving
        std::string filePath =
            getStaticFilePath(request.path);

        if(filePath.empty())
        {
            response.statusCode = 404;
            response.statusMessage = "Not Found";
            response.contentType = "text/plain";
            response.body = "Page Not Found";
        }
        else
        {
            response.statusCode = 200;
            response.contentType =
                MimeTypes::get(filePath);

            response.body =
                readFile(filePath);
        }
    }

    return response;
}