#pragma once

#include <string>

class UrlDecoder
{
public:
    static std::string decode(const std::string& encoded);
    static std::string decodeQueryComponent(const std::string& encoded);
    
};