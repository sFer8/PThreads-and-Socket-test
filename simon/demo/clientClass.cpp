#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

using namespace std;

class clientDataTransfer {
    private:
        int PORTS[5] = {10080, 10081, 10082, 10083, 10084};
        int status, valread, client_fd;
        struct sockaddr_in serv_addr;
        char buffer[1024] = { 0 };

        int socketInitialization() {
            if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
                printf("\n Socket creation error \n");
                return 1;
            }
            return 0;
        }

        int ipTextToBinary () {
            if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
                printf("\nInvalid address/ Address not supported \n");
                return 1;
            }
            return 0;
        }

        int connectionStatus() {
            if ((status= connect(client_fd, (struct sockaddr*)&serv_addr,sizeof(serv_addr)))< 0) {
                printf("\nConnection Failed \n");
                return 1;
            }
            return 0;
        }

        bool searchForPorts() {
            int i = 0;
            while (true) {
                cout << "I: " << i << endl;

                if (socketInitialization()) return 1;  // Move this inside the loop

                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons(PORTS[i]);

                if (ipTextToBinary()) return 1;

                if (connectionStatus()) {
                    close(client_fd);  // Close failed socket before retrying
                    i++;
                    if (i >= 5) {
                        cout << "No available ports found from 10080 to 10084" << endl;
                        return 1;
                    }
                } else {
                    cout << "Connected to port: " << PORTS[i] << endl;
                    break;
                }
            }
            return 0;
        }

    public:
        int startSocketClient() {
            if(socketInitialization()) return 1;

            serv_addr.sin_family = AF_INET;
            if(searchForPorts()) return 1;
            return 0;
        }

        void sendData(char* mail) {
            send(client_fd, mail, strlen(mail), 0);
            printf("Message sent\n");
        }

        char* receiveData() {
            valread = read(client_fd, buffer, 1024 - 1);
            buffer[valread] = '\0';

            return buffer;
        }

        void closeSocket() {
            close(client_fd);
        }
};

int main() {
    char message[] = "HelloThere";
    char* testMessage = message;

    clientDataTransfer client;
    while(!client.startSocketClient()) {
        client.sendData(testMessage);

        char* lineFromSocket = {client.receiveData()};
        cout << lineFromSocket << endl;

        client.closeSocket();
        break;
    }
    
}