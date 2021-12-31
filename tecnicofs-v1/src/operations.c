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

        /* Trucate (if requested) */
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

int write_to_block(int inumber, void *buffer , size_t len , int block_id , int block_offset) {

    void *block = data_block_get(get_inode_block(inumber , block_id));
    if (block == NULL) {
        return -1;
    }

      /* Perform the actual read */
    memcpy(block + block_offset,buffer,len);
    return len; // number of bytes that written 
}


/*
ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {

    //fhandle as the window e.g a google chrome tab 
    // you can have multiple instances of the same file opened
    // at each instance, you can have the cursor // offset at different position

    // gets the exact window 
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }
    
    // From the open file table entry, we get the inode 
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        return -1;
    }
    int written = 0;
    while(to_write - written) {
    
        // em que bloco vamos escrever 
        int inode_block = file->of_offset / BLOCK_SIZE;
        

        // allocate a block only if
        if(inode->i_size == file->of_offset && (file->of_offset % BLOCK_SIZE == 0)) {
            
            data_block_alloc();




        }
        


        

    
    }




         // ----------------------------------------------------

        // write everything we can at the current block 

        // if((inode->i_size % BLOCK_SIZE) == 0)
        
        //if (inode->i_size == 0) {
            /* If empty file, allocate new block 
        //    inode->i_data_block = data_block_alloc();
        //}

        // void *block = data_block_get(inode->i_data_block);
        //if (block == NULL) {
            //return -1;
        //}

        // Perform the actual write 
        //memcpy(block + file->of_offset, buffer, to_write);


        // ----------------------------------------------------

        /// The offset associated with the file handle is incremented accordingly 
        file->of_offset += to_write;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }

    return (ssize_t)to_write;
}

*/

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
        void *cur = buffer+read;
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
