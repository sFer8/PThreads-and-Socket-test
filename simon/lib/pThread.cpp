#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <functional>
#include <iostream>
#include "pThread.h"

// C++ compiler: g++ -lpthread pThreadsClass.cpp
// Run: ./a.out

typedef void*(* functionParameter)(void *);

using namespace std;


pthread_t thread;
int  iret;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


void pThread::createPThread(functionParameter func, const char *message)
{
    iret = pthread_create( &thread, NULL, func, (void*) message);

}

void pThread::joinThread() 
{
    pthread_join( thread, NULL);
}

void pThread::printIret() 
{
    printf("Thread returns: %d\n",iret);
}

void pThread::lockMutex() 
{
    pthread_mutex_lock( &mutex );
}

void pThread::unlockMutex()
{
    pthread_mutex_unlock( &mutex );
}