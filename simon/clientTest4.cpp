#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

//g++ -std=c++11 clientTest4.cpp ./lib/fileManager.cpp -lstdc++fs -o client4

#include "./lib/fileManager.h"

void printFileList(vector<vector<string>> listOfIDNames);
string serialize2DVector(const vector<vector<string>>& vec);

int main()
{
    myFile file;
    string filePath = "./DummyFiles/seed4";
    // creating socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    int PORTS[5] = {8080, 8081, 8082, 8083, 8084};

    // specifying address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;

    int x = 0;
    while(true) {
        int i = 0;
        while(true) {
            serverAddress.sin_port = htons(PORTS[i]);
            serverAddress.sin_addr.s_addr = INADDR_ANY;

            // sending connection request
            if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == 0) {
                std::cout << "client is starting at port " << PORTS[i] << std::endl;
                break;
            } else {
                perror("connect");
            }
            i++;
            if(i >= (sizeof(PORTS) / sizeof(PORTS[0]))) {
                std::cout << "No port found" << std::endl;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        x++;
        if(x >= 10) {
            break;
        }
    }
    

    
    // sending data
    // const char* message = "";
    // send(clientSocket, message, strlen(message), 0);

    int userInput = 0;
    while(true) {
        char buffer[1024] = { 0 };
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if(bytesReceived <= 0) {
            continue;
        }
        else {
            buffer[bytesReceived] = '\0'; // make sure it's a string
            std::cout << "Message from Server: " << buffer << std::endl;

            int userInput = atoi(buffer);

            if(userInput == 1) {
                std::cout << "sending out files" << std::endl;


                vector<vector<string>> listOfFiles = file.getIDFileList(filePath);
                printFileList(listOfFiles);
                //send(clientSocket, &listOfFiles, listOfFiles.size(), 0);

                string serialized = serialize2DVector(listOfFiles);
                send(clientSocket, serialized.c_str(), serialized.size(), 0);
            }
            if(userInput == 2) {
                //download using file lib
                //look up fseek
            }
            if(userInput == 3) {
                //download status
                //might need to send info aside from the file itself
            }
            if(userInput == 4) {
                //add code to close socket ports
                break;
            }
            userInput = 0;
        }
        
    }

    // closing socket
    close(clientSocket);

    return 0;
}

void printFileList(vector<vector<string>> listOfIDNames) {
    for (int i = 0; i < listOfIDNames.size(); i++) {
        std::cout << "[" << listOfIDNames[i][0] << "]  " << listOfIDNames[i][1] <<"\n";
    }
    std::cout << std::endl;
}

string serialize2DVector(const vector<vector<string>>& vec) {
    string data;
    for (const auto& row : vec) {
        for (size_t i = 0; i < row.size(); ++i) {
            data += row[i];
            if (i < row.size() - 1) data += "|";
        }
        data += "\n";
    }
    return data;
}