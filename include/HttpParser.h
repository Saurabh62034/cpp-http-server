#pragma once

#include "HttpRequest.h"

#include <cstddef>
#include <string>

enum class ParseStatus
{
    SUCCESS,
    NEED_MORE_DATA,
    BAD_REQUEST,
    METHOD_NOT_ALLOWED,
    HTTP_VERSION_NOT_SUPPORTED,
    PAYLOAD_TOO_LARGE,
    LENGTH_REQUIRED
};

constexpr std::size_t MAX_BODY_SIZE = 10 * 1024 * 1024;
constexpr std::size_t MAX_REQUEST_LINE_SIZE = 8 * 1024;
constexpr std::size_t MAX_HEADER_SIZE = 32 * 1024;

struct ParseResult
{
    ParseStatus status;
    HttpRequest request;
    std::size_t bytesConsumed = 0;
};

class HttpParser
{
public:
    ParseResult parse(const std::string& input) const;

private:
    ParseStatus parseRequestLine(
        const std::string& data,
        HttpRequest& request
    ) const;

    bool parseHeaders(
        const std::string& data,
        std::size_t headerEnd,
        HttpRequest& request
    ) const;

    void parseQueryParams(
        HttpRequest& request
    ) const;

    ParseStatus parseBody(
        const std::string& input,
        std::size_t bodyStart,
        HttpRequest& request,
        std::size_t& bytesConsumed
    ) const;
};