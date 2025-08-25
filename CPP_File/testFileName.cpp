#include <iostream>
#include <experimental/filesystem>

using namespace std;
using namespace experimental::filesystem;

int main() {
    string path = ".";

    try {
        for (const auto& entry : directory_iterator(path)) {
            cout << entry.path().string() << endl;
        }
    } catch (const filesystem_error& e) {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}

//g++ -std=c++11 testFileName.cpp -lstdc++fs -o main
