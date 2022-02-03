#include "operations.h"
#include "tfs_server.h"
#include <errno.h>

#define MAX_REQUEST_LEN 40

session_t session[S];
pthread_cond_t pode_enviar[S],pode_processar[S];
pthread_mutex_t session_lock[S];

size_t send_to_client(int fd,void *buffer, size_t number_of_bytes) {

    size_t written_bytes = 0;
    if(fd < 0)
        return -1;
    
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

void *tarefa_trabalhadora(int session_id){
    int fclient= -1;
    while(1){
        pthread_mutex_lock(&session_lock[session_id]);
        while(session[session_id].count == 0)
            pthread_cond_wait(&pode_processar[session_id], &session_lock[session_id]);
        
        void *return_message;
        switch (session[session_id].op_code){
            case 1:
                void *client_pipe_path = malloc(sizeof(char)*MAX_PIPE_LEN);
                return_message = malloc(sizeof(int));
                memcpy(client_pipe_path , session[session_id].new_command, (size_t) MAX_PIPE_LEN);
                fclient = open_failure_retry(client_pipe_path,O_WRONLY);
                if(fclient < 0)
                    ((int*)return_message)[0] = -1,session[session_id].valid = 0;
                else
                    ((int*)return_message)[0] = 0,session[session_id].valid = 1;
                free(client_pipe_path);
                break;
            case 2:
                return_message = malloc(sizeof(int));
                ((int*)return_message)[0] = 0;
                session[session_id].valid = 1;
                close(fclient);
                break;
            case 3:
                void *name = malloc(sizeof(char)*40);
                return_message = malloc(sizeof(int));
                int flags;
                memcpy(name , session[session_id].new_command , (size_t) 40);
                flags = ((int*) (((char*)name)+40))[0];

                ((int*)return_message)[0] = tfs_open(name , flags);
                free(name);
                break;
            case 4:
                int fhandle = ((int*)session[session_id].new_command)[0];
                return_message = malloc(sizeof(int));
                ((int*)return_message)[0] = tfs_close(fhandle);
                break;
            case 5:
                int fhandle = ((int*)session[session_id].new_command)[0];
                size_t len = ((int*)session[session_id].new_command)[1];
                void *buffer = malloc(sizeof(char)*len);
                memcpy(buffer , ((char*) (((int*)session[session_id].new_command)+ 2)), len*sizeof(char));
                return_message = malloc(sizeof(int));
                ((int*)return_message)[0] = tfs_write(fhandle, buffer, len);
                free(buffer);
                break;
            case 6:
                int fhandle = ((int*)session[session_id].new_command)[0];
                size_t len = ((int*)session[session_id].new_command)[1];
                void *buffer = malloc(sizeof(char)*len);
                size_t bytes_read = tfs_read(fhandle , buffer , len);
                if(bytes_read == -1)
                    return_message = malloc(sizeof(int)),((int*)return_message)[0] = -1;
                else{
                    return_message = malloc(sizeof(int)+ sizeof(char)*bytes_read);
                    ((int*)return_message)[0] = -1;
                    memcpy((char*) (((int*)return_message) + 1) , buffer , bytes_read);
                }
                free(buffer);
                break;
            case 7:
                    
                break;
            
            default:
                break;
        }
        send_to_client(fclient , return_message , sizeof(int));
        free(return_message);
        /* Processar o pedido session[session_id].new_command*/
        /*responder diretamente ao client*/

        session[session_id].count--;
        pthread_cond_signal(&pode_enviar[session_id]);
        pthread_mutex_unlock(&session_lock[session_id]);
    }
}

void init(){
    for(int i = 0 ; i < S ; i++){
        pthread_cond_init(&pode_enviar[i], NULL);
        pthread_cond_init(&pode_processar[i], NULL);
        pthread_mutex_init(&session_lock[i], NULL);
    }
    for(int i = 0 ; i < S ;i++)
        session[i].valid = 0,session[i].count = 0;
}

int main(int argc, char **argv) {

    if (argc < 2) {
        printf("Please specify the pathname of the server's pipe.\n");
        return 1;
    }
    init();

    char *pipename = argv[1];
    void *buffer = (void*) malloc(sizeof(char)*MAX_REQUEST_LEN);
    int fserv;
    printf("Starting TecnicoFS server with pipe called %s\n", pipename);

    unlink(pipename);
    if (mkfifo (pipename, 0777) < 0)
        exit (1);
    if ((fserv = open(pipename, O_RDONLY)) < 0) 
        exit(1);

    while(1){
        int session_id = -1;
        read(fserv, buffer , MAX_REQUEST_LEN);
        if(((char*)buffer)[0] == '1'){
            /*calculate new session_id*/
            pthread_mutex_lock(&session_lock[session_id]);
            /*create session and make it valid*/
            session[session_id].count++;
            pthread_cond_signal(&pode_processar[session_id]);
            pthread_mutex_unlock(&session_lock[session_id]);
        }
        else{
            pthread_mutex_lock(&session_lock[session_id]);
            while(session[session_id].count != 0)
                pthread_cond_wait(&pode_enviar[session_id], &session_lock[session_id]); 
            
            /*create new_command and copy to session table*/
            session[session_id].count++;
            pthread_cond_signal(&pode_processar[session_id]);
            pthread_mutex_unlock(&session_lock[session_id]);
        }
    }
    free(buffer);
    
    return 0;
}