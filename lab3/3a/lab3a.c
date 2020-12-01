// NAME: Miles Kang,Michelle Zhuang
// EMAIL: milesjkang@gmail.com,michelle.zhuang@g.ucla.edu
// ID: 405106565,505143435

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include "ext2_fs.h"

/* -----Global variables----- */

#define SUPERBLOCK_OFFSET 1024
int img_fd;
int block_size, block_count, inode_count, inode_table;
struct ext2_super_block superblock;
struct ext2_group_desc group;

/* -----Global variables----- */

void error_msg(char* message, int exit_code) {
    // Generic error msg.
    fprintf(stderr, "Error %d: %s\n", errno, message);
    exit(exit_code);
}

char get_file_type(uint16_t i_mode) {
    if (i_mode == 0xA000) {
        return 's';
    }
    else if (i_mode == 0x4000) {
        return 'd';
    }
    else if (i_mode == 0x8000) {
        return 'f';
    }
    // if none of the above
    return '?';
}

void get_time(uint32_t i_time, char* string) {
    time_t rawtime = i_time;
    struct tm* info = gmtime(&rawtime);
    strftime(string, 24, "%m/%d/%y %H:%M:%S", info);
}

long find_offset(int block_num){
    return SUPERBLOCK_OFFSET + block_size*(block_num-1);
}

void log_superblock() {
    // Parses information from the superblock.
    if (pread(img_fd, &superblock, sizeof(superblock), SUPERBLOCK_OFFSET) < 0){
        error_msg("pread failure.", 2);
    }
    block_size = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
    block_count = superblock.s_blocks_count;
    inode_count = superblock.s_inodes_count;
    
    if (superblock.s_magic != EXT2_SUPER_MAGIC)
        error_msg("Failure to locate superblock.",2);
    
    printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",
           block_count,
           inode_count,
           block_size,
           superblock.s_inode_size,
           superblock.s_blocks_per_group,
           superblock.s_inodes_per_group,
           superblock.s_first_ino);
}

void log_directory_entry(int parent_inode, int block_num){
    struct ext2_dir_entry dir;
    long offset = find_offset(block_num);
    int logical_offset = 0;
    
    while (logical_offset < block_size){
        if (pread(img_fd, &dir, sizeof(dir), offset+logical_offset) < 0)
            error_msg("pread failure.", 2);
        
        int name_len = dir.name_len;
        char name[EXT2_NAME_LEN];
        strncpy(name, dir.name, name_len);
        bzero(&name[name_len], EXT2_NAME_LEN-name_len);
        
        if (dir.inode){
            printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n",
                   parent_inode,
                   logical_offset,
                   dir.inode,
                   dir.rec_len,
                   name_len,
                   name
                   );
        }
        logical_offset += dir.rec_len;
    }
}

long log_single_indirect_block(uint32_t inode, uint32_t block,
                               uint32_t curr_block, int block_index, int ptr_index, char filetype) {
    
    if (curr_block == 0) {
        return 0;
    }
    
    long logical_block_offset = 0;
    
    if (filetype == 'd') {
        log_directory_entry(inode, curr_block);
    }
    
    int original_level = block_index - 11;
    
    // evaluate logical block offset for current data block.
    
    if (original_level == 1) {
        logical_block_offset = 12 + ptr_index;
    }
    else if (original_level == 2) {
        logical_block_offset = 256 + 12 + ptr_index;
    }
    else if (original_level == 3) {
        logical_block_offset = 256 * 256 + 256 + 12 + ptr_index;
    }
    
    printf("INDIRECT,%d,%d,%ld,%d,%d\n",
           inode,
           1,
           logical_block_offset,
           block,
           curr_block
           );
    
    return logical_block_offset;
}

long log_indirect_block(uint32_t inode, uint32_t block,
                        int block_index, int level, char filetype) {
    
    int num_ptrs = block_size / 4;
    
    long offset = find_offset(block);
    uint32_t* new_block = (uint32_t*) malloc(block_size);
    
    if (pread(img_fd, new_block, block_size, offset) < 0) {
        error_msg("pread failure.", 2);
    }
    
    // Base case: log indirect blocks.
    
    if (level == 1) {
        long tmp_logical_offset = 0;
        int offset_flag = 0;
        
        for (int i = 0; i < num_ptrs; i++) {
            long tmp = log_single_indirect_block(inode, block, new_block[i], block_index, i, filetype);
            if (tmp > 0 && !offset_flag) {
                tmp_logical_offset = tmp - i;
                offset_flag = 1;
            }
        }
        return tmp_logical_offset;
    }
    
    // Recursively call one level down for each pointer in block.
    
    if (level > 1) {
        long logical_block_offset;
        int nonempty_flag = 0;
        for (int i = 0; i < num_ptrs; i++) {
            if (new_block[i] == 0) {
                continue;
            }
            logical_block_offset = log_indirect_block(inode, new_block[i], block_index, level - 1, filetype);
            nonempty_flag = 1;
        }
        
        // PRINT.
        
        if (nonempty_flag) {
            printf("INDIRECT,%d,%d,%ld,%d,%d\n",
                   inode,
                   level,
                   logical_block_offset,
                   block,
                   new_block[0]
                   );
        }
        
        return logical_block_offset;
    }
    
    return 0;
}


