#include <cstring>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <csignal>
#include <map>
#include <vector>

class Client {
    public:
        std::string     request;
        std::string     response;
        std::ifstream   inputFile;
        int             isFinishReadInputFile;
        std::string     bufInputFile;
        std::string     buf;
};

void    acceptClient(int fdSocket, std::map<int, Client> &server, fd_set &readSet)
{
    struct sockaddr_in addr;
    int addr_len = sizeof(addr);

    int fdClient = accept(fdSocket, (struct sockaddr *)&addr, (socklen_t *)&addr_len);
    if (fdClient == -1) {
        perror("accept");
        exit (1);
    }
    server[fdClient];
    server[fdClient].isFinishReadInputFile = 0;
    FD_SET(fdClient, &readSet);
}

int request(std::map<int, Client>::iterator &it, std::vector<int> &clear, fd_set &readSet, fd_set &writeSet)
{
    char buf[1024];

    bzero(buf, 1024);
    int read_len = recv(it->first, buf, 1024, 0);
    if (read_len <= 0) {
        perror("read");
        FD_CLR(it->first, &readSet);
        close(it->first);
        clear.push_back(it->first);
        return 0;
    }
    it->second.buf.append(buf, read_len);
    if (it->second.buf.find("\r\n\r\n")) {
        std::cout << "read finish\n";
        FD_CLR(it->first, &readSet);
        FD_SET(it->first, &writeSet);
        it->second.buf = "";
    }
    return 1;
}

void response(std::map<int, Client>::iterator &it, std::vector<int> &clear, fd_set &readSet, fd_set &writeSet)
{
    if (it->second.isFinishReadInputFile) {
        int HowIwillsend = 1024;
        if (it->second.response.size() < 1024)
            HowIwillsend = it->second.response.size();
        int sizeRead = send(it->first, it->second.response.c_str(), HowIwillsend, MSG_NOSIGNAL);
        if (sizeRead == -1) {
            perror("send");
            FD_CLR(it->first, &writeSet);
            close(it->first);
            clear.push_back(it->first);
        }
        else {
            it->second.response.erase(0, sizeRead);
            if (it->second.response.empty()) {
                it->second.isFinishReadInputFile = 0;
                FD_CLR(it->first, &writeSet);
                FD_SET(it->first, &readSet);
                std::cout << "finish write\n";
            }
        }
    }
    else if (!it->second.inputFile.is_open()) {
        it->second.inputFile.open("video.mp4", std::ios::binary);
        if (!it->second.inputFile.is_open()) {
            std::cerr << "open inputFile\n";
            FD_CLR(it->first, &writeSet);
            close(it->first);
            clear.push_back(it->first);
        }
    }
    else {
        char buf[1024];
        bzero(buf, 1024);
        it->second.inputFile.read(buf, 1024);
        it->second.bufInputFile.append(buf, it->second.inputFile.gcount());
        if (it->second.inputFile.gcount() < 1024) {
            it->second.response = "HTTP/1.1 200 OK\n\r";
            it->second.response += "Content-Type: video/mp4\n\r";
            it->second.response += "Connection: Keep-Alive\n\r";
            it->second.response += "Content-Length: ";
            it->second.response.append(std::to_string(it->second.bufInputFile.size())).append("\r\n\r\n");
            it->second.response += it->second.bufInputFile;
            it->second.isFinishReadInputFile = 1;
        }
    }
}

int main ()
{
    int                     fdSocket;
    fd_set                  readSet;
    fd_set                  writeSet;
    fd_set                  tmp_readSet;
    fd_set                  tmp_writeSet;
    int                     max_socket;
    std::map<int, Client>   server;
    std::map<int, Client>::iterator it;
    std::vector<int>        clear;


    struct sockaddr_in addr;

    addr.sin_addr.s_addr = inet_addr("127.0.0.2");
    addr.sin_port  = htons(8080);
    addr.sin_family = AF_INET;
    int addr_len = sizeof(addr);

    fdSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (fdSocket == -1) {
        perror("Error fdSocket");
        exit (1);
    }
    int opt = 1;
    if (setsockopt(fdSocket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
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
    max_socket = fdSocket;
    while (1) {
        tmp_readSet = readSet;
        tmp_writeSet = writeSet;

        int max = (--server.end())->first;
        max_socket = std::max(max, max_socket);
        int nbrSelect = select(max_socket + 1, &tmp_readSet, &tmp_writeSet, 0, 0);
        if (nbrSelect == -1) {
            perror("Error select");
            exit (1);
        }
        if (FD_ISSET(fdSocket, &tmp_readSet)) {
            nbrSelect--;
            acceptClient(fdSocket, server, readSet);
        }
        for(it = server.begin(); it != server.end(); nbrSelect, it++) {
            if (FD_ISSET(it->first, &tmp_readSet)) {
                nbrSelect--;
                if (!request(it, clear, readSet, writeSet))
                    continue;
            }
            else if (FD_ISSET(it->first, &tmp_writeSet)) {
                nbrSelect--;
                response(it, clear, readSet, writeSet);
            }
        }
        for (int i = 0; i < clear.size(); i++) {
            server.erase(clear[i]);
        }
    }
}
