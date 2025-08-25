#include <iostream>
#include <string>
#include <experimental/filesystem>
#include <iostream>
#include <sys/stat.h>
#include <vector>
#include <stdio.h>

#include "./lib/fileManager.h"
#include "./lib/hostDataTransfer.h"
#include "./lib/clientDataTransfer.h"
#include "./lib/pThread.h"

//g++ -o server serverClass.cpp
//cd /data1/p4work/fernansf/simon/demo/serverTest
//g++ -std=c++11 -lpthread main.cpp ./lib/fileManager.cpp ./lib/hostDataTransfer.cpp ./lib/clientDataTransfer.cpp ./lib/pThread.cpp -lstdc++fs -o main
//./main

using namespace std;

myFile file;
pThread pThreads;
clientDataTransfer client;

void printMenu();
void connectViaSocket();
void creatPThread();
void printFileList(string path);
void printPorts();

vector<int> connectedPortNumbers;

int main() {
    for(int i = 0; i <= 20; i++) {
        cout << endl;
    }

    creatPThread();
    cout << "Finding available ports... Found port(s) ";
    printPorts();
    cout << "\nListening at ports ";
    printPorts();
    cout << "\n\n";
    

    int userInput = 0;
    myFile file;
    const string filePath = "./DummyFiles/seed";
    //string filePath = "./DummyFiles/blankFolderTest";

    while(true) {
        printMenu();
        cin >> userInput;
        cout << endl;

        if(userInput == 1) {
            cout << "Searching for files... done." << endl;

            char message[] = "send_list_of_files";
            char* testMessage = message;
            client.sendData(testMessage);

            int result = 0;
            file.checkFolderForFiles(filePath, result);

            if(result == 1) {
                cout << "Files available." << endl;
                printFileList(filePath);
            } else {
                cout << "No files found.\n" << endl;
            } 
        }
        if(userInput == 2) {
            //download using file lib
        }
        if(userInput == 3) {
            //download status
            //might need to send info aside from the file itself
        }
        if(userInput == 4) {
            //add code to close socket ports
            client.closeSocket();
            break;
        }

    }
}

void printPorts() {
    for (int i = 0; i < connectedPortNumbers.size(); i++) {
        if(connectedPortNumbers[i] != 0) {
            cout << connectedPortNumbers[i] << ", ";
        }
    }
}

void printFileList(string path) {
    vector<vector<string>> listOfIDNames = file.getIDFileList(path);
    for (int i = 0; i < 5; i++) {
        cout << "[" << listOfIDNames[i][0] << "]  " << listOfIDNames[i][1] <<"\n";
    }
    cout << endl;
}

void printMenu() {
    cout << "Seed App" << endl;
    cout << "[1] List available files." << endl;
    cout << "[2] Download file." << endl;
    cout << "[3] Download status." << endl;
    cout << "[4] Exits" << endl;
    cout << endl;
    cout << "? ";
}

void connectViaSocket() {
    clientDataTransfer client;
    char message[] = "HelloThere";
    char* testMessage = message;
    while(!client.startSocketClient()) {
        int port = client.searchForPorts();
        if(port != 0) {
            connectedPortNumbers.push_back(port);
        }
        
        client.sendData(testMessage);

        char* lineFromSocket = {client.receiveData()};
        cout << lineFromSocket << endl;

        client.closeSocket();
        break;
    }
}


void creatPThread() {
    for(int i = 0; i <= 4; i++) {
        pThreads.createPThread((functionParameter)connectViaSocket,NULL);
        pThreads.joinThread();
    }
}
