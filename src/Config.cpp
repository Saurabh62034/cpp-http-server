#include "Config.h"

#include <fstream>
#include <sstream>

bool Config::load(const std::string& filename)
{
    std::ifstream file(filename);

    if(!file.is_open())
    {
        return false;
    }

    std::string line;

    while(std::getline(file, line))
    {
        std::stringstream ss(line);

        std::string key;
        std::string value;

        if(std::getline(ss, key, '=') &&
           std::getline(ss, value))
        {
            if(key == "PORT")
            {
                port = std::stoi(value);
            }
            else if(key == "THREADS")
            {
                threads = std::stoi(value);
            }
        }
    }

    return true;
}