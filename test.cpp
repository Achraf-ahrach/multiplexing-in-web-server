#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include <sys/select.h>

#include <iostream>

#include <map>
#include <set>
#include <vector>

#define ANSWER "<h1>HELLO</h1>\n"

int main()
{



    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {perror("socket"); exit(2);}

    struct sockaddr_in ss;
    memset(&ss, 0, sizeof(ss));

    ss.sin_family = AF_INET;
    ss.sin_port = htons(9090);
    ss.sin_addr.s_addr = INADDR_ANY;

    	int optval = 1;
	    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optval));

    int i = bind (fd, (sockaddr*)&ss, sizeof(ss));
    if (i < 0) {perror("bind"); exit(2);}

    i = listen (fd, 100);
    if (i < 0) {perror("listen"); exit(2);}

    fd_set readSet;
    fd_set writeSet;
    fd_set readSetTemporary;
    fd_set writeSetTemporary;


    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);

    FD_SET(fd, &readSet);

    int maxSock = fd;
    std::map<int, int> client;
    std::vector<int> toDelete;
    while (true)
    {

        readSetTemporary = readSet;
        writeSetTemporary = writeSet;

        if (!client.empty())
        {
            maxSock = (--client.end())->first;
        }
    
        int s = select(maxSock + 1, &readSetTemporary, &writeSetTemporary, NULL , NULL);

        if (s == -1)
            {perror("select"); exit(2);}
        else
        {
            if (FD_ISSET(fd, &readSetTemporary))
            {
                int c = accept(fd, NULL, NULL);
                if (c < 0)
                {
                    {perror("accept"); exit(2);}
                }
                client[c];
                FD_SET(c, &readSet);
            }
            for (auto c : client)
            {
                const int &fileDescriptor = c.first;
                if (FD_ISSET(fileDescriptor, &readSetTemporary))
                {
                    char a[2000] = {};
                    std::cout << "read from ==> " << fileDescriptor << "\n";

                    int h = read(fileDescriptor, a, 1999);

                    if (h == -1 || !h)
                    {
                        perror("read");
                        close(fileDescriptor);
                        FD_CLR(fileDescriptor, &readSet);
                        toDelete.push_back(fileDescriptor);
                    }
                    else
                    {
                        a[h] = 0;
                        std::cout << "what did i read:\n[\n" << a << "\n]\n";
                        FD_CLR(fileDescriptor, &readSet);
                        FD_SET(fileDescriptor, &writeSet);
                    }

                }
                else if (FD_ISSET(fileDescriptor, &writeSetTemporary))
                {
                    std::cout << "write to ==> " << fileDescriptor << "\n";
                    int k = send(fileDescriptor, ANSWER, strlen(ANSWER), MSG_NOSIGNAL);
                    if (k == -1)
                    {
                        perror("send");
                        close(fileDescriptor);
                        FD_CLR(fileDescriptor, &writeSet);
                        toDelete.push_back(fileDescriptor);
                    }
                    else
                    {
                        FD_CLR(fileDescriptor, &writeSet);
                        FD_SET(fileDescriptor, &readSet);
                        //close(fileDescriptor); // comment if you dont want to close
                    }
                }
            }

            for (auto i : toDelete)
            {
                client.erase(i);
                std::cout << "JJ\n";
            }
            toDelete.clear();
        }

    /*
        go to terminal enter
         telnet localhost 9090
         enter anything
    */

    }
}
