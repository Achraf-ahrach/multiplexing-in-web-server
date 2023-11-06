#include <arpa/inet.h>
#include <iostream>
#include <errno.h>
// #include <string.h>
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
    int             max_socket;
    fd_set          readSet;
    fd_set          writeSet;
    fd_set          tmp_readSet;
    fd_set          tmp_writeSet;
    std::map<int, Clint> obj_clint;

    struct sockaddr_in host_addr;
              //        APV4      TCP
    if (fdSocket = socket(AF_INET, SOCK_STREAM, 0) == -1) {
        perror ("socket:");
        return (1);
    }
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = 8080;
    host_addr.sin_addr.s_addr = inet_addr("127.0.0.2");
    int host_addrlen = sizeof(host_addr);

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
        int fdClint = accept(fdSocket, (struct sockaddr *)&host_addr, (socklen_t *)&host_addrlen);
        if (fdClint < 0) {
            perror ("accept");
            return (1);
        }
        obj_clint[fdClint];
        max_socket = (--obj_clint.end())->first;
        std::cout << "==================> accept clint " << fdClint << "\n";

        int selectAnswer = select(max_socket, &tmp_readSet, &tmp_writeSet, 0 , 0);
        if (selectAnswer == -1) {
            perror ("select");
            return (1);
        }
        else if (!selectAnswer) {
            std::cerr << "select: timeout\n";
            return (1);
        }
        else {
            if (FD_ISSET(fdClint, &tmp_readSet)) {
                int len_read = read(fdClint, buf, 2000);
                if (len_read < 0) {
                    perror ("read:");
                    return (1);
                }
                buf[len_read] = '\0';
                obj_clint[fdClint].request.append(buf);
                if (len_read < 2000) {
                    FD_CLR(fdClint, &readSet);
                    FD_SET(fdClint, &writeSet);
                    std::cout << "=====================================================\n" << obj_clint[fdClint].request << "\n\n";
                }
            }
            else if (FD_ISSET(fdClint, &tmp_writeSet)) {
                if (!obj_clint[fdClint].inputFile.is_open()) {
                    obj_clint[fdClint].inputFile.open("video.mp4");
                    obj_clint[fdClint].response = "HTTP/1.0 200 OK\r\n";
                    obj_clint[fdClint].response += "Content-Tipe: video/mp4\r\n";
                    obj_clint[fdClint].response += "Content-Length: ";
                    obj_clint[fdClint].inputFile.seekg(0, std::ios::end);
                    int content_len = obj_clint[fdClint].inputFile.tellg();
                    obj_clint[fdClint].inputFile.seekg(0, std::ios::beg);
                    std::stringstream ss;
                    ss << content_len;
                    obj_clint[fdClint].response += ss.str();
                    obj_clint[fdClint].response += "\r\n\r\n";
                }
                else {
                    char buf[4000];
                    obj_clint[fdClint].inputFile.read(buf, 4000);
                    if (!obj_clint[fdClint].inputFile) {
                        std::cerr << "Error: read input file\n";
                        return (1);
                    }
                    buf[obj_clint[fdClint].inputFile.gcount()] = '\0';
                    obj_clint[fdClint].response.append(buf);
                    if (obj_clint[fdClint].inputFile.gcount() < 4000) {
                        write(fdClint, obj_clint[fdClint].response.c_str(), obj_clint[fdClint].response.size());
                        FD_CLR(fdClint, &writeSet);
                        FD_SET(fdClint, &readSet);
                    }
                }
            }
        }
    }
}