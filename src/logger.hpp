#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include <mutex>
#include <string>

namespace logger{
extern std::mutex outputLock;
const std::string color_code[5] = {"\033[32m", "\033[33m", "\033[34m", "\033[31m", "\033[35m"};
const std::string color_reset = "\033[0m";
void log_info(std::string msg, int target_no = -1);
void log_error(std::string msg, int target_no = -1);
} //logger

void quit(std::string msg);
#endif