#pragma once
#include<iostream>
#include <string>
#include <fstream>


class SaveData{
    public:
    std::unordered_map<std::string, std::string> body_message; 
    SaveData(std::unordered_map<std::string, std::string> body_message){
        this->body_message = body_message;
    }

    void Store(){
        std::ofstream myfile("../public/data-folder/userdata.txt", std::ios::app);
        if(!myfile.is_open()){
            return;
        }
        myfile<<"{\n";
        for(auto &p:body_message){
            myfile<<p.first<<": "<<p.second<<"\n";
        }
        myfile<<"}\n";
        myfile.close();

    }

};

