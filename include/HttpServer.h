#pragma once

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Router.h"
#include "ThreadPool.h"

class HttpServer
{
public:
    HttpServer(int port, int threadCount);

    void start();

private:
    int port_;

    ThreadPool pool;

    HttpRequest parseRequest(const char* buffer);

    void handleClient(int client_fd);
};