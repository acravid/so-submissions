#ifndef TFS_SERVER_H
#define TFS_SERVER_H

#define S 20


typedef struct{
    int valid;
    void *new_command;
    int op_code, count;
} session_t;

#endif // TFS_SERVER.H