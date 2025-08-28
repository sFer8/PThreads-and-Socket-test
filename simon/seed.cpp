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
#include <sstream>

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
    std::map<int, bool> expectingList;
};

struct DownloadArgs {
    std::string fileID;
    ServerContext* ctx;
    std::map<std::string, std::string>* fileIdMap;
    std::map<std::string, std::vector<int>>* filePresence;
    std::map<std::string, int>* fileSizeMap;
    std::vector<std::vector<std::string>>* downloadStatus;
    std::string filePath;
    myFile* file;
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
            return 0;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        return -1;
    }
    return static_cast<ssize_t>(total);
}

ssize_t sendAll(int sock, const void* buf, size_t len) {
    size_t total = 0;
    const char* p = static_cast<const char*>(buf);
    while (total < len) {
        ssize_t n = send(sock, p + total, len - total, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return static_cast<ssize_t>(total);
        total += static_cast<size_t>(n);
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
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if(bytesReceived <= 0) {
            if (bytesReceived == 0) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        buffer[bytesReceived] = '\0';
        std::string receivedData(buffer);

        auto vec2D = deserializeVector(receivedData);

        pthread_mutex_lock(&ctx.lock);
        ctx.clientData[clientSocket] = vec2D;
        pthread_mutex_unlock(&ctx.lock);

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
    for(int i = 0; i < SIZEOF_ARR(PORTS); i++) {
        address.sin_port = htons(PORTS[i]);
        if (bind(ctx->server_fd, (struct sockaddr*)&address, sizeof(address)) == 0) {
            std::cout << PORTS[i] << std::endl;
            std::cout << "Listening at port " << PORTS[i] << std::endl;

            listen(ctx->server_fd, 10);
            break;
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

void* downloadFileThread(void* arg) {
    DownloadArgs* args = static_cast<DownloadArgs*>(arg);
    std::string fileID = args->fileID;
    auto& fileIdMap = *args->fileIdMap;
    auto& filePresence = *args->filePresence;
    auto& fileSizeMap = *args->fileSizeMap;
    auto& downloadStatus = *args->downloadStatus;
    std::string filePath = args->filePath;
    myFile& file = *args->file;

    int noOfClientsWithFile = 0;
    constexpr int CHUNK = 32;

    for (const auto& elementInMap : fileIdMap) {
        if (elementInMap.second == fileID) {
            for (int presence : filePresence[elementInMap.first]) {
                noOfClientsWithFile += presence;
            }

            std::cout << "Download for File: [" << fileID << "] "
                      << elementInMap.first.substr(elementInMap.first.find("|") + 1)
                      << " is already started.\nFound " << noOfClientsWithFile
                      << " Client(s) with the file.\n\n";

            int numberOfChunks = (fileSizeMap[elementInMap.first] + CHUNK - 1) / CHUNK;
            std::vector<int> values(numberOfChunks);
            std::iota(values.begin(), values.end(), 0);
            std::shuffle(values.begin(), values.end(), std::mt19937{std::random_device{}()});

            std::vector<std::vector<int>> groups(noOfClientsWithFile);
            for (size_t i = 0; i < values.size(); ++i) {
                groups[i % noOfClientsWithFile].push_back(values[i]);
            }

            pthread_mutex_lock(&ctx.lock);
            std::vector<int> eligibleClients;
            for (size_t i = 0; i < ctx.clients.size(); ++i) {
                if (filePresence[elementInMap.first][i] == 1) {
                    eligibleClients.push_back(ctx.clients[i]);
                }
            }

            for (size_t i = 0; i < eligibleClients.size(); ++i) {
                std::ostringstream oss;
                oss << fileID << "|";
                for (int chunk : groups[i]) {
                    oss << chunk << ",";
                }
                std::string message = oss.str();
                if (!groups[i].empty()) {
                    message.pop_back();
                }
                std::cout << "[DEBUG] Sending assignment to client " << eligibleClients[i] << ": " << message << std::endl;

                ssize_t sent = sendAll(eligibleClients[i], message.c_str(), message.size());
                if (sent != (ssize_t)message.size()) {
                    std::cerr << "[ERROR] sendAll assignment to " << eligibleClients[i] << " returned " << sent << " errno=" << errno << std::endl;
                }
            }
            pthread_mutex_unlock(&ctx.lock);

            std::vector<int> senders;
            for (size_t i = 0; i < ctx.clients.size(); ++i) {
                if (filePresence[elementInMap.first][i] == 1) {
                    senders.push_back(ctx.clients[i]);
                }
            }

            timeval tv{0, 200000};
            for (int s : senders) {
                setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            }

            std::string realName = elementInMap.first.substr(elementInMap.first.find("|") + 1);
            downloadStatus.push_back({realName, "0"});

            std::set<int> done;
            size_t remaining = senders.size();
            while (remaining > 0) {
                for (int sock : senders) {
                    if (done.count(sock)) continue;

                    ChunkPacketHeader headerNet;
                    ssize_t hbytes = recvAll(sock, &headerNet, sizeof(headerNet));
                    if (hbytes <= 0) {
                        done.insert(sock);
                        --remaining;
                        continue;
                    }

                    int32_t fileID = ntohl(headerNet.fileID);
                    int32_t chunkNumber = ntohl(headerNet.chunkNumber);
                    int32_t dataSize = ntohl(headerNet.dataSize);

                    if (chunkNumber == -1 && dataSize == 0) {
                        done.insert(sock);
                        --remaining;
                        continue;
                    }

                    std::vector<char> chunkData(dataSize);
                    ssize_t totalReceived = recvAll(sock, chunkData.data(), dataSize);
                    if (totalReceived != dataSize) {
                        done.insert(sock);
                        --remaining;
                        continue;
                    }

                    file.appendToFile(realName.c_str(), chunkData.data(), totalReceived, filePath.c_str(), fileID, chunkNumber);

                    for (auto& row : downloadStatus) {
                        if (!row.empty() && row[0] == realName) {
                            if (row.size() < 2) row.resize(2);
                            row[1] = std::to_string(1 + std::stoi(row[1]));
                            break;
                        }
                    }

                    const char* ack = "ACK";
                    ssize_t ackSent = sendAll(sock, ack, 3);
                    if (ackSent != 3) {
                        std::cerr << "[WARN] Failed to send full ACK to " << sock << " (sent=" << ackSent << ", errno=" << errno << "). Marking sender done.\n";
                        done.insert(sock);
                        if (remaining > 0) --remaining;
                    } else {
                        // std::cout << "[DEBUG] Sent ACK to " << sock << std::endl;
                    }
                }
            }
            break;
        }
    }

    delete args;
    return nullptr;
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

    std::vector<std::vector<std::string>> downloadStatus;
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
                ctx.expectingList[sock] = true;
                send(sock, cmd, strlen(cmd), 0);
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
            std::string fileID;
            const char* cmd = "2";

            pthread_mutex_lock(&ctx.lock);
            ctx.clientData.clear();
            for (int sock : ctx.clients) {
                send(sock, cmd, strlen(cmd), 0);
            }
            pthread_mutex_unlock(&ctx.lock);

            std::cout << "Enter File ID: ";
            std::cin >> fileID;

            auto* args = new DownloadArgs{
                fileID, &ctx, &fileIdMap, &filePresence, &fileSizeMap,
                &downloadStatus, filePath, &file
            };

            pthread_t threadID;
            pthread_create(&threadID, nullptr, downloadFileThread, args);
            pthread_detach(threadID);
        }
        else if (userInput == 3) {
            std::cout << "\nDownload status:" << std::endl;
            int i = 1;
            for (const auto& row : downloadStatus) {
                std::cout << "[" << i << "]" << row[0] << "  " << (atoi(row[1].c_str())*32) << " kb/" << "fileSize" << " kb" << std::endl;
                i++;
            }
        }
        else if (userInput == 4) {
            const char* cmd = "4";

            pthread_mutex_lock(&ctx.lock);
            ctx.clientData.clear();
            pthread_mutex_unlock(&ctx.lock);

            pthread_mutex_lock(&ctx.lock);
            for (int sock : ctx.clients) {
                send(sock, cmd, strlen(cmd), 0);
            }
            pthread_mutex_unlock(&ctx.lock);

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