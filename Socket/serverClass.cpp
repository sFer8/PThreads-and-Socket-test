#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
//#define PORT 8080


class hostDataTransfer {
    private:
        int PORTS[5] = {8080, 8081, 8082, 8083, 8084};
        int server_fd, new_socket;
        //ssize_t valread;
        char valread;
        struct sockaddr_in address;
        int opt = 1;
        socklen_t addrlen = sizeof(address);
        char buffer[1024] = { 0 };

        bool socketInitialization() {
            if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
                perror("socket failed");
                exit(EXIT_FAILURE);
                return 1;
            }
            return 0;
        }

        bool forceConnectSocket() {
            if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
                perror("setsockopt");
                exit(EXIT_FAILURE);
                return 1;
            }
            return 0;
        }

        bool connectHostToPort () {
            if (bind(server_fd, (struct sockaddr*)&address,
             sizeof(address))
                < 0) {
                perror("bind failed");
                exit(EXIT_FAILURE);
                return 1;
            }
            if (listen(server_fd, 3) < 0) {
                perror("listen");
                exit(EXIT_FAILURE);
                return 1;
            }
            if ((new_socket
                = accept(server_fd, (struct sockaddr*)&address,
                        &addrlen))
                < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
                return 1;
            }
            return 0;
        }

    public:
        bool startSocketHost() {
            if(socketInitialization()) return 1;
            if(forceConnectSocket()) return 1;

            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(PORT);

            if(connectHostToPort()) return 1;

            return 0;
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

using namespace std;

int main(int argc, char const* argv[])
{
    char message[] = "GeneralKenobi";
    char* testMessage = message;

    hostDataTransfer host;
    while(!host.startSocketHost()) {
        char* lineFromSocket = {host.receiveData()};
        cout << lineFromSocket << endl;

        host.sendData(testMessage);

        host.closeConnectedSockets();
        host.closeListeningSockets();
        break;
    }
    return 0;
}