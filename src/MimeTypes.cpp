#include "MimeTypes.h"

#include <unordered_map>
#include <algorithm>
#include <cctype>

std::string MimeTypes::get(const std::string& path)
{
    size_t dot_pos = path.find_last_of('.');

    if(dot_pos == std::string::npos)
    {
        return "application/octet-stream";
    }

    std::string extension = path.substr(dot_pos + 1);

    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](unsigned char c)
        {
            return std::tolower(c);
        }
    );

    static const std::unordered_map<std::string, std::string> mimeTypes =
    {
        {"html", "text/html"},
        {"htm",  "text/html"},

        {"css",  "text/css"},

        {"js",   "application/javascript"},
        {"mjs",  "application/javascript"},

        {"json", "application/json"},

        {"txt",  "text/plain"},

        {"xml",  "application/xml"},

        {"png",  "image/png"},
        {"jpg",  "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif",  "image/gif"},
        {"svg",  "image/svg+xml"},
        {"webp", "image/webp"},
        {"ico",  "image/x-icon"},

        {"pdf",  "application/pdf"},

        {"zip",  "application/zip"},

        {"mp3",  "audio/mpeg"},
        {"wav",  "audio/wav"},

        {"mp4",  "video/mp4"},
        {"webm", "video/webm"}
    };

    auto it = mimeTypes.find(extension);

    if(it != mimeTypes.end())
    {
        return it->second;
    }

    return "application/octet-stream";
}