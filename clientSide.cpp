#include <iostream>
#include <stdlib.h>
#include "clientSide.h"

using namespace std;

ClientSide::ClientSide(int socket) : socket_client{socket}
{

}

/*vector<char> ClientSide::readDataMax(size_t maxRead){
    vector<char> buffer;
    char *buf_array = new char[maxRead];
    ssize_t bytes_read = 0;
    bytes_read = recv(socket_client, buf_array, maxRead, 0);
    if(bytes_read == -1){
        perror("recv error");
        exit(EXIT_FAILURE);
    }

    buffer.assign(buf_array, buf_array+bytes_read);
    cout << "buffer size = " << buffer.size() << " // bytes read = " << bytes_read << endl;
    delete[] buf_array;
    return buffer;
}*/
