//
// Created by biu on 14/10/16.
//

#ifndef NETNINNYCPP_SERVERSIDE_H
#define NETNINNYCPP_SERVERSIDE_H


#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <iostream>
#include <cstring>
#include <stdlib.h>
#include "proxy.h"
#include "clientSide.h"

class ServerSide{
public:
    int socket_server;
    std::string hostname;
    std::string port;
    ClientSide &client;

    ServerSide(std::string host, std::string port, ClientSide &client);
    int openConnection();
};

#endif //NETNINNYCPP_SERVERSIDE_H
