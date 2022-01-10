#include "operations.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

pthread_rwlock_t lock_blocks, lock_inumber_all, lock_fhandle_all;
pthread_rwlock_t lock_fhandle[MAX_OPEN_FILES], lock_inumber[INODE_TABLE_SIZE];



// TODO: fix lock_wr_all; unlock_all
// use lock_inumber_all; lock_fhanlde_all

// Instead of blocking all inumbers or fhandles
// one by one, we can block all inumbers/fhandles



/*void lock_wr_all(pthread_rwlock_t * table , int size){
    for(int i = 0 ; i < size ; i++)
        pthread_rwlock_wrlock(&table[i]);
}
void unlock_all(pthread_rwlock_t * table , int size){
    for(int i = 0 ; i < size ; i++)
        pthread_rwlock_unlock(&table[i]);
} */

void locks_destroy() {
    
    pthread_rwlock_destroy(&lock_blocks);
    pthread_rwlock_destroy(&lock_inumber_all);
    pthread_rwlock_destroy(&lock_fhandle_all);

    for(int i = 0; i < MAX_OPEN_FILES; i++) {
        pthread_rwlock_destroy(&lock_fhandle[i]);
    }

    for(int i = 0; i < INODE_TABLE_SIZE; i++) {
        pthread_rwlock_destroy(&lock_inumber[i]);
    }
}


int locks_bundle_init() {

    for(int i = 0; i < MAX_OPEN_FILES; i++) {
        if(pthread_rwlock_init(&lock_fhandle[i],NULL) != 0) {
            return -1;
        }
    }

    for(int i = 0; i < INODE_TABLE_SIZE; i++) {
        if(pthread_rwlock_init(&lock_inumber[i],NULL) != 0) {
            return -1;
        }
    }

    // if all initializations were successful
    return 0;
}

int locks_init() {

    int return_value = 0;
    return_value = locks_bundle_init();
    if(return_value = -1) {
        return return_value;
    } else {
        if(pthread_rwlock_init(&lock_blocks,NULL) != 0 || \
        pthread_rwlock_init(&lock_inumber_all,NULL) != 0  || \ 
        pthread_rwlock_init(&lock_fhandle_all, NULL) != 0 ) {
            return -1;
        } 
        return return_value;
    }
    return return_value;
}

    

int tfs_init() {

    // initialize all locks
    if(locks_init() != 0 || locks_bundle_init() != 0) {
        return -1;
    }
    state_init();

    /* create root inode */
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}


int tfs_destroy() {
    
    locks_destroy();
    state_destroy();
    return 0;
}


static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}


int tfs_lookup(char const *name) {
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;
    pthread_rwlock_rdlock(&lock_inumber[ROOT_DIR_INUM]);
    pthread_rwlock_rdlock(&lock_blocks);
    int return_val = find_in_dir(ROOT_DIR_INUM, name);
    pthread_rwlock_unlock(&lock_inumber[ROOT_DIR_INUM]);
    pthread_rwlock_unlock(&lock_blocks);
    return return_val;
}

