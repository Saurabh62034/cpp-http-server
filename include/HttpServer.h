#pragma once

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Router.h"
#include "ThreadPool.h"
#include <string.h>
using namespace std;

class HttpServer
{
public:
    HttpServer(int port, int threadCount);

    void start();

private:
    int port_;

    ThreadPool pool;

    HttpRequest parseRequest(const string& buffer);

    void handleClient(int client_fd);
};