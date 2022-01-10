#include "fs/operations.h"
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>



void* routine_3(){








}






int main(void ) {

    pthread_t th[5];

    assert(tfs_init() != -1);

    for(int i = 0; i < 5; i++) {

        if(pthread_create( &th[i],NULL,&routine_3,NULL) != 0) { // testing for errors
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

    printf("Sucessful test\n");
    return 0;
}