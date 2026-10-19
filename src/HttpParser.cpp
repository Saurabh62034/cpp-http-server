#include "HttpParser.h"
#include "UrlDecoder.h"

#include <cctype>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace
{
    std::string trim(const std::string& value)
    {
        const std::size_t start =
            value.find_first_not_of(" \t");

        if (start == std::string::npos)
        {
            return "";
        }

        const std::size_t end =
            value.find_last_not_of(" \t");

        return value.substr(start, end - start + 1);
    }

    bool isValidHeaderName(const std::string& name)
    {
        if (name.empty())
        {
            return false;
        }

        for (unsigned char c : name)
        {
            if (!std::isalnum(c) &&
                c != '!' && c != '#' && c != '$' &&
                c != '%' && c != '&' && c != '\'' &&
                c != '*' && c != '+' && c != '-' &&
                c != '.' && c != '^' && c != '_' &&
                c != '`' && c != '|' && c != '~')
            {
                return false;
            }
        }

        return true;
    }
}

ParseResult HttpParser::parse(
    const std::string& input) const
{
    HttpRequest request;

    // Step 1: Parse the request line.
    ParseStatus status =
        parseRequestLine(input, request);

    if (status != ParseStatus::SUCCESS)
    {
        return {status, std::move(request), 0};
    }

    // Step 2: Find the end of the complete header section.
    //
    // HTTP headers end with CRLF CRLF.
    // Until this delimiter arrives, the request is incomplete.

    const std::size_t headerEnd =
        input.find("\r\n\r\n");

    if (headerEnd == std::string::npos)
    {
        if (input.size() > MAX_HEADER_SIZE)
        {
            return {
                ParseStatus::BAD_REQUEST,
                std::move(request),
                0
            };
        }

        return {
            ParseStatus::NEED_MORE_DATA,
            std::move(request),
            0
        };
    }

    const std::size_t bodyStart = headerEnd + 4;

    if (bodyStart > MAX_HEADER_SIZE)
    {
        return {
            ParseStatus::BAD_REQUEST,
            std::move(request),
            0
        };
    }

    // Step 3: Parse the complete header section.
    if (!parseHeaders(input, headerEnd, request))
    {
        return {
            ParseStatus::BAD_REQUEST,
            std::move(request),
            0
        };
    }

    // Step 4: Parse query parameters.
    parseQueryParams(request);

    // Step 5: Parse the body from already-buffered input.
    std::size_t bytesConsumed = 0;

    status = parseBody(
        input,
        bodyStart,
        request,
        bytesConsumed
    );

    if (status != ParseStatus::SUCCESS)
    {
        return {
            status,
            std::move(request),
            0
        };
    }

    return {
        ParseStatus::SUCCESS,
        std::move(request),
        bytesConsumed
    };
}

ParseStatus HttpParser::parseRequestLine(
    const std::string& data,
    HttpRequest& request) const
{
    if (data.empty())
    {
        return ParseStatus::NEED_MORE_DATA;
    }

    const std::size_t requestLineEnd =
        data.find("\r\n");

    if (requestLineEnd == std::string::npos)
    {
        if (data.size() > MAX_REQUEST_LINE_SIZE)
        {
            return ParseStatus::BAD_REQUEST;
        }

        return ParseStatus::NEED_MORE_DATA;
    }

    if (requestLineEnd > MAX_REQUEST_LINE_SIZE)
    {
        return ParseStatus::BAD_REQUEST;
    }

    const std::string requestLine =
        data.substr(0, requestLineEnd);

    std::stringstream ss(requestLine);

    if (!(ss >> request.method))
    {
        return ParseStatus::BAD_REQUEST;
    }

    if (!(ss >> request.path))
    {
        return ParseStatus::BAD_REQUEST;
    }

    if (!(ss >> request.version))
    {
        return ParseStatus::BAD_REQUEST;
    }

    std::string extra;

    if (ss >> extra)
    {
        return ParseStatus::BAD_REQUEST;
    }

    if (request.version != "HTTP/1.1" &&
        request.version != "HTTP/1.0")
    {
        return ParseStatus::HTTP_VERSION_NOT_SUPPORTED;
    }

    if (request.method != "GET" &&
        request.method != "POST")
    {
        return ParseStatus::METHOD_NOT_ALLOWED;
    }

    if (request.path.empty())
    {
        return ParseStatus::BAD_REQUEST;
    }

    // Extract the query string before decoding the path.
    const std::size_t queryPos =
        request.path.find('?');

    if (queryPos != std::string::npos)
    {
        request.query =
            request.path.substr(queryPos + 1);

        request.path =
            request.path.substr(0, queryPos);
    }

    if (request.path.empty())
    {
        return ParseStatus::BAD_REQUEST;
    }

    request.path = UrlDecoder::decode(request.path);

    return ParseStatus::SUCCESS;
}

