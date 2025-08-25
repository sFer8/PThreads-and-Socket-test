#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

using namespace std;

class hostDataTransfer {
    private:
        int PORT = 10082;
        int server_fd, new_socket;
        char valread;
        struct sockaddr_in address;
        int opt = 1;
        socklen_t addrlen = sizeof(address);
        char buffer[1024] = { 0 };

        void socketInitialization() {
            server_fd = socket(AF_INET, SOCK_STREAM, 0);
        }

        void forceConnectSocket() {
            setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
        }

        void connectHostToPort () {
            bind(server_fd, (struct sockaddr*)&address, sizeof(address));
            listen(server_fd, 3);
            new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        }

    public:
        void startSocketHost() {
            socketInitialization();
            forceConnectSocket();

            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(PORT);

            connectHostToPort();
        }

        char* receiveData() {
            valread = read(new_socket, buffer, 1024 - 1); 
            buffer[valread] = '\0';

            return buffer;
        }

        void sendData(char* mail) {
            send(new_socket, mail, strlen(mail), 0);
            printf("Message sent\n");
        }

        void closeConnectedSockets() {
            close(new_socket);
        }

        void closeListeningSockets() {
            close(server_fd);
        }

};

int main(int argc, char const* argv[])
{
    char message[] = "";
    char* testMessage = message;

    hostDataTransfer host;
    host.startSocketHost();
    char* lineFromSocket = {host.receiveData()};
    cout << lineFromSocket << endl;

    host.sendData(testMessage);

    host.closeConnectedSockets();
    host.closeListeningSockets();

    return 0;
}