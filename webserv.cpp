#include <arpa/inet.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <signal.h>
#include <map>
#include <fstream>
#include <sstream>

class Clint {
    public:
    std::string buff;
        int             flage;
        int request_finish;
        int response_finish;
        std::string     request;
        std::string     response;
        std::ifstream   inputFile;
        std::string     buf_inputFile;
    Clint() {
        flage = 0;
    }
};

int main() {
    int             fdSocket;
    int             fdClint;
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

              //        APV4      TCP
    fdSocket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    int ret_bind = setsockopt(fdSocket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    if (fdSocket == -1) {
        perror ("socket:");
        return (1);
    }
    int             max_socket = fdSocket;
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(8080);
    host_addr.sin_addr.s_addr = inet_addr("127.0.0.2");
    int host_addrlen = sizeof(host_addr);
    int clint_addrlen = sizeof(host_addr);
    if (bind(fdSocket, (struct sockaddr *)&host_addr, host_addrlen) != 0) {
        perror ("bind");
        return (1);
    }
    server[fdSocket];
    if (listen(fdSocket, 100) != 0) {
        perror ("listen");
        return (1);
    }
    FD_SET(fdSocket, &readSet);
    while(true) {
        tmp_readSet = readSet;
        tmp_writeSet = writeSet;
        int answer =  select(max_socket + 1, &tmp_readSet, &tmp_writeSet, 0, 0);
        if (answer == -1) {
            std::cerr << "error: select" << "\n";
            exit (1);
        }
        if (FD_ISSET(fdSocket, &tmp_readSet))
        {
            fdClint = accept(fdSocket, (struct sockaddr*)&host_addr, (socklen_t *)&host_addrlen);  // non block
            if (fdClint == -1) {
                perror("accept");
                exit (1);
            }
            std::cout << "========================> accept clint: " << fdClint << "\n";
            server[fdClint];
            server[fdClint].request_finish = 0;
            server[fdClint].response_finish = 0;

            int max = (*(--server.end())).first;
            max_socket = std::max(max, max_socket);
            FD_SET(fdClint, &readSet);
            answer--;
        }
        for (it = server.begin(); it != server.end() && answer; it++) {
            signal(SIGPIPE, SIG_IGN);
            const int   &fdClint = it->first;
            Clint       &ClintObj = it->second;
            if (FD_ISSET(it->first, &tmp_readSet)) {
                char buf[1024];
                bzero(buf, 1024);
                int size = read(it->first, buf, 1024);
                if (size < 0)
                {
                    perror("read;");
                    answer--;
                    continue;
                }
                it->second.buff.append(buf, size);
                if (it->second.buff.find("\r\n\r\n"))
                {
                    std::cout << "found" << std::endl;
                    it->second.request_finish = 1;
                }
                if (it->second.request_finish == 1)
                {
                    FD_CLR(it->first, &readSet);
                    FD_SET(it->first, &writeSet);
                    it->second.buff = "";
                    answer--;
                }
                else
                    answer--;
            }
            else if (FD_ISSET(it->first, &tmp_writeSet)) {
                ClintObj.response = "HTTP/1.1 200 OK\r\n";
                ClintObj.response += "Content-Type: text/html\r\n";
                ClintObj.response += "Connection: Keep-Alive\r\n";
                ClintObj.response += "content-length: ";
                std::string ss("<html><h1>hello world</h1></html>");
                ClintObj.response.append(std::to_string(ss.size())).append("\r\n\r\n");
                ClintObj.response += ss;
                ClintObj.response;
                int ret = write(it->first, ClintObj.response.c_str(), ClintObj.response.size());
                if (ret != ClintObj.response.size())
                {
                    std::cout << "error " << std::endl;
                }
                std::cout << "|" << ClintObj.response << "|" << std::endl; 
                FD_CLR(it->first, &writeSet);
                FD_SET(it->first, &readSet);
                answer--;
            }
            else {}
        } 
    }
}



