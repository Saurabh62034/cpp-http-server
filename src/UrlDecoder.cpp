#include "UrlDecoder.h"

#include <sstream>
#include <iomanip>
#include <cctype>

std::string UrlDecoder::decode(const std::string& encoded)
{
    std::string decoded;

    for (size_t i = 0; i < encoded.size(); ++i)
    {
        if (encoded[i] == '%' && i + 2 < encoded.size())
        {
            char high = encoded[i + 1];
            char low  = encoded[i + 2];

            if (std::isxdigit(static_cast<unsigned char>(high)) &&
                std::isxdigit(static_cast<unsigned char>(low)))
            {
                std::string hex = encoded.substr(i + 1, 2);

                char decodedChar =
                    static_cast<char>(std::stoi(hex, nullptr, 16));

                decoded += decodedChar;
                i += 2;
                continue;
            }
        }
            decoded += encoded[i];
    }

    return decoded;
}

std::string UrlDecoder::decodeQueryComponent(const std::string& encoded)
{
    std::string decoded;

    for(size_t i = 0; i < encoded.size(); ++i)
    {
        if(encoded[i] == '+')
        {
            decoded += ' ';
        }
        else if(encoded[i] == '%' && i + 2 < encoded.size())
        {
            char high = encoded[i + 1];
            char low  = encoded[i + 2];

            if(std::isxdigit(static_cast<unsigned char>(high)) &&
               std::isxdigit(static_cast<unsigned char>(low)))
            {
                std::string hex = encoded.substr(i + 1, 2);

                char decodedChar =
                    static_cast<char>(std::stoi(hex, nullptr, 16));

                decoded += decodedChar;
                i += 2;
            }
            else
            {
                decoded += encoded[i];
            }
        }
        else
        {
            decoded += encoded[i];
        }
    }

    return decoded;
}