#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <errno.h>

//g++ -std=c++11 clientTest1.cpp ./lib/fileManager.cpp -lstdc++fs -o client1

#include "./lib/fileManager.h"

#pragma pack(push, 1)
struct ChunkPacketHeader {
    int fileID;
    int chunkNumber;
    int dataSize;
};
#pragma pack(pop)

ssize_t recvAll_client(int sock, void* buffer, size_t length) {
    size_t total = 0;
    char* buf = static_cast<char*>(buffer);
    while (total < length) {
        ssize_t bytes = recv(sock, buf + total, length - total, 0);
        if (bytes > 0) {
            total += bytes;
            continue;
        }
        if (bytes == 0) return 0;
        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        return -1;
    }
    return static_cast<ssize_t>(total);
}

void printFileList(vector<vector<string>> listOfIDNames);
string serialize2DVector(const vector<vector<string>>& vec);

int main()
{
    myFile file;
    string filePath = "./DummyFiles/seed1";

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    int PORTS[5] = {8080, 8081, 8082, 8083, 8084};

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;

    bool isNotConnected = 1;
    while(isNotConnected) {
        int x = 0;
        while(true) {
            serverAddress.sin_port = htons(PORTS[x]);
            serverAddress.sin_addr.s_addr = INADDR_ANY;

            if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == 0) {
                std::cout << "client is starting at port " << PORTS[x] << std::endl;
                isNotConnected = 0;
                break;
            } else {
                perror("connect");
            }
            x++;
            if(x >= (sizeof(PORTS) / sizeof(PORTS[0]))) {
                std::cout << "No port found" << std::endl;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    int userInput = 0;
    while(true) {
        char buffer[1024] = { 0 };
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if(bytesReceived <= 0) {
            continue;
        }
        else {
            buffer[bytesReceived] = '\0';
            std::cout << "Message from Server: " << buffer << std::endl;

            int userInput = atoi(buffer);

            if(userInput == 1) {
                std::cout << "sending out files" << std::endl;


                vector<vector<string>> listOfFiles = file.getIDFileList(filePath);
                printFileList(listOfFiles);

                string serialized = serialize2DVector(listOfFiles);
                send(clientSocket, serialized.c_str(), serialized.size(), 0);
            }
            if(userInput == 2) {
                char chunkAssignmentBuffer[2048] = { 0 };
                while(true) {
                    int chunkAssignmentReceived = recv(clientSocket, chunkAssignmentBuffer, sizeof(chunkAssignmentBuffer), 0);
                    if (chunkAssignmentReceived <= 0) {
                        continue;
                    }

                    chunkAssignmentBuffer[chunkAssignmentReceived] = '\0';
                    std::string assignmentStr(chunkAssignmentBuffer);

                    size_t sepPos = assignmentStr.find('|');
                    if (sepPos == std::string::npos) {
                        std::cerr << "Invalid format: " << assignmentStr << std::endl;
                        continue;
                    }

                    std::string idString = assignmentStr.substr(0, sepPos);
                    std::string chunksString = assignmentStr.substr(sepPos + 1);

                    int id = std::stoi(idString);

                    std::vector<int> chunks;
                    size_t pos = 0;
                    while ((pos = chunksString.find(',')) != std::string::npos) {
                        chunks.push_back(std::stoi(chunksString.substr(0, pos)));
                        chunksString.erase(0, pos + 1);
                    }
                    if (!chunksString.empty()) {
                        chunks.push_back(std::stoi(chunksString));
                    }

                    std::cout << "ID: " << id << std::endl;
                    std::cout << "Chunks: ";
                    for (int c : chunks) {
                        std::cout << c << " ";
                    }
                    std::cout << std::endl;

                    for (int c : chunks) {
                        std::vector<char> chunkData = file.getChunk(filePath.c_str(), id, c);

                        ChunkPacketHeader header;
                        header.fileID = htonl(id);
                        header.chunkNumber = htonl(c);
                        header.dataSize = htonl((int)chunkData.size());

                        auto sendAll = [](int sock, const void* buf, size_t len) -> bool {
                            size_t total = 0;
                            const char* p = static_cast<const char*>(buf);
                            while (total < len) {
                                ssize_t sent = send(sock, p + total, len - total, 0);
                                if (sent <= 0) return false;
                                total += sent;
                            }
                            return true;
                        };

                        if (!sendAll(clientSocket, &header, sizeof(header))) {
                            std::cerr << "[ERROR] Failed to send full header\n";
                            break;
                        }
                        if (!chunkData.empty() && !sendAll(clientSocket, chunkData.data(), chunkData.size())) {
                            std::cerr << "[ERROR] Failed to send full chunk\n";
                            break;
                        }

                        char ackBuf[4] = {0};
                        ssize_t ackRecv = recvAll_client(clientSocket, ackBuf, 3);
                        if (ackRecv == 3) {
                            ackBuf[3] = '\0';
                            if (std::string(ackBuf) == "ACK") {
                                std::cout << "[DEBUG] ACK received for chunk " << c << std::endl;
                            } else {
                                std::cerr << "[WARN] Got non-ACK '" << std::string(ackBuf, ackRecv) << "'\n";
                            }
                        } else if (ackRecv == 0) {
                            std::cerr << "[ERROR] Server closed connection while waiting for ACK\n";
                            break;
                        } else {
                            std::cerr << "[ERROR] recvAll_client for ACK returned " << ackRecv << " (errno=" << errno << ")\n";
                            break;
                        }
                    }

                    ChunkPacketHeader endHeader;
                    endHeader.fileID = htonl(id);
                    endHeader.chunkNumber = htonl(-1);
                    endHeader.dataSize = htonl(0);
                    send(clientSocket, &endHeader, sizeof(endHeader), 0);
                                            
                    break;
                }
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