cmake_minimum_required(VERSION 3.10)

project(cpp_http_server)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(
    http_server

    src/main.cpp
    src/HttpServer.cpp
    src/HttpResponse.cpp
    src/Router.cpp
    src/ThreadPool.cpp
    src/Logger.cpp
    src/Config.cpp
)