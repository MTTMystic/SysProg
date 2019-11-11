/**
 * Finding Filesystems
 * CS 241 - Fall 2019
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

/**
 * Virtual paths:
 *  Add your new virtual endpoint to minixfs_virtual_path_names
 */
char *minixfs_virtual_path_names[] = {"info", /* add your paths here*/};

/**
 * Forward declaring block_info_string so that we can attach unused on it
 * This prevents a compiler warning if you haven't used it yet.
 *
 * This function generates the info string that the virtual endpoint info should
 * emit when read
 */
static char *block_info_string(ssize_t num_used_blocks) __attribute__((unused));
static char *block_info_string(ssize_t num_used_blocks) {
    char *block_string = NULL;
    ssize_t curr_free_blocks = DATA_NUMBER - num_used_blocks;
    asprintf(&block_string, "Free blocks: %zd\n"
                            "Used blocks: %zd\n",
             curr_free_blocks, num_used_blocks);
    return block_string;
}

// Don't modify this line unless you know what you're doing
int minixfs_virtual_path_count =
    sizeof(minixfs_virtual_path_names) / sizeof(minixfs_virtual_path_names[0]);

int minixfs_chmod(file_system *fs, char *path, int new_permissions) {
    inode * target = get_inode(fs, path); //check file exists
    if (!target) {
        errno = ENOENT; //set errno if file does not exist
        return -1; //return failure
    }

    //only the lower 9 bits of new_permissions should be set
    //thus max val is 2^9 - 1 = 511
    int max_np_val = 511;
    if (new_permissions > max_np_val) {
        return -1; //return failure (invalid arg)
    }

    //get lower 16 bits of new_permissions
    //mask upper 7 bits with zeros
    uint16_t np_masked = new_permissions & 0x01ff;

    //mask lower 9 bits of mode
    uint16_t mode_masked = target->mode & 0xfe00;

    //mode --> masked_mode | np_masked
    target->mode = mode_masked | np_masked;

    return 0;
}

int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group) {
    // Land ahoy!
    return -1;
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path) {
    // Land ahoy!
    if (!valid_filename(path) || get_inode(fs, path)) {
        return NULL;
    }

    inode_number new_inode_num = first_unused_inode(fs);
    
    if (new_inode_num == -1) {
        return NULL;
    }

    inode * new_inode = &fs->inode_root[new_inode_num];

    const char * filename;
    inode * parent_inode = parent_directory(fs, path, &filename);
    
    int parent_path_len = 0;
    char * path_dup = strdup(path);
    while (path_dup != filename) {
        parent_path_len++;
        path_dup++;
    }

    char parent_path[parent_path_len + 1];

    strncpy(parent_path, path_dup, parent_path_len);

    parent_path[parent_path_len + 1] = '\0';
    char inode_num[8];
    sprintf(inode_num, "%d", new_inode_num);
    minixfs_dirent new_dirent;
    new_dirent.inode_num = new_inode_num;
    strcpy(new_dirent.name, filename);

    char dirent_string[FILE_NAME_ENTRY];
    make_string_from_dirent(dirent_string, new_dirent);
    
    off_t offset = parent_inode->size;
    minixfs_write(fs, parent_path, dirent_string, FILE_NAME_ENTRY, &offset);

    init_inode(parent_inode, new_inode);

    //free(path_dup);
    return new_inode;
}

ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off) {
    if (!strcmp(path, "info")) {
        // TODO implement the "info" virtual file here
    }
    // TODO implement your own virtual file here
    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
    
    inode * target = get_inode(fs, path);
    printf("offset access\n");
    uint64_t offset = *off;

    printf("creating inode if it doesn't exist\n");
    if (!target) {
        target = minixfs_create_inode_for_path(fs, path);
       
    }
    
    printf("calculating size");
    uint64_t new_size;   //calculating the new filesize after write
    if (offset < target->size) {
        //if there is overlap size after write is off (preceding bytes) + count (written bytes)
        new_size = offset + count;
    } else {
        //if there is no overlap size after write is size + (off - size) + count
        new_size = target->size + (offset - target->size) + count;
    }

    printf("calculating max size\n");
    //max filesize = # direct blocks * blocksize + # indirect blocks * blocksize
    uint64_t max_size = (NUM_DIRECT_BLOCKS + NUM_INDIRECT_BLOCKS) * sizeof(data_block);
    if (new_size > max_size) {
        errno = ENOSPC;
        return -1; 
    }

    //calculate # of blocks that need to be alloc'd
    int min_blocks = new_size / sizeof(data_block);
    //handle case where min_blocks is not a multiple of block size
    if (new_size % sizeof(data_block) != 0) {
        min_blocks++;
    }
    
    int blocks_allocd = minixfs_min_blockcount(fs, path, min_blocks);
    if (blocks_allocd != 0) {
        //already checked that file exists, so only error results from no available data blocks
        errno = ENOSPC; 
        return -1;
    }

    //determine which block to start writing to
    int block_offset = offset / sizeof(data_block); //which block offset falls in
    int byte_offset = offset % sizeof(data_block); //bytes to move within block

    int blocks_written = 0;
    uint64_t bytes_written = 0;
    uint64_t remaining_bytes;
    char write_indirect = 0;

    while ((remaining_bytes = count - bytes_written)) {
        int curr_offset = block_offset + blocks_written; //update array idx
        write_indirect = curr_offset >= NUM_DIRECT_BLOCKS;
        if (write_indirect) {
            curr_offset -= NUM_DIRECT_BLOCKS; //subtract dir blocks to find indir offset
        }

        uint64_t block_bytes; 
        if (byte_offset > 0) {
            block_bytes = sizeof(data_block) - byte_offset; //account for bytes offset
            byte_offset = 0; //next block will have no byte offset
        } else {
            block_bytes = sizeof(data_block); 
        }

        //write min(remaining_bytes, block_bytes)
        uint64_t wr_bytes = (remaining_bytes >= block_bytes) ? block_bytes : remaining_bytes;
        data_block target_block;
        data_block_number target_block_num;
        printf("trying to write bytes now.../n");
        if (write_indirect) {
            //get the indirect block at data_root + indirect
            data_block indirect_block = fs->data_root[target->indirect];
            //get the block number at indirect_block + current offset
            target_block_num = indirect_block.data[curr_offset];
        } else {
            //get the block number at direct + current offset
            target_block_num = target->direct[curr_offset];
        }

        //target block is at data root + calculated idx
        target_block = fs->data_root[target_block_num];

        //accounting for byte offset
        void * target_data_addr = (void *) target_block.data + byte_offset;

        //copy data
        memcpy(target_data_addr, buf + bytes_written, wr_bytes);

        //update mtim
        clock_gettime(CLOCK_REALTIME, &target->mtim);

        //update atim
        clock_gettime(CLOCK_REALTIME, &target->atim);

        //update size
        target->size = new_size;

        //update blocks_written
        blocks_written++;

        //update bytes_written
        bytes_written += wr_bytes;
    }
    
    //update offset
    *off += bytes_written;
    //return 
    return count;
    
   return 0;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off) {
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);
    // 'ere be treasure!
    return -1;
}
