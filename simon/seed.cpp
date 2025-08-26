#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <stdio.h>
#include <pthread.h>
#include <chrono>
#include <thread>
#include <map>
#include <set>
#include <algorithm>
#include <cstdlib>
#include <random>

#include "./lib/fileManager.h"
#include "./lib/hostDataTransfer.h"
#include "./lib/clientDataTransfer.h"
#include "./lib/pThread.h"

//g++ -std=c++11 -lpthread seed.cpp ./lib/fileManager.cpp ./lib/hostDataTransfer.cpp ./lib/clientDataTransfer.cpp ./lib/pThread.cpp -lstdc++fs -o seed

struct ServerContext {
    int server_fd;
    vector<int> clients;
    pthread_mutex_t lock;
    std::map<int, vector<vector<string>>> clientData;
};

ServerContext ctx;

vector<vector<string>> deserializeVector(const string& data) {
    vector<vector<string>> result;
    std::stringstream ss(data);
    string line;

    while (std::getline(ss, line)) { 
        if (line.empty()) continue;
        std::stringstream rowStream(line);
        string col;
        vector<string> row;
        while (std::getline(rowStream, col, '|')) {
            if (!col.empty())
                row.push_back(col);
        }
        result.push_back(row);
    }
    return result;
}

void* createListen(void* inClientSocket) {
    int clientSocket = *((int*)inClientSocket);

    char buffer[1024] = { 0 };
    while(true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if(bytesReceived <= 0) {
            continue;
        }

        buffer[bytesReceived] = '\0';
        std::string receivedData(buffer);

        auto vec2D = deserializeVector(receivedData);

        pthread_mutex_lock(&ctx.lock);
        ctx.clientData[clientSocket] = vec2D;
        pthread_mutex_unlock(&ctx.lock);
    }
    close(clientSocket);
    return 0;
}

void printMenu() {
    std::cout << std::endl;
    std::cout << "Seed App" << std::endl;
    std::cout << "[1] List available files." << std::endl;
    std::cout << "[2] Download file." << std::endl;
    std::cout << "[3] Download status." << std::endl;
    std::cout << "[4] Exits" << std::endl;
    std::cout << std::endl;
    std::cout << "? ";
}

void* createServer(void* arg) {
    ServerContext* ctx = (ServerContext*)arg;
    int PORTS[5] = {8080, 8081, 8082, 8083, 8084};
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    ctx->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->server_fd < 0) {
        perror("socket failed");
        return nullptr;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;

    std::cout << "Finding available ports... Found port ";
    int i = 0;
    while (true) {
        address.sin_port = htons(PORTS[i]);
        if (bind(ctx->server_fd, (struct sockaddr*)&address, sizeof(address)) == 0) {
            std::cout << PORTS[i] << std::endl;
            
            listen(ctx->server_fd, 10);
            break;
        }
        i++;
        if (i >= (sizeof(PORTS) / sizeof(PORTS[0]))) {
            std::cout << "No ports available.\n";
            return nullptr;
        }
    }

    while (true) {
        int client_fd = accept(ctx->server_fd, (struct sockaddr*)&address, &addrlen);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        pthread_mutex_lock(&ctx->lock);
        ctx->clients.push_back(client_fd);
        pthread_mutex_unlock(&ctx->lock);

        std::cout << "Listening at port " << PORTS[i] << std::endl;
        pthread_t listenerThread;
        int* client_fd_ptr = new int(client_fd);
        pthread_create(&listenerThread, nullptr, createListen, client_fd_ptr);
        pthread_detach(listenerThread);
    }

    return nullptr;
}

void* createClient(void* arg) {
    int* out_fd = (int*)arg;
    int PORTS[5] = {8080, 8081, 8082, 8083, 8084};
    int status, valread, client_fd;
    struct sockaddr_in serv_addr;
    const char* hello = "Hello from client";
    char buffer[1024] = { 0 };
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
    }

    serv_addr.sin_family = AF_INET;

    int i = 0;
    while(true) {
        serv_addr.sin_port = htons(PORTS[i]);

        // Convert IPv4 and IPv6 addresses from text to binary form
        if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
            printf("\nInvalid address/ Address not supported \n");
        }

        if (connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == 0) {
            *out_fd = client_fd;

            break;
        }
        i++;
        if(i >= (sizeof(PORTS) / sizeof(PORTS[0]))) {
            std::cout << "No port found" << std::endl;
            break;
        }
    }
    
  
    send(client_fd, hello, strlen(hello), 0);
    valread = read(client_fd, buffer, 1024 - 1); 
    printf("%s\n", buffer);
    return 0;
}

