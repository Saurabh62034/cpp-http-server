#pragma once

#include <string>

class Config
{
public:
    int port = 9090;
    int threads = 8;

    bool load(const std::string& filename);
};