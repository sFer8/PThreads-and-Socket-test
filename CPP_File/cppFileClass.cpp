#include <stdio.h>
#include <iostream>
#include <cstring>

using namespace std;

class myFile {
    private:
        FILE *fptr;
        char charFromFile[1000];

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
};

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

}