#include <iostream>
#include <cstdio>
#include <cstring>

//g++ -std=c++11 -lstdc++fs -o fseek fseek.cpp
//./fseek

int main() {
    const char* filename = "../DummyFiles/seed/1/STERwars.txt"; // Replace with your file name
    FILE* file = fopen(filename, "rb");   // Open file in binary read mode

    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    rewind(file);


    long offset = 0;  // Offset in bytes
    size_t dataSize = 32; // Number of bytes to read
    char buffer[33] = {0}; // Buffer to store data
    int packet = 0;
    while(true) {
        memset(buffer, 0, sizeof(buffer));
        std::cout << "Size of files is " << fileSize << " or " << (fileSize/32) << " packets" <<  std::endl;
        std::cout << "Offset [" << (fileSize/32) << "]: ";

        std::cin >> packet;
        offset = packet * 32;

        if (fseek(file, offset, SEEK_SET) != 0) {
            std::cerr << "Error: Unable to seek to the specified offset!" << std::endl;
            fclose(file);
            return 1;
        }

        size_t bytesRead = fread(buffer, 1, dataSize, file);
        std::cout << "Data read: " << buffer << std::endl;
    }
    

    fclose(file);
    return 0;
}
