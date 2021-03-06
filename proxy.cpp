#include "proxy.h"
#include "serverSide.h"
#include "clientSide.h"
#include "filter.h"

using namespace std;


void keepAliveToClose(std::vector<char> &vect_request);
std::vector<char> readDataMax(int socket, size_t maxRead);
int sendData(int socket, std::vector<char> buffer);
vector<char> editHeader(bool url);
bool checkGZIP(vector<char> buffer);
bool check_Content_Type_Text(vector<char> buffer);

int main(int argc, char **argv){
    /**
     * CHECKING ERROR ON CALL, requirement #7
     */
    if(argc != 2){
        perror("Bad call, one argument needed");

    }
    string str_argv = argv[1];
    bool var= true;
    size_t cpt = 0;
    while((cpt != str_argv.length()) && var){
        if(!isdigit(str_argv[cpt])){
            var = false;
        }
        cpt++;
    }
    if(!var){
        perror("Bad port number");
        exit(EXIT_FAILURE);
    }

    /**
     * RUNNING PROXY NOW
     */
    //requirement #7, choosing port
    Proxy proxy(argv[1]);
    //now proxy is listening
    cout << "launching connection 2" << endl;
    proxy.acceptLoop();



}

/**
 * create the socket and store it in the proxy
 * @param port
 */
