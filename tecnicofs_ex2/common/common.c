/*This file contains functions that will be 
both use by the client's API and the server*/

#include "common.h"

size_t send_to_server(int fd,void *buffer, size_t number_of_bytes) {

    size_t written_bytes = 0;
    
    while(written_bytes < number_of_bytes) {
        size_t already_sent = write(fd,buffer,number_of_bytes - written_bytes);

        if(already_sent == -1){
            if(errno == EINTR) { 
                continue;
            } else {
                return -1;
            }
        }
        written_bytes += already_sent;
    }

    return written_bytes;
}


size_t receive_from_server(int fd,void* buffer,size_t number_of_bytes) {

    size_t read_bytes = 0;
    int interrupted = 1;
    size_t already_read;

    while(interrupted) {
        already_read = read(fd,buffer,number_of_bytes - read_bytes);
        interrupted = (already_read == -1) && (errno == EINTR);
        read_bytes += already_read;
    }
    if(already_read == -1) {
        return -1;
    }

    return read_bytes;
}


int open_failure_retry(const char *path, int oflag) {
    
    int fd;
    do {
        fd = open(path,oflag);

    // only retry the operation if the error is caused by an interrupt
    }while((fd == -1 && errno == EINTR));

    if(fd == -1) {
        return -1;
    }
    
    return fd;
}


