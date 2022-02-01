#ifndef TFS_SERVER_H
#define TFS_SERVER_H

#define S 20

#define MOUNT '1'
#define UNMOUNT '2'
#define OPEN '3'
#define CLOSE '4'
#define WRITE '5'
#define READ '6'
#define SHUT_DOWN '6'

typedef struct{
    int valid;
    void *new_command;
    int op_code, count;
} session_t;

#endif // TFS_SERVER.H