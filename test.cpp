#include <iostream>

//<Return type> FunctionName (<argument list>);

void printHello(Void);

char c;     //1 byte. 8 bits
short s;    //2  bytes
int i;      //4 bytes
long l;     // 32 bit system - 4 bytes, 64 bit system - 8 bytes.
float f;    //
double d;   //

unsigned char uc;

void RomanToInteger(const char* roman) {
    int sz = sizeof(roman);  // will return 8 bytes
    int sz - strlen(roman); // will return size
}

int main(void) {
    int x = 1;
    if (x) {
        //true
    }
    else{

    }

    while(x) {
        x--;
    }
    x--;
    --x;

    while (--x) {
        cout << "while" << endl;

    }

    switch(x) {
        case 5:
            cout << "while" << endl;
            break;

        case 1:
        case 4:
            cout << "maybe" << endl;
            break;

        default:
            break;

    }

typedef int MYINT;

typedef struct data{
    char name[50];
    char address[100];
} Person;

    #define year (2025)

int main(void) {
    strict data personl;
    Person person2;

}


    int x = 0;
    int* px = &x;
    int** ppx = &px;
    int*** ppx = &ppx;

    printf("Address of x = %p\n", &x);
    printf("Address of px = %p\n", &px);
    
    printf("Value inside px = %d\n", *px);

    print("Value inside pppx = %p\n", pppx);

    printf("Value inside x = %d\n", ***pppx);

}

#ifndef __CPP_TUTORIAL_H__
#define __CPP_TUTORIAL_H__

void func1();
void func2();

#endif