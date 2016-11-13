#include <stdlib.h>
#include "serverSide.h"
#include "proxy.h"

using namespace std;

ServerSide::ServerSide(string host, string port, ClientSide &client) : socket_server{-1},
                                                                       hostname{host},
                                                                       port{port},
                                                                       client{client}

{

}

/**
 * opens the connection to the hostname
 * @param hostname the host to connect to
 * @param port the port to use
 * @return -1 if failure, else 0
 */
int ServerSide::openConnection(){
    //cout << "hello open connection" << endl;

    struct addrinfo *rp;

    struct addrinfo hints;
    struct addrinfo *servinfo;  //will point to the results

    int status;

    /*Setting up the structures we'll use later (addrinfo hints + addrinfo *servinfo */
    memset(&hints, 0, sizeof hints);             //make sure the struct (addrinfo hints) is empty
    hints.ai_family = AF_UNSPEC;                //ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM;            //TCP stream socket
    hints.ai_flags = 0;                         // assign the address of my local host to the socket structures

    status = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &servinfo);
    // The function returns 0 upon success and negative if it fails
    if(status != 0){
        fprintf(stderr, "server : getaddrinfo() error : %s\n", gai_strerror(status));
        return -1;
    } // From now, servinfo points to a linked list of 1 or more struct addrinfos
    //we bind with the first possible result
    for(rp = servinfo; rp != NULL; rp = rp->ai_next){
        socket_server = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(socket_server == -1){
            perror("server : socket");
            continue;
        }

        if(connect(socket_server, rp->ai_addr, rp->ai_addrlen) != -1){
            break;
        }
        close(socket_server); //at this point no break <=> was not successful
    }

    freeaddrinfo(servinfo); //free the linked-list of struct addrinfo(s)
    if(rp == NULL){
        perror("server : no connection was successful");
        return -1;
    }

    return 0;
}

