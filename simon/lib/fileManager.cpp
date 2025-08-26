#include "fileManager.h"
#include <iostream>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <experimental/filesystem>

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

void myFile::appendToFile(const char* fileName, const char* message) {
    fptr = fopen(fileName, "ab");
    fprintf(fptr, message);
    fclose(fptr);
}

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

vector<string> myFile::getChunk(const char*  filePath, int chunkNumber) {
    vector<string> dataChunk;
    fptr = fopen(filePath, "rb");

    fseek(fptr, 0, SEEK_END);
    int fileSize = ftell(fptr);
    rewind(fptr);


    long offset = chunkNumber * 32;
    size_t dataSize = 32;
    char buffer[33] = {0};

    memset(buffer, 0, sizeof(buffer));

    if (fseek(fptr, offset, SEEK_SET) != 0) {
        std::cerr << "Error: Unable to seek to the specified offset!" << std::endl;
        fclose(fptr);
    }

    size_t bytesRead = fread(buffer, 1, dataSize, fptr);
    dataChunk.push_back(to_string(chunkNumber));
    dataChunk.push_back({buffer});
    

    fclose(fptr);
    return dataChunk;
}

