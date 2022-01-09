#include "fs/operations.h"
#include <pthread.h>
#include <unistd.h>
#include <assert.h>




void* routine_1() {

// call tfs functions
}


void* routine_2() {

// cal tfs functions
}


int main(void ) {

    pthread_t th[5];


    for(int i = 0; i < 5; i++) {

        if(pthread_create( &th[i],NULL,&routine_1,NULL) != 0) { // testing for errors
            perror("Failed to create thread\n");
            return 1;
        }
        printf("Thread %d has started its execution\n", i);
    }


    for(int i = 0; i < 5 ; i++) {

        if(pthread_join(th[i], NULL) != 0) { // testing for errors
            return 2;
        }
        printf("Thread %d has finished its execution\n", i);
    }
















    return 0;
}