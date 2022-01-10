#include "fs/operations.h"
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#define COUNT 80
#define SIZE 256

void* routine_1() {

    char *str = "AAA!";
    char *path = "/f1";
    char buffer[40];


    int f;
    ssize_t r;

    f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_write(f, str, strlen(str));
    assert(r == strlen(str));

    assert(tfs_close(f) != -1);

    f = tfs_open(path, 0);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str));

    buffer[r] = '\0';
    assert(strcmp(buffer, str) == 0);

    assert(tfs_close(f) != -1);
    return 0;
}

int main(void ) {

    pthread_t th[5];

    assert(tfs_init() != -1);

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

    printf("Sucessful test\n");
    return 0;
}