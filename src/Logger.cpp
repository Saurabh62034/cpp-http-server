#include "Logger.h"

#include <iostream>

void Logger::info(const std::string& message)
{
    std::cout
        << "[INFO] "
        << message
        << '\n';
}

void Logger::error(const std::string& message)
{
    std::cerr
        << "[ERROR] "
        << message
        << '\n';
}