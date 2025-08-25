#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <functional>
#include <iostream>

// C++ compiler: g++ -lpthread pThreadsClass.cpp
// Run: ./a.out

void *print_message_function( void *ptr);
typedef void*(* functionParameter)(void *);

int  counter = 0;

using namespace std;

class pThread 
{
    private:
        pthread_t thread;
        int  iret;
        pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    public:
        void createPThread(functionParameter func, const char *message)
        {
            iret = pthread_create( &thread, NULL, func, (void*) message);

        }

        void joinThread() 
        {
            pthread_join( thread, NULL);
        }

        void printIret() 
        {
            printf("Thread returns: %d\n",iret);
        }

        void lockMutex() 
        {
            pthread_mutex_lock( &mutex );
        }

        void unlockMutex()
        {
            pthread_mutex_unlock( &mutex );
        }

};

int main()
{
    char message[] = "This is the way";
    char* testMessage = message;

    char message1[] = "(1)";
    char* testMessage1 = message1;

    pThread myThread;
    pThread myThread1;

    myThread.createPThread((functionParameter)print_message_function ,testMessage);
    myThread1.createPThread((functionParameter)print_message_function, testMessage1);

    myThread.joinThread();
    myThread1.joinThread();

    myThread.printIret();
    myThread1.printIret();

    pThread myThread2;
    pThread myThread3;

    myThread2.createPThread(print_message_function, testMessage);
    myThread3.createPThread(print_message_function, testMessage1);

    myThread2.joinThread();
    myThread3.joinThread();

    myThread2.lockMutex();
    myThread3.createPThread(print_message_function, testMessage1);
    myThread3.joinThread();
    myThread2.createPThread(print_message_function, testMessage);
    myThread2.joinThread();
    myThread2.unlockMutex();
    myThread2.createPThread(print_message_function, testMessage);
    myThread2.joinThread();
    

     return(0);
}

void *print_message_function( void *ptr)
{
     char *message;
     message = (char *) ptr;
     printf("%s \n", message);
     return ptr;
}