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
#include <iomanip>
#include <errno.h>

#include "./lib/fileManager.h"
#include "./lib/hostDataTransfer.h"
#include "./lib/clientDataTransfer.h"
#include "./lib/pThread.h"

//g++ -std=c++11 -lpthread seed.cpp ./lib/fileManager.cpp ./lib/hostDataTransfer.cpp ./lib/clientDataTransfer.cpp ./lib/pThread.cpp -lstdc++fs -o seed
//g++ -std=c++11 seed.cpp ./lib/fileManager.cpp ./lib/hostDataTransfer.cpp ./lib/clientDataTransfer.cpp ./lib/pThread.cpp -lstdc++fs -lpthread -o seed


struct ServerContext {
    int server_fd;
    vector<int> clients;
    pthread_mutex_t lock;
    std::map<int, vector<vector<string>>> clientData;

    // NEW: per-socket flag: only allow createListen() to read when true
    std::map<int, bool> expectingList;
};

ServerContext ctx;

#pragma pack(push, 1)
struct ChunkPacketHeader {
    int fileID;
    int chunkNumber;
    int dataSize;
};
#pragma pack(pop)

ssize_t recvAll(int sock, void* buffer, size_t length) {
    size_t total = 0;
    char* buf = static_cast<char*>(buffer);
    while (total < length) {
        ssize_t bytes = recv(sock, buf + total, length - total, 0);
        if (bytes > 0) {
            total += bytes;
            continue;
        }
        if (bytes == 0) {
            // peer closed connection
            return 0;
        }
        // bytes < 0: error
        if (errno == EINTR) {
            continue; // interrupted, retry
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // temporary, wait a little and retry
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        // unrecoverable error
        return -1;
    }
    return static_cast<ssize_t>(total);
}

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
        bool shouldRead = false;
        pthread_mutex_lock(&ctx.lock);
        auto it = ctx.expectingList.find(clientSocket);
        if (it != ctx.expectingList.end()) shouldRead = it->second;
        pthread_mutex_unlock(&ctx.lock);

        if (!shouldRead) {
            // Not expecting a file-list reply right now -> don't read because
            // main thread might be doing the data transfer. Sleep briefly.
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        // Only read when expecting the listing
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if(bytesReceived <= 0) {
            // keep waiting while expectingList is true; on error, break out
            if (bytesReceived == 0) {
                // peer closed
                break;
            }
            // bytesReceived < 0: temporary error, continue
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        buffer[bytesReceived] = '\0';
        std::string receivedData(buffer);

        auto vec2D = deserializeVector(receivedData);

        pthread_mutex_lock(&ctx.lock);
        ctx.clientData[clientSocket] = vec2D;
        pthread_mutex_unlock(&ctx.lock);

        // After we got a reply, set expectingList false so createListen stops reading
        pthread_mutex_lock(&ctx.lock);
        ctx.expectingList[clientSocket] = false;
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

#define SIZEOF_ARR(arr) sizeof(arr) / sizeof(arr[0])
// #define SAFE_DELETE(p) {    \
//     if(p! != Null) {    \
//         free(p);    \
//     }   \
// } 
//SAFE_DELETE(ctx);

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
    //change to for loop
    for(int i = 0; i < SIZEOF_ARR(PORTS); i++) {
        address.sin_port = htons(PORTS[i]);
        if (bind(ctx->server_fd, (struct sockaddr*)&address, sizeof(address)) == 0) {
            //std::cout << PORTS[i] << std::endl;
            std::cout << "Listening at port " << PORTS[i] << std::endl;
            
            listen(ctx->server_fd, 10);
            break;
        }
    }

    // int i = 0;
    // while (true) {
    //     address.sin_port = htons(PORTS[i]);
    //     if (bind(ctx->server_fd, (struct sockaddr*)&address, sizeof(address)) == 0) {
    //         std::cout << PORTS[i] << std::endl;
            
    //         listen(ctx->server_fd, 10);
    //         break;
    //     }
    //     i++;
    //     if (i >= (sizeof(PORTS) / sizeof(PORTS[0]))) {
    //         std::cout << "No ports available.\n";
    //         return nullptr;
    //     }
    // }

    while (true) {
        int client_fd = accept(ctx->server_fd, (struct sockaddr*)&address, &addrlen);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        pthread_mutex_lock(&ctx->lock);
        ctx->clients.push_back(client_fd);
        ctx->expectingList[client_fd] = false;
        pthread_mutex_unlock(&ctx->lock);


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
        if(i >= SIZEOF_ARR(PORTS)) {
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
    string filePath = "./DummyFiles/downloads";

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
                ctx.expectingList[sock] = true;       // allow createListen to read reply
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

            pthread_mutex_lock(&ctx.lock);
            for (int sock : ctx.clients) {
                ctx.expectingList[sock] = false;
            }
            pthread_mutex_unlock(&ctx.lock);

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

                    constexpr int CHUNK = 32;
                    numberOfChunks = (fileSizeMap[elementInMap.first] + CHUNK - 1) / CHUNK;
                    vector<int> values;
                    for (int i = 0; i < numberOfChunks; ++i) {
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

                    // Build the list of sockets that will actually send data for this file:
                    std::vector<int> senders;
                    for (size_t i = 0; i < ctx.clients.size(); ++i) {
                        if (filePresence[elementInMap.first][i] == 1) {
                            senders.push_back(ctx.clients[i]);
                        }
                    }

                    // Short recv timeout so we don't block forever on one socket
                    timeval tv;
                    tv.tv_sec = 0;
                    tv.tv_usec = 200000; // 200ms
                    for (int s : senders) {
                        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                    }


                    // Use the real filename from the listing instead of hardcoding reconstructed.txt
                    std::string realName = elementInMap.first.substr(elementInMap.first.find("|") + 1);

                    std::set<int> done;                    // sockets that sent the end marker
                    size_t remaining = senders.size();

                    while (remaining > 0) {
                        // loop senders in a snapshot (we remove failed senders via 'done')
                        for (int sock : senders) {
                            if (done.count(sock)) continue;

                            // Read header (network order)
                            ChunkPacketHeader headerNet;
                            ssize_t hbytes = recvAll(sock, &headerNet, sizeof(headerNet));
                            if (hbytes <= 0) {
                                std::cerr << "[ERROR] Socket " << sock << " closed or failed while reading header (recvAll returned " << hbytes << "). Dropping sender.\n";
                                done.insert(sock);
                                if (remaining > 0) --remaining;
                                continue; // skip this socket
                            }

                            // Convert header fields safely using uint32_t -> int32_t
                            uint32_t net_fileID      = static_cast<uint32_t>(headerNet.fileID);
                            uint32_t net_chunkNumber = static_cast<uint32_t>(headerNet.chunkNumber);
                            uint32_t net_dataSize    = static_cast<uint32_t>(headerNet.dataSize);

                            int32_t fileID      = static_cast<int32_t>(ntohl(net_fileID));
                            int32_t chunkNumber = static_cast<int32_t>(ntohl(net_chunkNumber));
                            int32_t dataSize    = static_cast<int32_t>(ntohl(net_dataSize));

                            // Basic sanity checks
                            if (dataSize < 0 || dataSize > 10 * 1024 * 1024) {
                                std::cerr << "[ERROR] Invalid header from socket " << sock
                                        << " fileID=" << fileID
                                        << " chunk=" << chunkNumber
                                        << " dataSize=" << dataSize << ". Dropping sender.\n";
                                done.insert(sock);
                                if (remaining > 0) --remaining;
                                continue;
                            }

                            // End-of-transmission marker
                            if (chunkNumber == -1 && dataSize == 0) {
                                std::cout << "End of transmission from client socket " << sock
                                        << " for FileID=" << fileID << std::endl;
                                done.insert(sock);
                                if (remaining > 0) --remaining;
                                continue;
                            }

                            // Receive the payload exactly dataSize bytes
                            // Receive the payload exactly dataSize bytes
                            // Receive the payload exactly dataSize bytes
                            std::vector<char> chunkData(dataSize);
                            ssize_t totalReceived = recvAll(sock, chunkData.data(), dataSize);
                            if (totalReceived != dataSize) {
                                std::cerr << "[ERROR] Socket " << sock
                                        << " lost connection while receiving chunk "
                                        << chunkNumber << " (got "
                                        << totalReceived << "/" << dataSize << ")\n";
                                done.insert(sock);
                                if (remaining > 0) --remaining;
                                continue;
                            }

                            // At this point we have the full chunk, write it
                            file.appendToFile(realName.c_str(),
                                            chunkData.data(),
                                            static_cast<size_t>(totalReceived),
                                            filePath.c_str(),
                                            fileID,
                                            chunkNumber);

                            std::cerr << "[RECV] fileID=" << fileID
                                    << " chunk=" << chunkNumber
                                    << " size=" << totalReceived
                                    << " offset=" << (chunkNumber * 32)
                                    << std::endl;

                            // Send ACK back so client knows it can send next chunk (client waits for ACK)
                            const char* ack = "ACK";
                            ssize_t sret = send(sock, ack, strlen(ack), 0);
                            if (sret <= 0) {
                                std::cerr << "[WARN] Failed to send ACK to socket " << sock << " (send returned " << sret << "). Marking sender done.\n";
                                done.insert(sock);
                                if (remaining > 0) --remaining;
                            }
                        }
                    }
                    break;
                }
            }
        }
        else if (userInput == 3) {
            //reference number of packets received from 2

            int placeHolderForPackets = 225;
            int placeHolderForTotalPackets = 311;
            std::cout << "Download status:" << std::endl;
            std::cout << "fileName\t" << placeHolderForPackets*32 << "/" << placeHolderForTotalPackets*32 << std::endl; 
            
            
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