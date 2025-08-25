#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#define PORT 8080

class dataTransfer {
    private:
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

    public:
        int startSocket() {
            if(socketInitialization()) return 1;

            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT);

            if(ipTextToBinary()) return 1;
            if(connectionStatus()) return 1;
            return 0;
        }

        void sendData(char* mail) {
            send(client_fd, mail, strlen(mail), 0);
            printf("Message sent\n");
        }

        char* receiveData() {
            valread = read(client_fd, buffer, 1024 - 1);
            buffer[valread] = '\0';
            //printf("%s\n", buffer);

            return buffer;
        }

        void closeSocket() {
            close(client_fd);
        }
};

using namespace std;

int main() {
    char message[] = "HelloThere";
    char* testMessage = message;

    dataTransfer client;
    while(!client.startSocket()) {
        client.sendData(testMessage);

        char* lineFromSocket = {client.receiveData()};
        cout << lineFromSocket << endl;

        client.closeSocket();
        break;
    }
    
}