void log_free_block(int block){
    char* buf = (char*)malloc(block_size);
    long offset = find_offset(block);
    int block_num = 1;
    
    if (pread(img_fd, buf, block_size, offset) < 0){
        error_msg("pread failure.", 2);
    }
    
    for(int i = 0; i < block_size; i++){
        char byte = buf[i];
        for(int j = 0; j < 8; j++){
            int mask = 1 << j;
            int read = byte & mask;
            if (!read){
                printf("BFREE,%d\n",block_num);
            }
            block_num++;
        }
        if (block_num > block_count){
            break;
        }
    }
    free(buf);
}

void log_allocated_inode(int inode_num) {
    struct ext2_inode inode;
    
    long offset = find_offset(inode_table) + (inode_num - 1) * sizeof(inode);
    
    if (pread(img_fd, &inode, sizeof(inode), offset) < 0){
        error_msg("pread failure.", 2);
    }
    
    uint16_t imode = inode.i_mode, link_count = inode.i_links_count;
    uint32_t file_size = inode.i_size;
    char ctime[24], mtime[24], atime[24];
    get_time(inode.i_ctime, ctime);
    get_time(inode.i_mtime, mtime);
    get_time(inode.i_atime, atime);
    
    if (imode == 0 || link_count == 0)
        return;
    
    char file_type = get_file_type(imode & 0xF000);
    
    //print first 12 entries
    printf("INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",
           inode_num,
           file_type,
           imode & 0xFFF,
           inode.i_uid,
           inode.i_gid,
           link_count,
           ctime,mtime,atime,
           file_size,
           inode.i_blocks);
    
    //print the 15 block addresses
    //NOTE: indirect levels 1,2,3 are at block indices 12,13,14
    if (!(file_type == 's' && file_size <= 60)){
        for (int i = 0; i < 15; i++){
            printf(",%d",inode.i_block[i]);
        }
    }
    printf("\n");
    
    // don't need to trace indirect blocks for symbolic links
    if (file_type == 's')
        return;
    
    // print directory entries for up to 12 data blocks
    if (file_type == 'd'){
        for(int i = 0; i < 12; i++){
            if (inode.i_block[i]){
                log_directory_entry(inode_num, inode.i_block[i]);
            }
        }
    }
    
    log_indirect_block(inode_num, inode.i_block[12], 12, 1, file_type);
    log_indirect_block(inode_num, inode.i_block[13], 13, 2, file_type);
    log_indirect_block(inode_num, inode.i_block[14], 14, 3, file_type);
}

void log_free_inode(int block){
    char* buf = (char*)malloc(block_size);
    long offset = find_offset(block);
    int inode_num = 1;
    
    if (pread(img_fd, buf, block_size, offset) < 0){
        error_msg("pread failure.", 2);
    }
    
    for(int i = 0; i < block_size; i++){
        char byte = buf[i];
        for(int j = 0; j < 8; j++){
            int mask = 1 << j;
            int read = byte & mask;
            if (!read){
                printf("IFREE,%d\n",inode_num);
            }
            else{
                log_allocated_inode(inode_num);
            }
            inode_num++;
        }
        if (inode_num > inode_count){
            break;
        }
    }
    free(buf);
}

void log_group() {
    // Parses information from the group.
    if (pread(img_fd, &group, sizeof(group), SUPERBLOCK_OFFSET + block_size) < 0){
        error_msg("pread failure.", 2);
    }
    
    int group_num = 0;
    int block_bitmap = group.bg_block_bitmap;
    int inode_bitmap = group.bg_inode_bitmap;
    inode_table = group.bg_inode_table;
    
    printf("GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n",
           group_num,
           block_count,
           inode_count,
           group.bg_free_blocks_count,
           group.bg_free_inodes_count,
           block_bitmap,
           inode_bitmap,
           inode_table);
    
    log_free_block(block_bitmap);
    log_free_inode(inode_bitmap);
}

int main(int argc, char* argv[]) {
    // Check for bad arguments, then open file.
    if (argc != 2) {
        error_msg("Invalid number of arguments were passed.", 1);
    }
    
    if ((img_fd = open(argv[1], O_RDONLY)) < 0) {
        error_msg("Could not open img file.", 1);
    }
    
    log_superblock();
    log_group();
    
    exit(0);
}

/*
 Exit codes:
 0 . . . analysis successful
 1 . . . bad arguments
 2 . . . corruption detected or other processing errors
 */
