#include "operations.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tfs_init() {
    state_init();

    /* create root inode */
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}


int tfs_destroy() {
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

    return find_in_dir(ROOT_DIR_INUM, name);
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
        inode_t *inode = inode_get(inum);
        if (inode == NULL) {
            return -1;
        }

        /* Truncate (if requested) */
        if (flags & TFS_O_TRUNC) {
            if(truncate_file(inum) == -1)
                return -1;
            else
                inode->i_size = 0;
        }
        /* Determine initial offset */
        if (flags & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (flags & TFS_O_CREAT) {
        /* The file doesn't exist; the flags specify that it should be created*/
        /* Create inode */
        inum = inode_create(T_FILE);
        if (inum == -1) {
            return -1;
        }
        /* Add entry in the root directory */
        if (add_dir_entry(ROOT_DIR_INUM, inum, name + 1) == -1) {
            inode_delete(inum);
            return -1;
        }
        offset = 0;
    } else {
        return -1;
    }

    /* Finally, add entry to the open file table and
     * return the corresponding handle */
    return add_to_open_file_table(inum, offset);

    /* Note: for simplification, if file was created with TFS_O_CREAT and there
     * is an error adding an entry to the open file table, the file is not
     * opened but it remains created */
}


int tfs_close(int fhandle) { return remove_from_open_file_table(fhandle); }


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

    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
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
    
    if(to_write > 0) {
        
        // this variable controls the total number of bytes of the buffer
        // we have already written
        size_t written = 0;
        while(to_write - written) { 

            // TODO: 
            // What if we already have pre allocated blocks

            // file->offset onde é que altera 

            // [[0][1][2][3][4][5]]            
            // [[ABDJFJDGFDKGKDGDKGKDGKDKG][ABDJFJDGFDKGKDGDKGKDGKDKG][ABDJFJDGFDKGKDGDKGKDGKDKG][ABDJFJDGFDKGKDGDKGKDGKDKG][ABDJFJDGFDKGKDGDKGKDGKDKG]]
            //  ?
            // [dkgkdgnkongjrnorjgo5jrrWIOROHNOO3OMOTHIT5HK5HK5HOK5H5H5................................................................................]

            // file->offset == inode->i_size e tamos prestes a começar um novo bloc

            // inode->i_size 
            // 
            //----------------------------------------------
            // a small walkthrough to verify the correctness 
            //----------------------------------------------
            // let's start at the base case
            // let's imagine that our file size ; inode->i_size = 0;
            // buffer = "ASDSDDD......"
            // file->offset = 0;
            // len buffer = 3080 bytes
            // first attempt to write on the FS
            // | 1st iteration:
            // 1: inode_block_id = 0 | inumber_block_alloc allocates the first ever block at index = 0 of the "data region"
            // 2: buffer_update = buffer + 0 
            // 3: to_write_block = to_write_block - 0
            // 4: if statement: 3080 and then: 1024
            // 5: write_to_block returns 1024, hence 1024 bytes were written to block 0 | direct access block
            // 6: written += bytes_written_in_block ; written = 1024
            // 7: file->offset = 1024
            // 8: updates the i_size

            // | 2nd iteration:
            // 1: the loop condition still verifies ; 3080 - 1024  = 2056 bytes
            // 2: inode_block_id = 1 | block_id = inode_table[inumber].i_size / BLOCK_SIZE; | 1024/1024
            // 3: updates the buffer ?
            // 4: to_write_block = 2056
            // 5: if statement: 2056 > 1024 -> to_write = 1024
            // 6: write_to_block returns (1024 bytes) were written to block 1 | direct access block
            // 7: written += bytes_written_in_block = 1024 + 1024 = 2048
            // 8: file->offset = 2048
            // 9: updates the i_size

            // | 3rd iteration:
            // 1: the loop condition still verifies; 3080 - 2048 = 1032 > 0
            // 2: inode_block_id = 2 | block_id = inode_table[inumber].i_size / BLOCK_SIZE; | 2048 / 1024
            // 3: updates the buffer ?
            // 4: to_write_block = 1032
            // 5: if statement: 1032 > 1024 -> to_write = 1024
            // 6: write_to_block returns (1024 bytes) were written to block 2 | direct access block
            // 7: written += bytes_written_in_block = 2048 + 1024 = 3072
            // 8: file->offset = 3072
            // 9: updates the i_size

            // | 4th iteration:
            // 1: the loop condition still verifies; 3080 - 3072 > 0 ;  8 > 0
            // 2: inode_block_id = 3 | 3072 / 1024
            // 3: updates the buffer ?
            // 4; to_write_block = 8
            // 5: if statement: 8 > 1024 | fails
            // 6: write_to_block returns (8 bytes) were written to block 3 | direct access block
            // 7: written += bytes_written_in_block = 3072 + 8 = 3080
            // 8: file->offset = 3080
            // 9: updates the i_size 

            // | 5th iteration ? the loop breaks 
            // 
            int inode_block_id = inumber_block_alloc(file->of_inumber); 

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
            

            if(to_write_block > BLOCK_SIZE - (file->of_offset % BLOCK_SIZE)) {
                to_write_block = BLOCK_SIZE - (file->of_offset % BLOCK_SIZE);
            }
            // NOTE: tries to write the buffer in the previously obtained inode block id
            int bytes_written_in_block = write_to_block(file->of_inumber, buffer_update, to_write_block, inode_block_id, file->of_offset % BLOCK_SIZE );
            if(bytes_written_in_block == -1) {
                return -1;
            } else { // if we were able to write a certain amount of bytes to a block then:
                written += bytes_written_in_block; // updates the total amount of bytes written so far
           
                // The offset associated with the file handle is
                //incremented accordingly 
                file->of_offset += to_write_block;
                // if we wrote past the the previous file size
                // we need to update it
                if (file->of_offset > inode->i_size) {

                    inode->i_size = file->of_offset;
                }   
            }
        }
    }  
    return (ssize_t)to_write;
}


