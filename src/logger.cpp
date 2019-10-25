#include <cstdlib>
#include <iostream>
#include <mutex>

#include "logger.hpp"

std::mutex logger::outputLock;

void logger::log_info(std::string msg, int target_no){
    std::lock_guard<std::mutex> mLock(outputLock);
    std::cout<<"[*]";
    if(target_no != -1)std::cout<<color_code[(target_no - 1) % 5] << "[target " << target_no << "]" << color_reset;
    std::cout<<" ";
    std::cout<<msg<<std::endl;
}

void logger::log_error(std::string msg, int target_no){
    std::lock_guard<std::mutex> mLock(outputLock);
    std::cout<<"[!]";
    if(target_no != -1)std::cout<<color_code[(target_no - 1) % 5] << "[target " << target_no << "]" << color_reset;
    std::cout<<" ";
    std::cout<<msg<<std::endl;
}

void quit(std::string msg){
    logger::log_error(msg);
    exit(1);
}