bool HttpParser::parseHeaders(
    const std::string& data,
    std::size_t headerEnd,
    HttpRequest& request) const
{
    const std::size_t firstLineEnd =
        data.find("\r\n");

    if (firstLineEnd == std::string::npos ||
        firstLineEnd > headerEnd)
    {
        return false;
    }

    std::size_t position = firstLineEnd + 2;

    std::size_t headerCount = 0;

    while (position < headerEnd)
    {
        const std::size_t lineEnd =
            data.find("\r\n", position);

        if (lineEnd == std::string::npos ||
            lineEnd > headerEnd)
        {
            return false;
        }

        const std::string line =
            data.substr(position, lineEnd - position);

        if (line.empty())
        {
            return false;
        }

        // Header field names must be followed by a colon.
        const std::size_t colon =
            line.find(':');

        if (colon == std::string::npos)
        {
            return false;
        }

        const std::string key =
            line.substr(0, colon);

        if (!isValidHeaderName(key))
        {
            return false;
        }

        const std::string value =
            trim(line.substr(colon + 1));

        // Preserve the existing duplicate Content-Length policy
        // for this step. Case-insensitive framing validation
        // will be handled separately.
        if (key == "Content-Length" &&
            request.headers.find("Content-Length") !=
                request.headers.end())
        {
            return false;
        }

        ++headerCount;

        if (headerCount > 100)
        {
            return false;
        }

        request.headers[key] = value;

        position = lineEnd + 2;
    }

    return true;
}

void HttpParser::parseQueryParams(
    HttpRequest& request) const
{
    std::string query = request.query;

    while (!query.empty())
    {
        const std::size_t ampPos =
            query.find('&');

        std::string params;

        if (ampPos == std::string::npos)
        {
            params = query;
            query.clear();
        }
        else
        {
            params = query.substr(0, ampPos);
            query = query.substr(ampPos + 1);
        }

        const std::size_t equalPos =
            params.find('=');

        std::string key;
        std::string value;

        if (equalPos == std::string::npos)
        {
            key = params;
            value = "";
        }
        else
        {
            key = params.substr(0, equalPos);
            value = params.substr(equalPos + 1);
        }

        key = UrlDecoder::decodeQueryComponent(key);
        value = UrlDecoder::decodeQueryComponent(value);

        request.queryParams[key] = value;
    }
}

ParseStatus HttpParser::parseBody(
    const std::string& input,
    std::size_t bodyStart,
    HttpRequest& request,
    std::size_t& bytesConsumed) const
{
    const auto it =
        request.headers.find("Content-Length");

    // Preserve the current server's POST policy.
    if (it == request.headers.end())
    {
        if (request.method == "POST")
        {
            return ParseStatus::LENGTH_REQUIRED;
        }

        request.body.clear();
        bytesConsumed = bodyStart;

        return ParseStatus::SUCCESS;
    }

    // Content-Length must contain decimal digits only.
    if (it->second.empty())
    {
        return ParseStatus::BAD_REQUEST;
    }

    for (unsigned char c : it->second)
    {
        if (!std::isdigit(c))
        {
            return ParseStatus::BAD_REQUEST;
        }
    }

    unsigned long long parsedLength = 0;
    std::size_t parsedPosition = 0;

    try
    {
        parsedLength = std::stoull(
            it->second,
            &parsedPosition
        );
    }
    catch (const std::exception&)
    {
        return ParseStatus::BAD_REQUEST;
    }

    if (parsedPosition != it->second.size())
    {
        return ParseStatus::BAD_REQUEST;
    }

    if (parsedLength > MAX_BODY_SIZE)
    {
        return ParseStatus::PAYLOAD_TOO_LARGE;
    }

    if (parsedLength >
        std::numeric_limits<std::size_t>::max())
    {
        return ParseStatus::BAD_REQUEST;
    }

    const std::size_t bodyLength =
        static_cast<std::size_t>(parsedLength);

    // bodyStart is already known to be within input.
    // Subtraction avoids an overflowing bodyStart + bodyLength
    // comparison.
    if (input.size() - bodyStart < bodyLength)
    {
        return ParseStatus::NEED_MORE_DATA;
    }

    request.body.assign(
        input.data() + bodyStart,
        bodyLength
    );

    bytesConsumed = bodyStart + bodyLength;

    return ParseStatus::SUCCESS;
}