int tfs_open(char const *name, int flags) {
    int inum;
    size_t offset;

    /* Checks if the path name is valid */
    if (!valid_pathname(name)) {
        return -1;
    }

    inum = tfs_lookup(name);

    if (inum >= 0) {
        /* The file already exists */
        pthread_rwlock_rdlock(&lock_inumber[inum]);
        inode_t *inode = inode_get(inum);
        if (inode == NULL) {
            return -1;
        }

        /* Truncate (if requested) */
        if (flags & TFS_O_TRUNC) {
            pthread_rwlock_wrlock(&lock_blocks);
            if(truncate_file(inum) == -1){
                pthread_rwlock_unlock(&lock_inumber[inum]);
                pthread_rwlock_unlock(&lock_blocks);
                return -1;
            }
            else
                inode->i_size = 0;
            pthread_rwlock_unlock(&lock_blocks);
        }
        /* Determine initial offset */
        if (flags & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
        pthread_rwlock_unlock(&lock_inumber[inum]);
    } else if (flags & TFS_O_CREAT) {
        /* The file doesn't exist; the flags specify that it should be created*/
        /* Create inode */
        // -- FIX
        lock_wr_all(lock_inumber , INODE_TABLE_SIZE);
        inum = inode_create(T_FILE);
        if (inum == -1) {
            // -- FIX
            unlock_all(lock_inumber , INODE_TABLE_SIZE);
            return -1;
        }
        pthread_rwlock_wrlock(&lock_blocks);
        /* Add entry in the root directory */
        if (add_dir_entry(ROOT_DIR_INUM, inum, name + 1) == -1) {
            inode_delete(inum);
            // -- FIX
            unlock_all(lock_inumber , INODE_TABLE_SIZE);
            pthread_rwlock_unlock(&lock_blocks);
            return -1;
        }
        // -- FIX
        unlock_all(lock_inumber , INODE_TABLE_SIZE);
        pthread_rwlock_unlock(&lock_blocks);
        offset = 0;
    } else {
        return -1;
    }

    /* Finally, add entry to the open file table and
     * return the corresponding handle */
    // -- FIX
    lock_wr_all(lock_fhandle , MAX_OPEN_FILES);
    int return_val = add_to_open_file_table(inum, offset);
    // -- FIX
    unlock_all(lock_fhandle , MAX_OPEN_FILES);
    return return_val;

    /* Note: for simplification, if file was created with TFS_O_CREAT and there
     * is an error adding an entry to the open file table, the file is not
     * opened but it remains created */
}


int tfs_close(int fhandle) { 
    // -- FIX
    lock_wr_all(lock_fhandle , MAX_OPEN_FILES);
    int return_val = remove_from_open_file_table(fhandle); 
    // -- FIX
    unlock_all(lock_fhandle , MAX_OPEN_FILES);
    return return_val;
}


ssize_t write_to_block(int inumber, void *buffer , size_t len , int block_id , int block_offset) {

    void *block = data_block_get(get_inode_block(inumber , block_id));
    if (block == NULL) {
        return -1;
    }

      /* Perform the actual read */
    memcpy(block + block_offset,buffer,len);
    return (ssize_t)len; // number of bytes that  were written 
}


// TODO: 1:fix block_offset
//       2:updates the buffer ? void const *  | and int
ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {

    if(fhandle < 0 || fhandle >= MAX_OPEN_FILES)
        return -1;
    pthread_rwlock_wrlock(&lock_fhandle[fhandle]);
    open_file_entry_t *file = get_open_file_entry(fhandle);
    
    if (file == NULL) {
        pthread_rwlock_unlock(&lock_fhandle[fhandle]);
        return -1;
    }

    if(file->of_inumber < 0 || file->of_inumber > INODE_TABLE_SIZE){
        pthread_rwlock_unlock(&lock_fhandle[fhandle]);
        return -1;
    }
    pthread_rwlock_rdlock(&lock_inumber[file->of_inumber]);
    
    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        pthread_rwlock_unlock(&lock_inumber[file->of_inumber]);
        pthread_rwlock_unlock(&lock_fhandle[fhandle]);
        return -1;
    }

    /* Determine how many bytes to write */
    // Check if what we're trying to write in conjunction 
    // to what has already been written is bigger than the maximum
    // number of bytes permitted per file, if so, we can only write
    // the difference between what's possible and what we already have
    // [    ,   ,   ,   ,   ,   ,   ,] -> the maximum number of bytes per file
    // [    ,   ,   ,   ,   ,   ,   ,   ,   ,] -> the buffer 
    // [    ,   ,   ,   ,   ?   ,   ,] -> ? offset, NOTE: where the "cursor" is currently at
    // [                    |       |] -> |     | bytes we can still write
    if (to_write + file->of_offset > MAXIMUM_FILE_BYTES) {
        to_write = MAXIMUM_FILE_BYTES - file->of_offset;
    }
    
        
    // this variable controls the total number of bytes of the buffer
    // we have already written
    size_t written = 0;
    while(to_write - written) { 
        
        int inode_block_id = (int) file->of_offset / BLOCK_SIZE;
        pthread_rwlock_wrlock(&lock_blocks);
        if(file->of_offset == inode->i_size && inode->i_size % BLOCK_SIZE == 0)
            inumber_block_alloc(file->of_inumber); 

        // NOTE: we have to continuously update the buffer
        // let's imagine the following buffer: [| |,| |,| |,| |,| |,| |,| |,| |,.......] 
        // let's assume that each | | -> 1 byte
        // if we have already written 5 bytes of the buffer, the next time we call
        // write_to_block we have to pass a newer version of the buffer that reflects
        // the changes
        // [| |,| |,| |,| |,| |,| |,| |,| |.......] 
        //                      ? start here ; next iteration
        void *buffer_update = (void*)((char*)buffer + written); 
        // number of bytes (to attempt) to write to a block
        size_t to_write_block = to_write - written;

        if(to_write_block > BLOCK_SIZE - (file->of_offset % BLOCK_SIZE))
            to_write_block = BLOCK_SIZE - (file->of_offset % BLOCK_SIZE);
        // NOTE: tries to write the buffer in the previously obtained inode block id
        ssize_t bytes_written_in_block = write_to_block(file->of_inumber, buffer_update, to_write_block, inode_block_id, file->of_offset % BLOCK_SIZE );
        pthread_rwlock_unlock(&lock_blocks);
        if(bytes_written_in_block == -1) {
            pthread_rwlock_unlock(&lock_inumber[file->of_inumber]);
            pthread_rwlock_unlock(&lock_fhandle[fhandle]);
            return -1;
        } else { // if we were able to write a certain amount of bytes to a block then:
            written = written + (size_t)bytes_written_in_block; // updates the total amount of bytes written so far
        
            // The offset associated with the file handle is
            //incremented accordingly 
            file->of_offset += (size_t) bytes_written_in_block;
            // if we wrote past the the previous file size
            // we need to update it
            if (file->of_offset > inode->i_size)
                inode->i_size = file->of_offset;
        }
    }
    pthread_rwlock_unlock(&lock_inumber[file->of_inumber]);
    pthread_rwlock_unlock(&lock_fhandle[fhandle]);
    return (ssize_t)to_write;
}


