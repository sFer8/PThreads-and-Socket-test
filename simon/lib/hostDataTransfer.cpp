#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include "hostDataTransfer.h"
#include <vector>

using namespace std;

//int PORTS[5] = {10080, 10081, 10082, 10083, 10084};
int server_fd, new_socket;
char valread;
struct sockaddr_in address;
int opt = 1;
socklen_t addrlen = sizeof(address);
char buffer[1024] = { 0 };

void hostDataTransfer::socketInitialization() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
}

void hostDataTransfer::connectHostToPort () {
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);
    new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
}

void hostDataTransfer::startSocketHost(int* port) {
    socketInitialization();

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(*port);

    connectHostToPort();
}

char* hostDataTransfer::receiveData() {
    valread = read(new_socket, buffer, 1024 - 1); 
    buffer[valread] = '\0';

    return buffer;
}

void hostDataTransfer::sendData(char* mail) {
    send(new_socket, mail, strlen(mail), 0);
}

void hostDataTransfer::sendVectorStringData(vector<vector<string>>* mailVector) {
    send(new_socket, mailVector, mailVector->size(), 0);
}

void hostDataTransfer::closeConnectedSockets() {
    close(new_socket);
}

void hostDataTransfer::closeListeningSockets() {
    close(server_fd);
}
