#ifndef pThread_H
#define pThread_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <functional>
#include <iostream>

typedef void*(* functionParameter)(void *);

using namespace std;

class pThread 
{
    private:
        pthread_t thread;
        int  iret;
        pthread_mutex_t mutex;

    public:
        void createPThread(functionParameter func, const char *message);
        void joinThread();
        void printIret();
        void lockMutex();
        void unlockMutex();
};

#endif