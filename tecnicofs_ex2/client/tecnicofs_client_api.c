#include "tecnicofs_client_api.h"
#include "common/common.h"

int session_id = -1, fclient = -1, fserver = -1;

int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {

    unlink(client_pipe_path);
    if (mkfifo(client_pipe_path, 0777) < 0) {
        return -1;
    }

    fclient = open_failure_retry(client_pipe_path,O_RDONLY);
    if(fclient < 0) {
        return -1;
    }

    fserver = open_failure_retry(server_pipe_path,O_WRONLY);
    if(fserver < 0) {
        return -1;
    }

    void *command = malloc(sizeof(char)*(MAX_PIPE_LEN+1));
    ((char*) command)[0] = MOUNT;
    strcpy(command+1 , client_pipe_path);

    if(send_to_server(fserver,command, sizeof(char)*(MAX_PIPE_LEN + 1)) != sizeof(char)*(MAX_PIPE_LEN + 1)) {
        perror("client in tfs_mount: error sending request to server\n");
        return -1;
    }
    
    int temp;
    if(receive_from_server(fclient,&temp,sizeof(int)) < 0) {
        perror("client in tfs_mount: error receiving from server\n");
        return -1;
    }

    session_id = temp;

    return 0;
}

int tfs_unmount() {

    void *buffer = malloc(sizeof(char) + sizeof(int));
    ((char*)buffer)[0] = UNMOUNT;
    ((int*) ((char*)buffer)+1)[0] = session_id;

    if(send_to_server(fserver, buffer, sizeof(char) + sizeof(int)) != (sizeof(char) + sizeof(int))) {
        perror("client in tfs_unmount: error sending request to server\n");
        return -1;
    }

    close(fserver);
    close(fclient);

    return 0;
}

int tfs_open(char const *name, int flags) {
    
    void *command = malloc((MAX_PIPE_LEN + 1)* sizeof(char) + 2 * sizeof(int));
    void *last_pos = command;
    ((char*)last_pos)[0] = OPEN;
    last_pos = (void*)(((char*)last_pos) + 1);
    ((int*)last_pos)[0] = session_id;
    last_pos = (void*)(((int*)last_pos) + 1);
    memcpy( command , name , MAX_PIPE_LEN);
    last_pos = (void*)(((char*)last_pos) + MAX_PIPE_LEN);
    ((int*)last_pos)[0] = flags;

    
    if(send_to_server(fserver,command,(MAX_PIPE_LEN + 1) * sizeof(char) + 2 * sizeof(int)) != (MAX_PIPE_LEN + 1) * sizeof(char) + 2 * sizeof(int)) {
        perror("client in tfs_open: error sending request to server\n");
        return -1;
    }
    int temp;
    if(receive_from_server(fclient,&temp,sizeof(int)) < 0) {
        perror("client in tfs_open: error receiving from server\n");
        return -1;
    }

    int file_handle = temp;

    return file_handle;
}

int tfs_close(int fhandle) {

    void *buffer = malloc(sizeof(char) + 2 * sizeof(int));
    void *last_pos = buffer;
    ((char*)last_pos)[0] = CLOSE;
    last_pos = (void*)(((char*)last_pos) + 1);
    ((int*)last_pos)[0] = session_id;
    last_pos = (void*)(((int*)last_pos) + 1);
    ((int*)last_pos)[0] = fhandle;

    if(send_to_server(fserver,buffer, sizeof(char) + 2 * sizeof(int)) != (sizeof(char) + 2 * sizeof(int))) {
        perror("client in tfs_close: error sending request to server\n");
        return -1;

    }
    int temp;
    if(receive_from_server(fclient,&temp,sizeof(int)) < 0) {
        perror("client in tfs_close: error receiving from server\n");
        return -1;
    }
    
    if(temp == 0) {
        return temp;
    }

    return 0;

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

    if(send_to_server(fserver,command,(1 + len) * sizeof(char) + 3 * sizeof(int)) != ((1 + len) * sizeof(char) + 3 * sizeof(int))) {
        perror("client in tfs_write: error sending request to server\n");
        return -1;

    }
    size_t written_bytes = 0;
    if(receive_from_server(fclient,command, sizeof(int)) < 0) { 
        perror("client in tfs_write: error receiving from the server\n");
        return -1;
    }

    written_bytes = ((int*)command)[0];
    free(command);

    return (ssize_t)written_bytes;

}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {

    void *buffer = malloc(sizeof(char) + sizeof(int)*3);
    void *last_pos = buffer;
    ((char*)last_pos)[0] = READ; 
    last_pos = (void*)(((char*)last_pos) + 1);
    ((int*)last_pos)[0] = session_id;
    last_pos = (void*)(((int*)last_pos) + 1);
    ((int*)last_pos)[0] = fhandle;
    last_pos = (void*)(((int*)last_pos) + 1);
    ((size_t*)last_pos)[0] = len;
 

    if(send_to_server(fserver,buffer,sizeof(char) + 3 * sizeof(int)) != (sizeof(char) + 3 * sizeof(int))) {
        perror("client in tfs_read: error sending request to server\n");
        return -1;
    }   

    void *answer_from_server = malloc(sizeof(int) + sizeof(char)*MAX_FILE_LENGTH);

    if(receive_from_server(fclient,answer_from_server,sizeof(int) + sizeof(char)*MAX_FILE_LENGTH) < 0) {
        perror("client in tfs_read: error receiving from the server\n");
        return -1;
    }

    size_t read_content = 0;

    read_content = ((int*)answer_from_server)[0];

    return (ssize_t) read_content;
   
}


int tfs_shutdown_after_all_closed() {

    void* buffer = malloc(sizeof(char) + sizeof(int));
    void *last_pos = buffer;
    ((char*)last_pos)[0] = SHUT_DOWN;
    last_pos = (void*)(((char*)last_pos) + 1);
    ((int*)last_pos)[0] = session_id;
    
    if(send_to_server(fserver,buffer,sizeof(char) + sizeof(int)) != (sizeof(char) + sizeof(int))) {
        perror("client in tfs_shutdown_after_all_closed: error sending request to server\n");
        return -1;
    }

    if(receive_from_server(fclient,buffer,sizeof(int)) < 0){
        perror("client in tfs_shutdown_after_all_closed: error receiving from server");
        return -1;
    }

    return 0;
}
