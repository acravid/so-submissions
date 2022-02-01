#include "operations.h"
#include "tfs_server.h"

#define MAX_REQUEST_LEN 40

session_t session[S];
pthread_cond_t pode_enviar[S],pode_processar[S];
pthread_mutex_t session_lock[S];

void *tarefa_trabalhadora(int session_id){
    while(1){
        pthread_mutex_lock(&session_lock[session_id]);
        while(session[session_id].count == 0)
            pthread_cond_wait(&pode_processar[session_id], &session_lock[session_id]);
        
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
    
    return 0;
}