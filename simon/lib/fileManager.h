#ifndef fileManager_H
#define fileManager_H

#include <iostream>
#include <string>
#include <sys/stat.h>
#include <experimental/filesystem>

using namespace std;
using namespace experimental::filesystem;

class myFile{
    private:
        FILE *fptr;
        char charFromFile[1000];

    public:
        void writeToFile(const char* fileName, const char* message);
        //void appendToFile(const char* fileName, const char* message);
        char* readFromFile(const char* fileName);
        vector<string> getFileList(string filePath);
        vector<vector<string>> getIDFileList(string filePath);
        int checkFolderForFiles(const string& folderPath, int& result);
        std::vector<char> getChunk(const char* basePath, int fileID, int chunkNumber);
        void appendToFile(const char* fileName, const char* data, size_t dataSize, const char* basePath, int fileID, int chunkNumber);
};

#endif