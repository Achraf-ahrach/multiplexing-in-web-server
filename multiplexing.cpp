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
    std::cout << "accept client [" << fdClient << "]\n";
    server[fdClient];
    FD_SET(fdClient, &readSet);
}

void    request(std::map<int, Client>::iterator &it, std::vector<int> &clear, fd_set &readSet, fd_set &writeSet)
{
    char buf[1024];

    bzero(buf, 1024);
    int read_len = recv(it->first, buf, 1024, 0);
    if (read_len <= 0)
    {
        perror("read");
        FD_CLR(it->first, &readSet);
        clear.push_back(it->first);
        return ;
    }
    it->second.buf.append(buf, read_len);
    if (it->second.buf.find("\r\n\r\n"))
    {
        std::cout << "read finish\n";
        FD_CLR(it->first, &readSet);
        FD_SET(it->first, &writeSet);
        it->second.buf = "";
    }
}

void    SendInSocket(std::map<int, Client>::iterator &it, std::vector<int> &clear, fd_set &readSet, fd_set &writeSet)
{
    int HowIwillsend = 1024;
    if (it->second.response.size() < 1024)
        HowIwillsend = it->second.response.size();
    int sizeRead = send(it->first, it->second.response.c_str(), HowIwillsend, MSG_NOSIGNAL);
    if (sizeRead == -1)
    {
        perror("send");
        FD_CLR(it->first, &writeSet);
        clear.push_back(it->first);
    }
    else
    {
        it->second.response.erase(0, sizeRead);
        if (it->second.response.empty())
            it->second.response = "";
    }
}

void response(std::map<int, Client>::iterator &it, std::vector<int> &clear, fd_set &readSet, fd_set &writeSet)
{
    if (!it->second.inputFile.is_open())
    {
        it->second.inputFile.open("video.mp4", std::ios::binary);
        if (!it->second.inputFile.is_open())
        {
            std::cerr << "open inputFile\n";
            FD_CLR(it->first, &writeSet);
            clear.push_back(it->first);
        }
        it->second.response = "HTTP/1.1 200 OK\n\r";
        it->second.response += "Content-Type: video/mp4\n\r";
        it->second.response += "Connection: Keep-Alive\n\r";
        it->second.response += "Content-Length: ";
        it->second.inputFile.seekg(0, it->second.inputFile.end);
        int length = it->second.inputFile.tellg();
        it->second.inputFile.seekg(0, it->second.inputFile.beg);
        it->second.response.append(std::to_string(length)).append("\r\n\r\n");
    }
    else
    {
        if (it->second.response.empty())
            SendInSocket(it, clear, readSet, writeSet);
        else
        {
            char buf[1024];
            bzero(buf, 1024);
            it->second.inputFile.read(buf, 1024);
            it->second.bufInputFile.append(buf, it->second.inputFile.gcount());
            it->second.response += it->second.bufInputFile;
            SendInSocket(it, clear, readSet, writeSet);
            it->second.bufInputFile = "";
            if (it->second.inputFile.gcount() < 1024)
            {
                it->second.inputFile.close();
                FD_CLR(it->first, &writeSet);
                FD_SET(it->first, &readSet);
                std::cout << "finish write\n";
            }
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
    if (fdSocket == -1)
    {
        perror("Error fdSocket");
        exit (1);
    }
    int opt = 1;
    if (setsockopt(fdSocket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt");
        exit (1);
    }

    if (bind(fdSocket, (struct sockaddr *)&addr,addr_len) == -1)
    {
        perror("Error bind");
        exit (0);
    }
    if (listen(fdSocket, 100) == -1)
    {
        perror("Error listen");
        exit (1);
    }

    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);

    FD_SET(fdSocket, &readSet);
    while (1)
    {
        tmp_readSet = readSet;
        tmp_writeSet = writeSet;

        int max_socket = fdSocket;
        if (!server.empty())
            max_socket = (--server.end())->first;

        int nbrSelect = select(max_socket + 1, &tmp_readSet, &tmp_writeSet, 0, 0);
        if (nbrSelect == -1)
        {
            perror("Error select");
            exit (1);
        }
        if (FD_ISSET(fdSocket, &tmp_readSet))
        {
            nbrSelect--;
            acceptClient(fdSocket, server, readSet);
        }
        for(it = server.begin(); it != server.end() && nbrSelect > 0; it++)
        {
            if (FD_ISSET(it->first, &tmp_readSet))
            {
                nbrSelect--;
                request(it, clear, readSet, writeSet);
            }
            else if (FD_ISSET(it->first, &tmp_writeSet))
            {
                nbrSelect--;
                response(it, clear, readSet, writeSet);
            }
        }
        for (int i = 0; i < clear.size(); i++)
        {
            std::cout << "close client [" << clear[i] << "]\n";
            server.erase(clear[i]);
            close(clear[i]);
        }
        clear.clear();
    }
}
