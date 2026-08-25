#pragma once

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Router.h"
#include "ThreadPool.h"
#include "HttpParser.h"
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
    HttpParser parser;
    void handleClient(int client_fd);
};