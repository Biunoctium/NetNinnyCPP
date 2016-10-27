#ifndef NETNINNYCPP_CLIENTSIDE_H
#define NETNINNYCPP_CLIENTSIDE_H

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <stdlib.h>
#include "proxy.h"

class ClientSide{
public:
    int socket_client;

    ClientSide(int socket);
    //std::vector<char> readDataMax(size_t maxRead);
};

#endif //NETNINNYCPP_CLIENTSIDE_H