ssize_t read_in_block(int inumber, void *buffer , size_t len , int block_id , int block_offset){
    void *block = data_block_get(get_inode_block(inumber , block_id));
    if (block == NULL) {
        return -1;
    }
      /* Perform the actual read */
    memcpy(buffer, block + block_offset, len);
    return (ssize_t) len;
}


ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    if(fhandle < 0 || fhandle >= MAX_OPEN_FILES)
        return -1;
    pthread_rwlock_wrlock(&lock_fhandle[fhandle]);
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        pthread_rwlock_unlock(&lock_fhandle[fhandle]);
        return -1;
    }

    if(file->of_inumber < 0 || file->of_inumber > INODE_TABLE_SIZE){
        pthread_rwlock_unlock(&lock_fhandle[fhandle]);
        return -1;
    }
    pthread_rwlock_rdlock(&lock_inumber[file->of_inumber]);

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        pthread_rwlock_unlock(&lock_inumber[file->of_inumber]);
        pthread_rwlock_unlock(&lock_fhandle[fhandle]);
        return -1;
    }

    /* Determine how many bytes to read */
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }

    if (file->of_offset + to_read >= MAXIMUM_FILE_BYTES) {
        pthread_rwlock_unlock(&lock_inumber[file->of_inumber]);
        pthread_rwlock_unlock(&lock_fhandle[fhandle]);
        return -1;
    }

    size_t read = 0;    
    while(to_read > read) {
        void *cur = (void*)((char*)buffer+read);
        size_t to_read_block = to_read-read;

        int block = (int) file->of_offset/BLOCK_SIZE;
        if(to_read_block > BLOCK_SIZE - (file->of_offset % BLOCK_SIZE))
            to_read_block = BLOCK_SIZE - (file->of_offset % BLOCK_SIZE);
        pthread_rwlock_rdlock(&lock_blocks);
        if(read_in_block(file->of_inumber, cur , to_read_block , block , file->of_offset % BLOCK_SIZE) == -1){
            pthread_rwlock_unlock(&lock_blocks);
            pthread_rwlock_unlock(&lock_inumber[file->of_inumber]);
            pthread_rwlock_unlock(&lock_fhandle[fhandle]);
            return -1;
        }
        pthread_rwlock_unlock(&lock_blocks);
        
        file->of_offset += to_read_block;
        if(file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
        read += to_read_block;
    }
    pthread_rwlock_unlock(&lock_inumber[file->of_inumber]);
    pthread_rwlock_unlock(&lock_fhandle[fhandle]);
    return (ssize_t)to_read;
}


int tfs_copy_to_external_fs(char const *source_path, char const *dest_path){
    
    FILE *fp = fopen(dest_path,"w");

    int fhandle;
    size_t len;
    void *buffer;

    if(tfs_lookup(source_path) == -1 || fp == NULL){
        return -1;
    }

    fhandle = tfs_open(source_path,0);

    if(fhandle < 0 || fhandle >= MAX_OPEN_FILES)
        return -1;
    pthread_rwlock_rdlock(&lock_fhandle[fhandle]);

    // gets the file from the entry
    open_file_entry_t *file = get_open_file_entry(fhandle);
    // gets a pointer to the file's inode
    if(file->of_inumber < 0 || file->of_inumber > INODE_TABLE_SIZE){
        pthread_rwlock_unlock(&lock_fhandle[fhandle]);
        return -1;
    }
    pthread_rwlock_rdlock(&lock_inumber[file->of_inumber]);
    inode_t *inode = inode_get(file->of_inumber);
    
    
    len = inode->i_size;
    buffer = (void*)malloc(sizeof(char)*len);

    pthread_rwlock_unlock(&lock_inumber[file->of_inumber]);
    pthread_rwlock_unlock(&lock_fhandle[fhandle]);

    // given the buffer, our goal is to write the file contents to the buffer 
    // and only after copy its contents to a file in the main OS

    if(tfs_read(fhandle,buffer,len) != (ssize_t)len || \
    fwrite(buffer,sizeof(char), len ,fp) != len) {
        fclose(fp);
         return -1;
    } else {
         free(buffer);
         fclose(fp);
         return 0;
    }
}