#include <stdio.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <experimental/filesystem>

using namespace std;
using namespace experimental::filesystem;

class myFile {
    private:
        FILE *fptr;
        char charFromFile[1000];
        string path = "./DummyFiles/1";

    public:
        void writeToFile(const char* fileName, const char* message){
            fptr = fopen(fileName, "wb");
            fprintf(fptr, message);
            fclose(fptr);
        }

        void appendToFile(const char* fileName, const char* message){
            fptr = fopen(fileName, "ab");
            fprintf(fptr, message);
            fclose(fptr);
        }

        char* readFromFile(const char* fileName) {
            fptr = fopen(fileName, "rb");
            fgets(charFromFile, 1000, fptr);
            fclose(fptr);

            return charFromFile;  
        }

        
        vector<string> getFileList() {
            vector<string> listOfNames;

            for (const auto& entry : directory_iterator(path)) {
                listOfNames.push_back(entry.path().string());
            }

            return listOfNames;
        }

};


//g++ -std=c++11 cppFileClass.cpp -lstdc++fs -o main
int main() {
    /////////////////////////////////////////////
    //testing the reading, one line
    /////////////////////////////////////////////
    char fileName[] = "testTextFile.txt";  //the text file contains "Some text"
    char* file = fileName;

    myFile testRead;
    char* lineFromFile = {testRead.readFromFile(file)};
    cout << lineFromFile << endl;

    /////////////////////////////////////////////
    //testing the writing and creating a file
    /////////////////////////////////////////////
    char writeFileName[] = "testWriteFile.txt";
    char* fileWrite = writeFileName;

    char writeFileMessage[] = "testing 1 2 3";
    char* message = writeFileMessage;

    myFile testWrite;
    testWrite.writeToFile(fileWrite, message);

    /////////////////////////////////////////////
    //testing the appending to the created file
    /////////////////////////////////////////////
    char appendFileName[] = "testWriteFile.txt";
    char* fileAppend = appendFileName;

    char appendFileMessage[] = "\nappended new line";
    char* message1 = appendFileMessage;

    myFile testAppend;
    testAppend.appendToFile(fileAppend, message1);

    /////////////////////////////////////////////
    //testing the listing of file names
    /////////////////////////////////////////////
    myFile testListFiles;
    vector<string> listOfFiles = testListFiles.getFileList();

    for (const string& str : listOfFiles) {
        cout << str << endl;
    } 
}

    