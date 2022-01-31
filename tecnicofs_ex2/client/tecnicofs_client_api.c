#include "tecnicofs_client_api.h"
#include "fs/tfs_server.h"
#include <errno.h>

#define MAX_PIPE_LEN 40
int session_id = -1,fclient = -1, fserver = -1;

size_t send_to_server(void *buffer,int fd, size_t number_of_bytes) {

    int return_value = 1;
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


size_t read_from_server() {





}

int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {

    unlink(client_pipe_path);
    if (mkfifo (client_pipe_path, 0777) < 0)
        return -1;

    if ((fclient = open(client_pipe_path, O_RDONLY)) < 0) 
        return -1;
    if ((fserver = open(server_pipe_path, O_WRONLY)) < 0) 
        return -1;

    void *buffer = malloc(sizeof(char)*(MAX_PIPE_LEN+1));
    ((char*) buffer)[0] = MOUNT;
    strcpy(buffer+1 , client_pipe_path);
    if(write(fserver, buffer , MAX_PIPE_LEN +1) <= 0)
        return -1;
    
    if(read(fclient , buffer , sizeof(int)) <= 0)
        return -1;
    session_id = ((int*) buffer)[0];

    return 0;
}

int tfs_unmount() {
    void *buffer = malloc(sizeof(char) + sizeof(int));
    ((char*)buffer)[0] = UNMOUNT;
    ((int*) ((char*)buffer)+1)[0] = session_id;

    if(write(fserver , buffer , sizeof(char)+sizeof(int)) <= 0)
        return -1;

    close(fserver);
    close(fclient);
    return -1;
}

int tfs_open(char const *name, int flags) {
    void *buffer = malloc(41*sizeof(char)+ 2*sizeof(int) );
    void *last_pos = buffer;
    ((char*)last_pos)[0] = OPEN;
    last_pos = (void*)(((char*)last_pos) + 1);
    ((int*)last_pos)[0] = session_id;
    last_pos = (void*)(((int*)last_pos) + 1);
    memcpy( buffer , name , 40);
    last_pos = (void*)(((char*)last_pos) + 40);
    ((int*)last_pos)[0] = flags;

    if(write(fserver, buffer , 41*sizeof(char)+2*sizeof(int)) <= 0)
        return -1;
    
    if(read(fclient , buffer , sizeof(int)) <= 0)
        return -1;
    return -1;
}

int tfs_close(int fhandle) {
    void *buffer = malloc(sizeof(char)+ 2*sizeof(int) );
    void *last_pos = buffer;
    ((char*)last_pos)[0] = CLOSE;
    last_pos = (void*)(((char*)last_pos) + 1);
    ((int*)last_pos)[0] = session_id;
    last_pos = (void*)(((int*)last_pos) + 1);
    ((int*)last_pos)[0] = fhandle;

    if(write(fserver, buffer ,sizeof(char)+ 2*sizeof(int) ) <= 0)
        return -1;
    
    if(read(fclient , buffer , sizeof(int)) <= 0)
        return -1;
    return -1;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t len) {
    void *command = malloc((1+len)*sizeof(char)+ 3*sizeof(int) );
    void *last_pos = command;
    ((char*)last_pos)[0] = WRITE;
    last_pos = (void*)(((char*)last_pos) + 1);
    ((int*)last_pos)[0] = session_id;
    last_pos = (void*)(((int*)last_pos) + 1);
    ((int*)last_pos)[0] = fhandle;
    last_pos = (void*)(((int*)last_pos) + 1);
    ((size_t*)last_pos)[0] = fhandle;
    last_pos = (void*)(((size_t*)last_pos) + 1);

    memcpy(command , buffer , len);
    if(write(fserver, command ,(1+len)*sizeof(char)+ 3*sizeof(int) ) <= 0)
        return -1;
    
    if(read(fclient , command , sizeof(int)) <= 0)
        return -1;

    return -1;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    /* TODO: Implement this */

    /* (char) OP_CODE=6 | (int) session_id | (int) fhandle | (size_t) len*/

    void* buffer = malloc(sizeof(char) + sizeof(int) + sizeof(int) + sizeof(size_t));
    ((char*)buffer)[0] = READ;
    ((int*) ((char*)buffer)+ 1)[0] = session_id;
    ((int*) ((char*)buffer)+ 1)[1] = fhandle;
    ((size_t*)((int*)buffer) + 2)[0] = len;

    if(write(fserver,buffer,sizeof(char) + sizeof(int) + sizeof(int) + sizeof(size_t)) <= 0) {
        return -1;
    }

    int bytes_read = read(fclient,buffer,sizeof(int)); /*+ sizeof(char)*/ 

    if(bytes_read <= 0) {
        return -1;
    } else {
        return bytes_read;
    }
   
}

int tfs_shutdown_after_all_closed() {
    /* TODO: Implement this */

    void* buffer = malloc(sizeof(char) + sizeof(int));
    ((char*)buffer)[0] = SHUT_DOWN;
    ((int*) ((char*)buffer)+1)[0] = session_id;


    if(write(fserver ,buffer , sizeof(char) + sizeof(int)) <= 0) {
        return -1;
    }

    if(read(fclient,buffer,sizeof(int)) <= 0)  { // <= ? < ?
        return - 1;
    }

    return 0;
}
