#ifndef clientDataTransfer_H
#define clientDataTransfer_H

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

using namespace std;

class clientDataTransfer {
    private:
        int PORTS[5];
        int status, valread, client_fd;
        struct sockaddr_in serv_addr;
        char buffer[1024];

        int socketInitialization();
        int ipTextToBinary ();
        int connectionStatus();

    public:
        int startSocketClient();
        int searchForPorts();
        void sendData(char* mail);
        char* receiveData();
        void closeSocket();
};

#endif