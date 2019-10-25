#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <boost/program_options.hpp>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "logger.hpp"

namespace po = boost::program_options;

int server_socket_fd;
void sigint_handler(int sig)
{
    logger::log_error("\nShuting down server ...");
    shutdown(server_socket_fd, SHUT_RDWR);
    close(server_socket_fd);
    exit(0);
}

int setup(uint16_t port){
    int fd, enable = 1;
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)quit("Failed to creat socket!");
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if(bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0)quit("Failed to bind socket on port " + std::to_string(port) + "!");
    if(listen(fd, 10) < 0)quit("Failed to start listening for connection!");
    
    logger::log_info("Listing on port " + std::to_string(port) + " ...");
    
    server_socket_fd = fd;
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    return fd;
}

void print_help(po::options_description desc){
    std::cout<<"Usage: ./server [--help] port\n\n";
    std::cout<<"TCP Ping - Server - Made by Brian su (b08902107)\n\n";
    std::cout<<"Positional Argument\n  port - Port to listen on\n\n";
    std::cout<<"Optional Arguments\n";
    std::cout<<desc<<std::endl;
    exit(0);
}

int main(int argc, char *argv[])
{
    uint16_t port;
    po::options_description desc, desc_all;
    desc.add_options()
    ("help,h", "Print help messages");
    desc_all.add(desc);
    desc_all.add_options()
    ("port", po::value<uint16_t>(&port)->required(), "Port to listen on");
    
    po::positional_options_description pos_desc;
    pos_desc.add("port", 1);

    po::variables_map vmap;
    try{
        po::store(po::command_line_parser(argc, argv).options(desc_all).positional(pos_desc).run(), vmap);
        if(vmap.count("help"))print_help(desc);
        po::notify(vmap);
    }catch(const std::exception &ex){
        logger::log_error("Failed to parse arguments: " + std::string(ex.what()) + "\n");
        print_help(desc);
    }
    
    logger::log_info("== TCP Ping - Server - Made by Brian su (b08902107) ==");
    int fd = setup(port);
    
    while(1)
    {
        sockaddr_in conn_addr;
        socklen_t conn_addr_len = sizeof(conn_addr);
        int conn_fd = accept(fd, (struct sockaddr*)&conn_addr, &conn_addr_len);
        if(conn_fd < 0)
        {
            logger::log_error("Failed to establish new connection!");
            continue;
        }
        char conn_ip[INET_ADDRSTRLEN + 2];
        memset(conn_ip, 0, sizeof(conn_ip));
        inet_ntop(AF_INET, &conn_addr.sin_addr, conn_ip, INET_ADDRSTRLEN);
        logger::log_info("recv from " + std::string(conn_ip) + ":" + std::to_string(ntohs(conn_addr.sin_port)));
        shutdown(conn_fd, SHUT_RDWR);
        close(conn_fd);
    }
}