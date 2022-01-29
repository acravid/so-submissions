#include "operations.h"

#define MAX_REQUEST_LEN 40

int main(int argc, char **argv) {

    if (argc < 2) {
        printf("Please specify the pathname of the server's pipe.\n");
        return 1;
    }

    char *pipename = argv[1],*buffer = (char*) malloc(sizeof(char)*MAX_REQUEST_LEN);
    int fserv;
    printf("Starting TecnicoFS server with pipe called %s\n", pipename);

    unlink(pipename);
    if (mkfifo (pipename, 0777) < 0)
        exit (1);
    if ((fserv = open(pipename, O_RDONLY)) < 0) 
        exit(1);
    for(;;){
        read(fserv, buffer , MAX_REQUEST_LEN);
        /***
         * switch(buffer[0])
         *     1 
         *         tfs_() 
         * **/
        //mkfifo()
        //int fclient = open(write_pipe , )
        //write(fclient , buffer , MAX_REQUEST_LEN);
    }
    
    return 0;
}