int main() {
    myFile file;
    pThread pThreads;
    clientDataTransfer client;
    srand(time(0));

    ctx.server_fd = -1;
    pthread_mutex_init(&ctx.lock, nullptr);

    pthread_t serverThread;
    pthread_create(&serverThread, nullptr, createServer, &ctx);

    std::map<std::string, std::vector<int>> filePresence;
    std::map<std::string, std::string> fileIdMap;
    std::map<std::string, int> fileSizeMap;

    int userInput = 0;
    while (true) {
        int noOfClientsWithFile = 0, numberOfChunks = 0;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        printMenu();
        std::cin >> userInput;
        std::cout << std::endl;

        if (userInput == 1) {
            const char* cmd = "1";

            pthread_mutex_lock(&ctx.lock);
            ctx.clientData.clear();
            pthread_mutex_unlock(&ctx.lock);

            filePresence.clear();
            fileIdMap.clear();

            pthread_mutex_lock(&ctx.lock);
            for (int sock : ctx.clients) {
                send(sock, cmd, strlen(cmd), 0);
            }
            pthread_mutex_unlock(&ctx.lock);

            //std::this_thread::sleep_for(std::chrono::milliseconds(200));
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
            std::cout << "Searching for files.. ";
            while (true) {
                pthread_mutex_lock(&ctx.lock);
                size_t gotResponses = ctx.clientData.size();
                size_t expected = ctx.clients.size();
                pthread_mutex_unlock(&ctx.lock);

                if (gotResponses >= expected) break;
                if (std::chrono::steady_clock::now() > deadline) break;

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            std::cout << "done." << std::endl;

            std::set<std::string> uniqueFiles;
            std::cout << "Files available." << std::endl;
            for (std::map<int, std::vector<std::vector<std::string>>>::iterator it = ctx.clientData.begin();
                it != ctx.clientData.end(); ++it) 
            {
                int client = it->first;
                const std::vector<std::vector<std::string>>& rows = it->second;

                for (size_t j = 0; j < rows.size(); ++j) {
                    const std::vector<std::string>& row = rows[j];
                    if (row.size() >= 2) {
                        std::string entry = row[0] + "/" + row[1];
                        if (uniqueFiles.insert(entry).second) {
                        }

                        std::string fileId = row[0];
                        std::string fileName = row[1];
                        std::string fileSize = row[2];
                        std::string key = fileId + "|" + fileName;

                        fileIdMap[key] = fileId;
                        fileSizeMap[key] = stoi(fileSize);

                        if (filePresence.find(key) == filePresence.end()) {
                            filePresence[key] = std::vector<int>(ctx.clients.size(), 0);
                        }

                        auto itClient = std::find(ctx.clients.begin(), ctx.clients.end(), client);
                        if (itClient != ctx.clients.end()) {
                            int idx = std::distance(ctx.clients.begin(), itClient);
                            filePresence[key][idx] = 1;
                        }
                    }
                }
            }

            std::cout << "id\tfilename\t";
            for (size_t i = 0; i < ctx.clients.size(); i++) {
                std::cout << ctx.clients[i] << "\t";
            }
            std::cout << "\n";


            for (auto& kv : filePresence) {
                std::string key = kv.first;
                std::string id = fileIdMap[key];
                std::string fileName = key.substr(key.find("|") + 1);

                std::cout << "[" << id << "]" << "\t" << fileName << "\t";
                for (int v : kv.second) {
                    std::cout << v << "\t";
                }
                std::cout << "\n";
            }

            //std::cout << << endl;
            //pthread_mutex_unlock(&ctx.lock);

        }
        else if (userInput == 2) {
            string fileID = "0";
            const char* cmd = "2";

            pthread_mutex_lock(&ctx.lock);
            ctx.clientData.clear();
            pthread_mutex_unlock(&ctx.lock);

            pthread_mutex_lock(&ctx.lock);
            for (int sock : ctx.clients) {
                send(sock, cmd, strlen(cmd), 0);
            }
            pthread_mutex_unlock(&ctx.lock);

            std::cout << "Enter File ID: ";
            std::cin >> fileID;

            //std::cout << "clients:  ";
            // for (size_t i = 0; i < ctx.clients.size(); i++) {
            //     //std::cout << ctx.clients[i] << "\t";
            // }
            std::cout << std::endl;
            for (const auto& elementInMap : fileIdMap) {
                if (elementInMap.second == fileID) { 
                    //std::cout << elementInMap.first.substr(elementInMap.first.find("|") + 1) << "       ";
                    
                    for (int presence : filePresence[elementInMap.first]) {
                        //std::cout << presence << "\t";
                        noOfClientsWithFile += presence;
                    }
                    std::cout << "Download for File: [" << fileID << "] " << elementInMap.first.substr(elementInMap.first.find("|") + 1) << " is already started." << std::endl; 
                    std::cout << "Found " << noOfClientsWithFile << " Client(s) with the file." << endl;
                    std::cout << std::endl;

                    //std::cout << "File size in bytes: " << fileSizeMap[elementInMap.first] << std::endl;
                    numberOfChunks = fileSizeMap[elementInMap.first]/32;
                    //std::cout << "Number of 32 byte packets: " << numberOfChunks << std::endl;

                    //int assignedChunksPerClient[noOfClientsWithFile][(numberOfChunks/noOfClientsWithFile)];
                    vector<int> values;
                    for (int i = 0; i <= numberOfChunks; ++i) {
                        values.push_back(i);
                    }

                    std::random_device rd;
                    std::mt19937 g(rd());
                    std::shuffle(values.begin(), values.end(), g);

                    vector<vector<int>> groups(noOfClientsWithFile);
                    for (size_t i = 0; i < values.size(); ++i) {
                        groups[i % noOfClientsWithFile].push_back(values[i]);
                    }

                    for (int i = 0; i < noOfClientsWithFile; ++i) {
                        std::cout << "Group " << i + 1 << ": ";
                        for (int val : groups[i]) {
                            std::cout << val << " ";
                        }
                        std::cout << std::endl;
                    }

                    pthread_mutex_lock(&ctx.lock);
                    int groupIndex = 0;
                    for (size_t i = 0; i < ctx.clients.size(); ++i) {
                        int sock = ctx.clients[i];
                        if (filePresence[elementInMap.first][i] == 1) {
                            std::ostringstream oss;
                            oss << fileID << "|";
                            for (int chunk : groups[groupIndex]) {
                                oss << chunk << ",";
                            }
                            std::string message = oss.str();
                            message.pop_back(); // remove trailing comma

                            send(sock, message.c_str(), message.size(), 0);
                            groupIndex++;
                        }
                    }
                    pthread_mutex_unlock(&ctx.lock);

                    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
                    std::cout << "Searching for files.. ";
                    while (true) {
                        pthread_mutex_lock(&ctx.lock);
                        size_t gotResponses = ctx.clientData.size();
                        size_t expected = ctx.clients.size();
                        pthread_mutex_unlock(&ctx.lock);

                        if (gotResponses >= expected) break;
                        if (std::chrono::steady_clock::now() > deadline) break;

                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                    std::cout << "done." << std::endl;

                    break;
                }
            }
            //pthread to receive download files
            //receive chunks <file ID>, <chunk number>, <32 byte data>
            //count number of chunks received
            //write out the chunks

        }
        else if (userInput == 3) {
            //reference number of packets received from 2
            //multiply by 32 bytes
            //print
        }
        else if (userInput == 4) {
            std::cout << "Closing server..." << std::endl;
            pthread_mutex_lock(&ctx.lock);
            for (int sock : ctx.clients) {
                close(sock);
            }

            pthread_mutex_unlock(&ctx.lock);
            close(ctx.server_fd);
            break;
        }
    }

    pthread_cancel(serverThread);
    pthread_mutex_destroy(&ctx.lock);

    return 0;
}