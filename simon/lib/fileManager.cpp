#include "fileManager.h"
#include <iostream>
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

