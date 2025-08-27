#include "fileManager.h"
#include <iostream>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <experimental/filesystem>
#include <iomanip>

using namespace std;
using namespace experimental::filesystem;

FILE *fptr;
char charFromFile[1000];
//string filePath = "./DummyFiles/1";

void myFile::writeToFile(const char* fileName, const char* message) {
    fptr = fopen(fileName, "wb");
    fprintf(fptr, message);
    fclose(fptr);
}

// void myFile::appendToFile(const char* fileName, const char* message) {
//     fptr = fopen(fileName, "ab");
//     fprintf(fptr, message);
//     fclose(fptr);
// }

char* myFile::readFromFile(const char* fileName) {
    fptr = fopen(fileName, "rb");
    fgets(charFromFile, 1000, fptr);
    fclose(fptr);
    return charFromFile;
}

vector<string> myFile::getFileList(string filePath) {
    vector<string> listOfNames;
    for (const auto& entry : directory_iterator(filePath)) {
        listOfNames.push_back(entry.path().filename().string());
    }
    return listOfNames;
}

vector<vector<string>> myFile::getIDFileList(string filePath) {
    vector<vector<string>> listOfIDNames;

    for (const auto& entry : directory_iterator(filePath)) {
        if (is_directory(entry)) {
            string folderName = entry.path().filename().string();

            for (const auto& fileEntry : directory_iterator(entry.path())) {
                if (is_regular_file(fileEntry)) {
                    string fileName = fileEntry.path().filename().string();

                    if (fileName.find(":Zone.Identifier") != string::npos) {
                        continue;
                    }
                    
                    listOfIDNames.push_back({folderName, fileName, to_string(file_size(fileEntry))});
                }
            }
        }
    }

    return listOfIDNames;
}

int myFile::checkFolderForFiles(const string& folderPath, int& result) {
    for (const auto& entry : directory_iterator(folderPath)) {
        if (is_regular_file(entry)) {
            result = 1;
            return 0;
        } else if (is_directory(entry)) {
            checkFolderForFiles(entry.path().string(), result);
            if (result == 1) return 0;
        }
        return 0;
    }
    return 1;
}

std::vector<char> myFile::getChunk(const char* basePath, int fileID, int chunkNumber) {
    std::vector<char> dataChunk;
    std::string folderPath = std::string(basePath) + "/" + std::to_string(fileID);
    std::vector<std::string> files = getFileList(folderPath);
    std::string targetFilePath = folderPath + "/" + files[0];

    // std::string targetFilePath;
    // for (const auto& f : files) {
    //     if (f.find(":Zone.Identifier") != std::string::npos) {
    //         continue; // skip system stream
    //     }
    //     if (!f.empty() && f[0] == '.') {
    //         continue; // skip hidden files like .DS_Store
    //     }
    //     targetFilePath = folderPath + "/" + f;
    //     break;
    // }

    // if (targetFilePath.empty()) {
    //     std::cerr << "Error: No valid file found in " << folderPath << std::endl;
    //     return dataChunk;
    // }

    FILE* fptr = fopen(targetFilePath.c_str(), "rb");
    if (!fptr) {
        std::cerr << "Error: Unable to open file " << targetFilePath << std::endl;
        return dataChunk;
    }

    constexpr size_t CHUNK_SIZE = 32;
    long offset = chunkNumber * CHUNK_SIZE;
    if (fseek(fptr, offset, SEEK_SET) != 0) {
        std::cerr << "Error: Unable to seek to offset " << offset << std::endl;
        fclose(fptr);
        return dataChunk;
    }

    dataChunk.resize(CHUNK_SIZE);
    size_t bytesRead = fread(dataChunk.data(), 1, CHUNK_SIZE, fptr);
    dataChunk.resize(bytesRead); // shrink if last chunk is smaller

    fclose(fptr);
    return dataChunk;
}

void myFile::appendToFile(const char* fileName, const char* data, size_t dataSize, const char* basePath, int fileID, int chunkNumber) {
    std::string folderPath = std::string(basePath) + "/" + std::to_string(fileID);
    std::string targetFilePath = folderPath + "/" + fileName;
    create_directories(folderPath); // C++11 experimental::filesystem call

    // Open for update (no truncate). If doesn’t exist, create it.
    FILE* fptr = fopen(targetFilePath.c_str(), "r+b");
    if (!fptr) {
        fptr = fopen(targetFilePath.c_str(), "w+b");
        if (!fptr) {
            std::cerr << "[ERROR] Unable to open file " << targetFilePath << std::endl;
            return;
        }
    }

    constexpr long CHUNK_SIZE = 32;
    long offset = chunkNumber * CHUNK_SIZE;

    if (fseek(fptr, offset, SEEK_SET) != 0) {
        std::cerr << "[ERROR] Unable to seek to offset " << offset
                  << " in file " << targetFilePath << std::endl;
        fclose(fptr);
        return;
    }

    size_t written = fwrite(data, 1, dataSize, fptr);
    if (written != dataSize) {
        std::cerr << "[ERROR] fwrite wrote " << written 
                  << " bytes (expected " << dataSize << ") in " 
                  << targetFilePath << std::endl;
    }

    // Debug logging
    std::cerr << "[DEBUG] appendToFile: fileID=" << fileID
              << " chunk=" << chunkNumber
              << " dataSize=" << dataSize
              << " offset=" << offset << std::endl;

    for (size_t i = 0; i < dataSize; i++) {
        unsigned char c = (unsigned char)data[i];
        std::cerr << std::hex << std::setw(2) << std::setfill('0') 
                  << (int)c << " ";
    }
    std::cerr << std::dec << std::setfill(' ') << std::endl; // reset flags

    fclose(fptr);
}