Proxy::Proxy(string port)
{
    struct addrinfo *rp;

    struct addrinfo hints;
    struct addrinfo *servinfo;  //will point to the results

    int backlog = 10;
    int status;
    int yes = 1;

    /*Setting up the structures we'll use later (addrinfo hints + addrinfo *servinfo */
    memset(&hints, 0, sizeof hints);             //make sure the struct (addrinfo hints) is empty
    hints.ai_family = AF_UNSPEC;                //ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM;            //TCP stream socket
    hints.ai_flags = AI_PASSIVE;                // assign the address of my local host to the socket structures

    status = getaddrinfo(NULL, port.c_str(), &hints, &servinfo);
    // The function returns 0 upon success and negative if it fails
    if(status != 0){
        fprintf(stderr, "getaddrinfo() error : %s\n", gai_strerror(status));
        exit(1);
    } // From now, servinfo points to a linked list of 1 or more struct addrinfos
    //we bind with the first possible result
    for(rp = servinfo; rp != NULL; rp = rp->ai_next){
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sockfd == -1){
            perror("socket");
            continue;
        }
        //kill the "already in use" message
        if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        if(bind(sockfd, rp->ai_addr, rp->ai_addrlen) == -1){
            perror("bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo); //free the linked-list of struct addrinfo(s)
    if(rp == NULL){
        perror("failed to bind");
        exit(EXIT_FAILURE);
    }

    //now we can start listening for connections
    if(listen(sockfd, backlog) == -1){
        perror("listen");
        exit(EXIT_FAILURE);
    }
}

/**
 * main accept loop for incoming connections
 * forks at each connection, creating a new client/server each time
 */
void Proxy::acceptLoop(){
    //initiate filter
    vector<string> words{"SpongeBob", "Britney Spears", "Paris Hilton", "Norrköping"};
    Filter filter(words);
    //loop to accept and fork at each connection asked by client
    struct sockaddr_storage their_addr;
    while(true) {
        int client_soc;
        socklen_t addr_size = sizeof their_addr;
        client_soc = accept(sockfd, (struct sockaddr *) &their_addr, &addr_size);
        if(client_soc== -1){
            perror("accept");
        }

        if(!fork()) { //forking every incoming connection
            //create a clientSide for each connection
            ClientSide clientSide(client_soc);
            bool first = true;
            string host;
            string port;
            vector<char> buffer;
            //first, find the hostname
            while (first) {
                //read data the client sends
                vector<char> tmp_buff = readDataMax(clientSide.socket_client, MAX_BUFFER_SIZE);
                copy(tmp_buff.begin(), tmp_buff.end(), back_inserter(buffer));

                if (!parseForHostname(buffer.data(), host, port)) {
                    cout << "Couldn't find hostname" << endl;
                } else {
                    first = false;
                }
            }
            //now that we have the hostname, we open the connection
            ServerSide serverSide(host, "80", clientSide);
            if(serverSide.openConnection() == -1){
                cout << "Could not open connection" << endl;
                close(serverSide.socket_server);
                close(clientSide.socket_client);
                exit(-1);
            }


            if(!fork()){ //different thread to send results to client
                vector<char> tmp_buff;
                vector<char> total_buff;
                bool text = false;
                bool gzip = true;
                bool go_filter = false;

                do{
                    tmp_buff = readDataMax(serverSide.socket_server, MAX_BUFFER_SIZE);

                    /**
                     * REQUIREMENT #8 CHECK FOR COMPRESSED CONTENT
                     */
                    if(gzip && !tmp_buff.empty()){
                        gzip = checkGZIP(tmp_buff);
                    }
                    /**
                     * REQUIREMENT #5 no limit on size
                     */
                    if(!text && !tmp_buff.empty()) {
                        text = check_Content_Type_Text(tmp_buff);
                    }

                    if(text && !gzip && !tmp_buff.empty()){
                        go_filter = true;
                        copy(tmp_buff.begin(), tmp_buff.end(), back_inserter(total_buff));
                    }else{
                        //send on the fly
                        sendData(clientSide.socket_client, tmp_buff);
                    }

                }while(tmp_buff.size());

                text = false;
                gzip = true;
                tmp_buff.clear();

                /**
                 * REQUIREMENT #4 handling bad content
                 */

                if(go_filter){
                    if(filter.process(total_buff.data())){
                        vector<char> new_req{};
                        new_req = editHeader(false);
                        sendData(clientSide.socket_client, new_req);
                        tmp_buff.clear();
                        total_buff.clear();
                        do {
                            tmp_buff = readDataMax(serverSide.socket_server, MAX_BUFFER_SIZE);
                            copy(tmp_buff.begin(), tmp_buff.end(), back_inserter(total_buff));
                        } while (tmp_buff.size());
                    }
                    sendData(clientSide.socket_client, total_buff);
                }

                close(serverSide.socket_server);
                close(clientSide.socket_client);
                exit(0);
            }
            //sending the request
            /**
             * REQUIREMENT #3 handling bad URLs
             */
            if (filter.process(buffer.data())) {
                // Edit the header to redirect to error URL page
                vector<char> new_req{};
                new_req = editHeader(true);
                sendData(clientSide.socket_client, new_req);
                vector<char> tmp_buff{};
                buffer.clear();
                do{
                    tmp_buff = readDataMax(serverSide.socket_server, MAX_BUFFER_SIZE);
                    copy(tmp_buff.begin(), tmp_buff.end(), back_inserter(buffer));
                }while(tmp_buff.size());

                //cout << "buff = " << buffer.data() << endl;
            }

            keepAliveToClose(buffer);
            sendData(serverSide.socket_server, buffer);


            close(serverSide.socket_server);
            close(clientSide.socket_client);
            exit(0);
        }
        close(client_soc); //parent doesn't need this fd
    }
}


/**
 * This function gets the hostname out of the request, stores it in parameter hostname
 * @param request to parse, hostname to save
 * @return true if hostname found, else false
 */
bool Proxy::parseForHostname(string request, string &hostname, string &portnumber){
    size_t pos_start = 0;
    size_t pos_end = 0;
    size_t pos_col = 0;
    string start_pattern = "\r\nHost: ";
    string end_pattern = "\r\n";

    pos_start = request.find(start_pattern);
    //find() returns npos value if not found
    if(pos_start != string::npos){
        pos_start += start_pattern.length();
        pos_end = request.find(end_pattern, pos_start);
        if(pos_end != string::npos){
            hostname = request.substr(pos_start, pos_end - pos_start);
            //Now we check if there is a port number attached to the hostname
            pos_col = hostname.find(':');
            if(pos_col != string::npos){ //there is a port number, extract it
                portnumber = hostname.substr(pos_col + 1, hostname.length());
                hostname = hostname.substr(0, pos_col);
            }else{
                portnumber = "80"; //default port number
            }
            return true;
        }
    }
    return false;
}

/**
 * (requirement #2)
 * reading data from the mentionned socket, returning this data
 * @param socket
 * @param maxRead the number of bytes to read
 * @return the data read
 */
vector<char> readDataMax(int socket, size_t maxRead){
    vector<char> buffer;
    char *buf_array = new char[maxRead];
    ssize_t bytes_read = 0;
    bytes_read = recv(socket, buf_array, maxRead, 0);
    if(bytes_read == -1){
        perror("recv error");
        exit(EXIT_FAILURE);
    }

    buffer.assign(buf_array, buf_array+bytes_read);
    delete[] buf_array;
    return buffer;
}

/**
 * (requirement #2)
 * Sending data to the mentionned socket
 * @param socket
 * @param buffer
 * @return -1 if any problem
 */
int sendData(int socket, vector<char> buffer){
    ssize_t bytes_sent = 0;
    size_t total_sent = 0;
    while(total_sent != buffer.size()){
        bytes_sent = send(socket, buffer.data()+total_sent, buffer.size()-total_sent, 0);
        if (bytes_sent == -1) {
            perror("send error");
            return -1;
        }
        total_sent += bytes_sent;
    }
    return 0;
}

/**
 * change the parameter of connection "keep alive" to connection "close"
 * @param request to analyse
 */
void keepAliveToClose(vector<char> &vect_request){
    string request = vect_request.data();
    size_t pos_start = 0;
    size_t pos_start2 = 0;
    string pattern = "\r\nConnection: ";
    string pattern2 = "\r\n";
    vector<char> vect1;
    string word; //the part to change
    vector<char> vect2;
    vector<char> vect3;
    pos_start = request.find(pattern);
    if(pos_start != string::npos) {
        vect1.assign(vect_request.begin(), vect_request.begin()+pos_start+pattern.length());
        pos_start2 = request.find(pattern2, pos_start);
        if(pos_start2 != string::npos) {
            word = "close";
            vect2.assign(word.begin(), word.end());
            vect3.assign(vect_request.begin()+pos_start2, vect_request.end());
            vect_request.assign(vect1.begin(), vect1.end());
            vect_request.insert(vect_request.end(), vect2.begin(), vect2.end());
            vect_request.insert(vect_request.end(), vect3.begin(), vect3.end());
        }
    }
}

/**
 * (requirement #3 + #4)
 * edit the header for redirection
 * @param url if we find a bad word in the URL ==> true ; else it's in the content : call with false
 * @param buffer to analyze
 */
vector<char> editHeader(bool url){
    vector<char> new_req{};
    string data{};
    string new_URL;

    data.append("HTTP/1.1 302 Found\r\n");

    if(url){
        //bad url
        new_URL = "http://www.ida.liu.se/~TDTS04/labs/2011/ass2/error1.html";
    }else{
        //bad content
        new_URL = "http://www.ida.liu.se/~TDTS04/labs/2011/ass2/error2.html";
    }
    data.append("Location: " + new_URL + "\r\n\r\n");
    /*
    string new_host = "http://www.ida.liu.se";
    regex expr_GET{"(^GET )(.*)( )", regex_constants::icase};
    regex expr_host{"(^Host: )(.*)", regex_constants::icase};
    data = regex_replace(data, expr_GET, "$1" + new_URL + "$3");
    data = regex_replace(data, expr_host, "$1" + new_host);
    */
    copy(data.begin(), data.end(), back_inserter(new_req));

    return new_req;
}


/**
 * (requirement 8)
 * Check if the buffer is of type asked
 * @param type to compare
 * @param buffer to analyze
 * @return true if the buffer is of type asked, else false
 */

bool checkGZIP(vector<char> buffer){
    /*regex expr1{"^Content-Encoding: .*" + type + ".*$", regex_constants::icase};
    return (regex_match(buffer.data(), expr1));*/
    string buf = buffer.data();
    string pattern{"Content-Encoding: gzip"};
    return(buf.find(pattern) != string::npos);
}

/**
 * (requirement 5)
 * true : doesn't contain "content type" ==> ok for filtering OR has "content type" and "text" ==> filter
 * false : has "content type" but not text ==> no filter
 * */
bool check_Content_Type_Text(vector<char> buffer){
    //cout << buffer.data() << endl;

    string pattern1{"Content-Type: "};
    string pattern2{"Content-Type: text"};
    string buf = buffer.data();

    bool res;
    if(buf.find(pattern1) != string::npos){
        res = false;
        if(buf.find(pattern2) != string::npos){
            res = true;
        }
    }else{
        res = true;
    }
    return res;
}