int read_in_block(int inumber, void *buffer , size_t len , int block_id , int block_offset){
    void *block = data_block_get(get_inode_block(inumber , block_id));
    if (block == NULL) {
        return -1;
    }

      /* Perform the actual read */
    memcpy(buffer, block + block_offset, len);
    return len;
}


ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        return -1;
    }

    /* Determine how many bytes to read */
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }

    if (file->of_offset + to_read >= MAXIMUM_FILE_BYTES) {
        return -1;
    }

    int read = 0;
    while(to_read > read) {
        void *cur = (void*)((char*)buffer+read);
        int to_read_block = to_read-read;

        int block = file->of_offset/BLOCK_SIZE;
        if(to_read_block > BLOCK_SIZE - (file->of_offset % BLOCK_SIZE))
            to_read_block = BLOCK_SIZE - (file->of_offset % BLOCK_SIZE);
        if(read_in_block(file->of_inumber, cur , to_read_block , block , file->of_offset % BLOCK_SIZE) == -1)
            return -1;
        
        file->of_offset += to_read_block;
        if(file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
        read += to_read_block;
    }
    return (ssize_t)to_read;
}


int tfs_copy_to_external_fs(char const *source_path, char const *dest_path){
    
    FILE *fp = fopen(dest_path,"w");

    int fhandle,len;
    char *buffer;

    if(tfs_lookup(source_path) == -1 || fp == NULL){
        return -1;
    }

    fhandle = tfs_open(source_path,0);
    
    // gets the file from the entry
    open_file_entry_t *file = get_open_file_entry(fhandle);
    // gets a pointer to the file's inode
    inode_t *inode = inode_get(file->of_inumber);
    
    
    len = inode->i_size;
    buffer = (char*)malloc(sizeof(char)*len);

    // given the buffer, our goal is to write the file contents to the buffer 
    // and only after copy its contents to a file in the main OS
    if(tfs_read(fhandle,buffer,len) != len || \
    fwrite(buffer,sizeof(char),sizeof(buffer),fp) != sizeof(buffer)) {
         return -1;
     } else {
         free(buffer);
         return 1;
    }
}
