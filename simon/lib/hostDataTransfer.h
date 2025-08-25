#ifndef hostDataTransfer_H
#define hostDataTransfer_H

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <vector>

using namespace std;

class hostDataTransfer {
    private:
        int PORTS[5];
        int server_fd, new_socket;
        char valread;
        struct sockaddr_in address;
        int opt;
        socklen_t addrlen;
        char buffer[1024];

        void socketInitialization();
        void connectHostToPort();

    public:
        void startSocketHost(int* port);
        char* receiveData();
        void sendData(char* mail);
        void sendVectorStringData(vector<vector<string>>* mailVector);
        void closeConnectedSockets();
        void closeListeningSockets();

};

#endif