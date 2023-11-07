#include <arpa/inet.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <map>
#include <fstream>
#include <sstream>

class Clint {
    public:
        std::string     request;
        std::string     response;
        std::ifstream   inputFile;
        std::string     buf_inputFile;
};

int main() {
    char            buf[2000];
    int             fdSocket;
    int             fdClint;
    int             max_socket;
    fd_set          readSet;
    fd_set          writeSet;
    fd_set          tmp_readSet;
    fd_set          tmp_writeSet;
    std::map<int, Clint> server;
    std::map<int, Clint>::iterator it;

    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);
    struct sockaddr_in host_addr;
    struct sockaddr_in Clint_addr;
    int host_addrlen = sizeof(host_addr);
    int clint_addrlen = sizeof(host_addr);

              //        APV4      TCP
    fdSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (fdSocket == -1) {
        perror ("socket:");
        return (1);
    }
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(8080);
    host_addr.sin_addr.s_addr = inet_addr("127.0.0.2");
    host_addrlen = sizeof(host_addr);

    if (bind(fdSocket, (struct sockaddr *)&host_addr, host_addrlen) != 0) {
        perror ("bind");
        return (1);
    }

    if (listen(fdSocket, 100) != 0) {
        perror ("listen");
        return (1);
    }
    while(true) {
        tmp_readSet = readSet;
        tmp_writeSet = writeSet;
        int answer =  select(max_socket + 1, &tmp_readSet, &tmp_writeSet, 0, 0);
        if (answer == -1) {
            std::cerr << "error: select" << "\n";
            exit (1);
        }
        else if (answer) {
            memset(&host_addr, 0, sizeof(host_addr));
            host_addrlen = 0;
            fdClint = accept(fdSocket, (struct sockaddr*)&Clint_addr, (socklen_t *)&clint_addrlen);  // non block
            if (fdClint == -1) {
                perror("accept");
                exit (1);
            }
            std::cout << "========================> accept clint: " << fdClint << "\n";
            server[fdClint];
            for (it = server.begin(); it != server.end(); ++it) {
                const int   &fdClint = it->first;
                Clint       &ClintObj = it->second;

                if (FD_ISSET(fdClint, &tmp_readSet)) {
                }
                else if (FD_ISSET(fdClint, &tmp_writeSet)) {
                    if (!ClintObj.inputFile.is_open()) {
                        ClintObj.inputFile.open("video.mp4");
                        if (!ClintObj.inputFile.is_open()) {
                            std::cerr << "error: open file\n";
                            exit (1);
                        }
                        ClintObj.response = "HTTP/1.1 200 OK\r\n";
                        ClintObj.response += "Content-Tipe: video/mp4\r\n";
                        ClintObj.response += "Connection: Keep-Alive\r\n";
                        ClintObj.response += "Content-Length: \r\n";
                        ClintObj.inputFile.seekg(0, std::ios::end);
                        int content_len = ClintObj.inputFile.tellg();
                        ClintObj.inputFile.seekg(0, std::ios::beg);
                        std::stringstream ss;
                        ss << content_len;
                        ClintObj.response += ss.str();
                        ClintObj.response += "\r\n\r\n";
                    }
                    else {
                        char buf[4000];
                        ClintObj.inputFile.read(buf, 4000);
                        buf[ClintObj.inputFile.gcount()] = '\0';
                        ClintObj.response.append(buf);
                        if (ClintObj.inputFile.gcount() < 4000) {
                            write(fdClint, ClintObj.response.c_str(), ClintObj.response.size());
                            FD_CLR(fdClint, &tmp_readSet);
                            FD_SET(fdClint, &tmp_writeSet);
                            server.erase(fdClint);
                        }
                    }
                }
                else {}
            }
        }
        else {
            std::cerr << "time out\n";
            exit (1);
        }
        
    }
}



