#pragma once
#include <string>
#include<unordered_map>
using namespace std;

class FormParser{
    public:

    unordered_map<string, string> parse(const string &body);
};