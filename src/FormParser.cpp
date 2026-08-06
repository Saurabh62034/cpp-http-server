#include <FormParser.h>

std::unordered_map<string,string> FormParser::parse(const string& body){
    unordered_map<string, string> form;
    string parse_body = body;
    while(!parse_body.empty()){
        size_t idx = parse_body.find("&");

        if(idx !=string::npos){
            string data = parse_body.substr(0,idx);
            
            size_t next_idx = data.find("=");

            string key = data.substr(0,next_idx);
            string val = data.substr(next_idx+1);

            form[key] = val;
            parse_body = parse_body.substr(idx+1);
        }
        else{
            size_t next_idx = parse_body.find("=");

            string key = parse_body.substr(0,next_idx);
            string val = parse_body.substr(next_idx+1);

            form[key] = val;
            break;
        }
    }


    return form;
}