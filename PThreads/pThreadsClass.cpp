#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <functional>
#include <iostream>

// C++ compiler: g++ -lpthread pThreadsClass.cpp
// Run: ./a.out

void *functionC();
void *print_message_function( void *ptr);
typedef void*(* functionParameter)(void *);
typedef void*(* functionParameter2)(pthread_t arg);

int  counter = 0;

using namespace std;

class pThread 
{
    private:
        pthread_t thread;
        int  iret;
        pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
        

        void joinThread() 
        {
            pthread_join( thread, NULL);
        }

    public:
        void createPThread(functionParameter func, const char *message)
        {
            iret = pthread_create( &thread, NULL, func, (void*) message);
            joinThread();
        }
        void createPThread(functionParameter2 func, const char *message)
        {
            iret = pthread_create( &thread, NULL, func, (void*) message);
            joinThread();
        }

        void printIret() 
        {
            printf("Thread returns: %d\n",iret);
        }

        // void lockMutex() 
        // {
        //     pthread_mutex_lock( &mutex );
        // }

        // void unlockMutex()
        // {
        //     pthread_mutex_unlock( &mutex );
        // }

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

    myThread.printIret();
    myThread1.printIret();


    // int rc1, rc2;

    // pThread myThread2;
    // pThread myThread3;

    // myThread2.createPThread((functionParameter)functionC, NULL);
    // myThread3.createPThread((functionParameter)functionC, NULL);
    
     return(0);
}

void *print_message_function( void *ptr)
{
     char *message;
     message = (char *) ptr;
     printf("%s \n", message);
     return ptr;
}

// void *functionC(pThread obj)
// {
//    obj.lockMutex();
//    counter++;
//    printf("Counter value: %d\n",counter);
//    obj.unlockMutex();
//    return 0;
// }