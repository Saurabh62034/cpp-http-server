#pragma once
#include<iostream>
#include <string>


class SaveData{
    public:
    std::string body_message=""; 
    SaveData(std::string body_message){
        this->body_message = body_message;
    }

    void Store(){
        std::cout<<"Ready store the data"<<std::endl;
    }

};

