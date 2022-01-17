#include "fs/operations.h"
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#define SIZE 256
#define COUNT 40

void* routine_3(){

    char *path = "/f2";

    /* Writing this buffer multiple times to a file stored on 1KB blocks will 
       always hit a single block (since 1KB is a multiple of SIZE=256) */
    char input[SIZE]; 
    memset(input, 'B', SIZE);

    char output [SIZE];


    /* Write input COUNT times into a new file */
    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd != -1);
    for (int i = 0; i < COUNT; i++) {
        assert(tfs_write(fd, input, SIZE) == SIZE);
    }
    assert(tfs_close(fd) != -1);

    /* empty file*/
    fd = tfs_open(path,TFS_O_TRUNC);
    assert(fd != -1 );

    assert(tfs_read(fd,output,0) == 0);

    assert(tfs_close(fd) != -1);
    return 0;

}

int main(void ) {

    pthread_t th[100];

    assert(tfs_init() != -1);

    for(int i = 0; i < 100; i++) {

        if(pthread_create( &th[i],NULL,&routine_3,NULL) != 0) { 
            perror("Failed to create thread\n");
            return 1;
        }
        printf("Thread %d has started its execution\n", i);
    }

    for(int i = 0; i < 100; i++) {

        if(pthread_join(th[i], NULL) != 0) { 
            return 2;
        }
        printf("Thread %d has finished its execution\n", i);
    }

    printf("Sucessful test\n");
    return 0;
}