#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <boost/program_options.hpp>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "logger.hpp"

namespace po = boost::program_options;

struct target{
    std::string host;
    uint16_t port;
    std::string ip;
    int ping_send, pong_recv;
    double rtt_tt;
    target() = default;
    target(std::string host, uint16_t port): host(host), port(port), ip(""), ping_send(0), pong_recv(0), rtt_tt(0) {}
    std::string to_string(bool uip = true){
        if(uip)return ip + ":" + std::to_string(port);
        else return host + ":" + std::to_string(port);
    }
};

std::string domain_to_ip(std::string s){
    addrinfo hints, *host_info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    int result = getaddrinfo(s.c_str(), NULL, &hints, &host_info);
    if(result != 0)return "";
    char host_ip[INET_ADDRSTRLEN + 2];
    memset(host_ip, 0, sizeof(host_ip));
    getnameinfo(host_info->ai_addr, host_info->ai_addrlen, host_ip, sizeof(host_ip), NULL, 0, NI_NUMERICHOST);
    return std::string(host_ip);
}

target cmd_to_target(std::string s){
    int p = -1;
    for(int i = 0;i < s.size();i += 1)if(s[i] == ':'){
        if(p != -1 || i == 0 || i + 1 == s.size())quit("Can not parse target '" + s + "'");
        p = i;
    }
    if(p == -1)quit("Can not parse target '" + s + "', maybe the port number is missing");
    std::string host = s.substr(0, p);
    uint16_t port;
    try{
        int _port = std::stoi(s.substr(p + 1));
        if(_port < 0 || _port > 65535)quit("Invalid port " + s.substr(p + 1) + " in target '" + s + "', shoulde be in [0, 65535]");
        port = _port;
    }catch(const std::invalid_argument& ia){
        quit("Can not parse port '" + s.substr(p + 1) + "' in target '" + s + "'");
    }
    return target(host, port);
}

void close_socket(int fd){
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

void pinger(target &tg, int target_id, int num, int tout, int twait){
    std::string domain = tg.host;
    std::string ip = domain_to_ip(domain);
    if(ip == "")return logger::log_error("Cannot resolve host: " + domain, target_id);
    else tg.ip = ip;
    logger::log_info("Start pinging " + tg.to_string(0) + " (" + ip + ")", target_id);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(tg.port);
    addr.sin_addr.s_addr = inet_addr(tg.ip.c_str());

    while(1)
    {
        int fd;
        if((fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)return logger::log_error("Failed to creat socket!", target_id);

        fd_set write_fd;
        FD_ZERO(&write_fd);
        FD_SET(fd, &write_fd);
        timeval timeout;
        timeout.tv_sec = (tout / 1000);
        timeout.tv_usec = (tout % 1000) * 1000;

        auto start = std::chrono::steady_clock::now();
        int con_ret = connect(fd, (struct sockaddr*) &addr, sizeof addr);
        tg.ping_send += 1;
        if(con_ret != 0 && errno != EINPROGRESS)
            return close_socket(fd), logger::log_error("Failed to connect to target '" + tg.to_string() + "'", target_id);
        int sel_ret = select(fd + 1, NULL, &write_fd, NULL, &timeout);
        auto end = std::chrono::steady_clock::now();
        
        if(sel_ret == -1)
            return close_socket(fd), logger::log_error("Error occurs when connecting to target '" + tg.to_string() + "'", target_id);
        
        double delta = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
        if(sel_ret == 0 || delta > double(tout))logger::log_info("timeout when connecting to " + tg.to_string(), target_id);
        else{
            char delta_str[20];
            memset(delta_str, 0, sizeof(delta_str));
            sprintf(delta_str, "%0.2f", delta);
            logger::log_info("recv from " + tg.to_string() + ", RTT = " + std::string(delta_str) + " msec", target_id);
            tg.pong_recv += 1;
            tg.rtt_tt += delta;
        }

        close_socket(fd);

        if(num == 1)break;
        else if(num > 1)num --;
        std::this_thread::sleep_for(std::chrono::milliseconds(twait));
    }
}

void print_help(po::options_description desc){
    std::cout<<"Usage: ./client [--help] [-n num] [-t timeout] [-w wait] target1 [target2 ...]\n\n";
    std::cout<<"TCP Ping - Client - Made by Po-Hsuan Su (b08902107)\n\n";
    std::cout<<"Positional Argument\n  target - Target(s) in `host:port` form, host can be in hostname or ip\n\n";
    std::cout<<"Optional Arguments\n";
    std::cout<<desc<<std::endl;
    exit(0);
}

std::vector<target> targets;

// also handles sigint
void finalize(int sig){
    for(target &t:targets){
        if(t.ping_send == 0)continue;
        std::cout<<"\n--- " + t.to_string(0) + " (" + t.ip + ") statistics ---"<<std::endl;
        std::cout<< t.ping_send << " pings sent, " << t.pong_recv << " pongs received, " << std::fixed 
            << std::setprecision(2) << (t.ping_send - t.pong_recv) / double(t.ping_send) * 100 << "% ping loss" <<std::endl;
        if(t.pong_recv > 0)std::cout<< "RTT average = " << std::fixed << std::setprecision(2) << t.rtt_tt / t.pong_recv << " msec" << std::endl;
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    int num, tout, twait;
    std::vector<std::string> target_strs;
    po::options_description desc, desc_all;
    desc.add_options()
    ("help,h", "Print help messages")
    ("num,n", po::value<int>(&num)->default_value(0), "Amount of packets to send to each target, 0 means keep sending messages until program close")
    ("timeout,t", po::value<int>(&tout)->default_value(1000), "Maximum millisecond(s) the client needs to wait for each response")
    ("wait,w", po::value<int>(&twait)->default_value(1000), "Millisecond(s) the client need to wait between each ping");
    desc_all.add(desc);
    desc_all.add_options()
    ("target", po::value<std::vector<std::string>>(&target_strs), "Target(s) in `host:port` form, host can be in hostname or ip");

    po::positional_options_description pos_desc;
    pos_desc.add("target", -1);

    po::variables_map vmap;
    try{
        po::store(po::command_line_parser(argc, argv).options(desc_all).positional(pos_desc).run(), vmap);
        if(vmap.count("help"))print_help(desc);
        po::notify(vmap);
        if(tout < 0)throw std::invalid_argument("timeout should not be negative");
        if(num < 0)throw std::invalid_argument("num should not be negative");
        if(twait < 0)throw std::invalid_argument("waiting time should not be negative");
        if(target_strs.size() == 0)throw std::invalid_argument("there should be at least one target");
    }catch(const std::exception &ex){
        logger::log_error("Failed to parse arguments: " + std::string(ex.what()) + "\n");
        print_help(desc);
    }

    for(auto &s:target_strs)targets.push_back(cmd_to_target(s));

    logger::log_info("== TCP Ping - Client - Made by Po-Hsuan Su (b08902107) ==");

    std::vector<std::thread> thread_v;

    // Start all thread
    for(int i = 0;i < targets.size();i ++)thread_v.emplace_back(pinger, std::ref(targets[i]), i + 1, num, tout, twait);
    
    struct sigaction sa;
    sa.sa_handler = finalize;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    // Wail all thread to finish
    for(auto &t:thread_v)t.join();

    finalize(0);
}