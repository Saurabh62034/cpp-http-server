#pragma once

#include "HttpRequest.h"
#include "HttpResponse.h"

class Router
{
public:
    HttpResponse route(const HttpRequest& request);
};