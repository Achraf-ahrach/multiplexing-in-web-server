#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>
#include <map>
#include <vector>

class Client {
    public:
        std::string request;
        std::string response;
        std::string inputFile;
};

int main ()
{
    int                     fdSocket;
    fd_set                  readSet;
    fd_set                  writeSet;
    fd_set                  tmp_readSet;
    fd_set                  tmp_writeSet;
    std::map<int, Client>   server;
    std::map<int, Client>::iterator it;
    std::vector<int>        clear;


    struct sockaddr_in addr;

    addr.sin_addr.s_addr = inet_addr("127.0.0.2");
    addr.sin_port  = htons(9090);
    addr.sin_family = AF_INET;
    int addr_len = sizeof(addr);

    fdSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (fdSocket == -1) {
        perror("Error fdSocket");
        exit (1);
    }
    int opt = 1;
    if (setsockopt(fdSocket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt) == -1)) {
        perror("setsockopt");
        exit (1);
    }
    server[fdSocket];
    if (bind(fdSocket, (struct sockaddr *)&addr,addr_len) == -1) {
        perror("Error bind");
        exit (0);
    }
    if (listen(fdSocket, 100) == -1) {
        perror("Error listen");
        exit (1);
    }
    FD_SET(fdSocket, &readSet);
    while (1) {
        tmp_readSet = readSet;
        tmp_writeSet = writeSet;
        int max_socket = (--server.end())->first;
        int nbrSelect = select(max_socket + 1, &tmp_readSet, &tmp_writeSet, 0, 0);
        if (nbrSelect == -1) {
            perror("Error select");
            exit (1);
        }
        if (FD_ISSET(fdSocket, &tmp_readSet)) {
            int fdClient = accept(fdSocket, (struct sockaddr *)&addr, (socklen_t *)&addr_len);
            if (fdClient == -1) {
                perror("accept");
                exit (1);
            }
            server[fdClient];
            FD_SET(fdClient, &tmp_readSet);
            nbrSelect--;
        }
        for(it = server.begin(); it != server.end(); nbrSelect, it++) {
            if (FD_ISSET(it->first, &tmp_readSet)) {
                char buf[100000];
                int read_len = recv(it->first, buf, 100000, 0);
                if (read_len <= 0) {
                    FD_CLR(it->first, &tmp_readSet);
                    //close(it->first);
                    clear.push_back(it->first);
                }
                buf[read_len] = '\0';
                if (read_len < 100000) {
                    std::cout << "read finish\n";
                    FD_SET(it->first, &writeSet);
                }
                //std::cout << "\n{" << buf << "}\n";
                nbrSelect--;
            }
            else if (FD_ISSET(it->first, &tmp_writeSet)) {
                std::string str = "<html><h1>hello achraf</html></h1>";
                it->second.response = "HTTP/1.1 200 OK\n\r";
                it->second.response += "Content-Type: text/html\n\r";
                it->second.response += "Connection: Keep-Alive\n\r";
                it->second.response += "Content-Lengh: \n\r";
                it->second.response.append(std::to_string(str.size())).append("\r\n\r\n");
                it->second.response += str;
                if (send(it->first, it->second.response.c_str(), it->second.response.size(), MSG_NOSIGNAL) != it->second.response.size()) {
                    perror("error");
                    FD_CLR(it->first, &tmp_readSet);
                    close(it->first);
                    clear.push_back(it->first);
                }
                FD_SET(it->first, &readSet);
                std::cout << "here\n";
                nbrSelect--;
            }
        }
        for (int i = 0; i < clear.size(); i++) {
            server.erase(clear[i]);
        }
    }
}