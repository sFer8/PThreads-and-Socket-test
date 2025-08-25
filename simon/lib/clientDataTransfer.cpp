#include "clientDataTransfer.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

using namespace std;

int clientPORTS[5] = {10080, 10081, 10082, 10083, 10084};
int status, clientValRead, client_fd;
struct sockaddr_in serv_addr;
char clientbuffer[1024] = { 0 };

int clientDataTransfer::socketInitialization() {
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        printf("\n Socket creation error \n");
        return 1;
    }
    return 0;
}

int clientDataTransfer::ipTextToBinary () {
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return 1;
    }
    return 0;
}

int clientDataTransfer::connectionStatus() {
    if ((status= connect(client_fd, (struct sockaddr*)&serv_addr,sizeof(serv_addr)))< 0) {
        return 1;
    }
    return 0;
}

int clientDataTransfer::searchForPorts() {
    int i = 0;
    while (true) {
        socketInitialization();

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(clientPORTS[i]);

        ipTextToBinary();

        if (connectionStatus()) {
            close(client_fd);
            i++;
            if (i >= 5) {
                cout << "No available ports in " << clientPORTS[i] << endl;
                return 0;
            }
        } else {
            cout << "Connected to port: " << clientPORTS[i] << endl;
            return clientPORTS[i];
        }
    }
}

int clientDataTransfer::startSocketClient() {
    if(socketInitialization()) return 1;
    serv_addr.sin_family = AF_INET;

    return 0;
}

void clientDataTransfer::sendData(char* mail) {
    send(client_fd, mail, strlen(mail), 0);
}

char* clientDataTransfer::receiveData() {
    clientValRead = read(client_fd, clientbuffer, 1024 - 1);
    clientbuffer[clientValRead] = '\0';

    return clientbuffer;
}

void clientDataTransfer::closeSocket() {
    close(client_fd);
}