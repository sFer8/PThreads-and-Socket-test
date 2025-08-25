#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <vector>

#include "../lib/fileManager.h"
#include "../lib/hostDataTransfer.h"

//g++ -std=c++11 -lpthread server.cpp ../lib/fileManager.cpp ../lib/hostDataTransfer.cpp -lstdc++fs -o server

void printFileList(string path);
vector<vector<string>> getFileList(string path);
void printFileList(vector<vector<string>>* list);

myFile file;

using namespace std;
using namespace experimental::filesystem;


int main()
{
    
    hostDataTransfer server;

    char message[] = "";
    char* testMessage = message;

    int port = 10080;
    int* portPTR = &port;
    server.startSocketHost(portPTR);

    char* lineFromSocket = {server.receiveData()};
    cout << lineFromSocket << endl;

    const string filePath = "../DummyFiles/seed";
    int result = 1;
    file.checkFolderForFiles(filePath, result);
    
    while(true) {
        char* lineFromSocket = {server.receiveData()};
        cout << "client command: " << lineFromSocket << endl;

        if(lineFromSocket == "send_list_of_files") {
            if(result == 1) {
                cout << "Files available." << endl;
                vector<vector<string>> listOfFiles = getFileList(filePath);
                vector<vector<string>>* listOfFilesPackage = &listOfFiles;

                server.sendVectorStringData(listOfFilesPackage);
                printFileList(listOfFilesPackage);
            } else {
                cout << "No files found.\n" << endl;
            }
        }
  
    }
    
    server.closeConnectedSockets();
    server.closeListeningSockets();

    return 0;
}

vector<vector<string>> getFileList(string path) {
    vector<vector<string>> listOfIDNames = file.getIDFileList(path);
    return listOfIDNames;
}

void printFileList(vector<vector<string>>* list) {
    for (int i = 0; i < 5; i++) {
        cout << "[" << (*list)[i][0] << "]  " << (*list)[i][1] << "\n";
    }
    cout << endl;
}
