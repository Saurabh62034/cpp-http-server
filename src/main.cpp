#include "HttpServer.h"
#include "Config.h"


int main()
{
    Config config;

    config.load("config/server.conf");

    HttpServer server(
        config.port,
        config.threads
    );

    server.start();

    return 0;
}