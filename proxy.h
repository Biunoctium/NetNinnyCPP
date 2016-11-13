//
// Created by biu on 14/10/16.
//

#ifndef NETNINNYCPP_PROXY_H
#define NETNINNYCPP_PROXY_H

#include <vector>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <iostream>
#include <regex>

#define MAX_BUFFER_SIZE 8192 //max size of a request


class Proxy{
public:
    Proxy(std::string port);

    int sockfd; //socket descriptors

    void acceptLoop();
    bool parseForHostname(std::string request, std::string &hostname, std::string &portnumber);

};

#endif //NETNINNYCPP_PROXY_H
