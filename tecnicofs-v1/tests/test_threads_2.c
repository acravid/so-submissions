#include "fs/operations.h"
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#define COUNT 80
#define SIZE 256

void* routine_2() {

    char *path = "/f1";

    /* Writing this buffer multiple times to a file stored on 1KB blocks will 
       always hit a single block (since 1KB is a multiple of SIZE=256) */
    char input[SIZE]; 
    memset(input, 'A', SIZE);

    char output [SIZE];


    /* Write input COUNT times into a new file */
    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd != -1);
    for (int i = 0; i < COUNT; i++) {
        assert(tfs_write(fd, input, SIZE) == SIZE);
    }
    assert(tfs_close(fd) != -1);

    /* Open again to check if contents are as expected */
    fd = tfs_open(path, 0);
    assert(fd != -1 );

    for (int i = 0; i < COUNT; i++) {
        assert(tfs_read(fd, output, SIZE) == SIZE);
        assert (memcmp(input, output, SIZE) == 0);
    }

    assert(tfs_close(fd) != -1);
    return 0;
}


int main(void ) {

    pthread_t th[5];

    assert(tfs_init() != -1);

    for(int i = 0; i < 5; i++) {

        if(pthread_create( &th[i],NULL,&routine_2,NULL) != 0) { // testing for errors
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

