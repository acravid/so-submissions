#ifndef COMMON_H
#define COMMON_H


#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Writes to a file specified as its argument, normally to a named pipe
 * Input:
 *  - fd - file descriptor, where we want to write
 *  - buffer containing the contents to write
 *  - number_of_bytes, length of the contents (in bytes)
 * 	
 * Returns the number of bytes that were written (can only be number_of_bytes)
 * or -1 in case of error
 */
size_t send_to_server(int fd,void *buffer, size_t number_of_bytes);

/* Reads from a file, normally to a named pipe
 * Input:
 * 	- fd - file descriptor
 * 	- destination buffer
 * 	- length of the buffer
 * Returns the number of bytes that were copied from the file to the buffer
 * (can only be equal to 'number_of_bytes') or -1 in case of error
 */
size_t receive_from_server(int fd,void* buffer,size_t number_of_bytes);

/* Opens a file
*
*  Input: 
*   -path - file's path
*   -permissions
* Returns a file descriptor for the named file or -1 in case of error
*/
int open_failure_retry(const char *path, int oflag);

/* tfs_open flags */
enum {
    TFS_O_CREAT = 0b001,
    TFS_O_TRUNC = 0b010,
    TFS_O_APPEND = 0b100,
};

/* operation codes (for client-server requests) */
enum {
    TFS_OP_CODE_MOUNT = 1,
    TFS_OP_CODE_UNMOUNT = 2,
    TFS_OP_CODE_OPEN = 3,
    TFS_OP_CODE_CLOSE = 4,
    TFS_OP_CODE_WRITE = 5,
    TFS_OP_CODE_READ = 6,
    TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED = 7
};

#define MAX_FILE_LENGTH (1024)
#define MAX_COMMAND_LENGTH (1024)
#define MAX_PIPE_LEN (40)
#define MOUNT '1'
#define UNMOUNT '2'
#define OPEN '3'
#define CLOSE '4'
#define WRITE '5'
#define READ '6'
#define SHUT_DOWN '6'


#endif /* COMMON_H */