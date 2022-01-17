#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>


int mails = 0;
pthread_mutex_t mutex;

void* routine() {
    for(int i = 0; i < 1000000; i++) {
        pthread_mutex_lock(&mutex);
        mails++;  // critical section  
        pthread_mutex_unlock(&mutex);

        // a thread then another,  block a section a code until the current thread 
        // finishes its execution 

        // execution steps bellow 

        // read mails
        // increment mails
        // write mails to memory
        
    }

}

int main(int argc, char* argv[]) {
    
    // 4 different threads 
    // it is known that threads share the same address block ? |
    // heap ( global and dinamically allocated variables) are shared between all of them 
    
    pthread_t p1, p2, p3, p4;

    // initializes the mutex, allocates memory
    pthread_mutex_init(&mutex, NULL);
    if (pthread_create(&p1, NULL, &routine, NULL) != 0) { // check for error
        return 1;
    }
    if (pthread_create(&p2, NULL, &routine, NULL) != 0) {  // check for error
        return 2;
    }
    if (pthread_create(&p3, NULL, &routine, NULL) != 0) {  // check for error
    }
    if (pthread_create(&p4, NULL, &routine, NULL) != 0) {  // check for error
        return 4;
    }
    if (pthread_join(p1, NULL) != 0) {  // check for error
        return 5;
    }
    if (pthread_join(p2, NULL) != 0) { // check for error
        return 6;
    }
    if (pthread_join(p3, NULL) != 0) {  // check for error
        return 7;
    }
    if (pthread_join(p4, NULL) != 0) { // check for error
        return 8;
    }
    pthread_mutex_destroy(&mutex);
    printf("Number of mails: %d\n", mails);
    return 0;
}