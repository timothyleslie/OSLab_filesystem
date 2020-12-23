#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define SYS_MAGIC_NUM  180110318
#define BLOCK_SIZE 1024
#define SUPER_BLOCK_INDEX 0 //super block放在第0块
#define INODE_BLOCK_INDEX 1 //inode 的起始块号为1
#define INODE_BLOCK_NUMS 32 //inode总共有32个块
#define INODE_NUMS_EACH_BLOCK 32 //每个数据块里面有32个inode
#define DIR_ITEMS_EACH_BLOCK  8
#define TYPE_FOLDER 0
#define TYPE_FILE   1

#define DIR_VALID 1
#define DIR_INVALID 0

typedef unsigned   uint32_t;

typedef struct super_block {
    int32_t magic_num;                  // 幻数
    int32_t free_block_count;           // 空闲数据块数
    int32_t free_inode_count;           // 空闲inode数
    int32_t dir_inode_count;            // 目录inode数
    uint32_t block_map[128];            // 数据块占用位图
    uint32_t inode_map[32];             // inode占用位图
} sp_block;


typedef struct inode {
    uint32_t size;              // 文件大小
    uint16_t file_type;         // 文件类型（文件/文件夹）
    uint16_t link;              // 连接数
    uint32_t block_point[6];    // 数据块指针
} inode;


typedef struct dir_item {               // 目录项一个更常见的叫法是 dirent(directory entry)
    uint32_t inode_id;          // 当前目录项表示的文件/目录的对应inode
    uint16_t valid;             // 当前目录项是否有效 
    uint8_t type;               // 当前目录项类型（文件/目录）
    char name[121];             // 目录项表示的文件/目录的文件名/目录名
}dir_item;

sp_block super_block_buf;
inode inode_buf[INODE_NUMS_EACH_BLOCK];
dir_item dir_table[DIR_ITEMS_EACH_BLOCK];

void filesys_init();
void ls(char *path);
void mkdir(char *path);
void torch(char *path);
void copy(char *dest, char *src);
int get_free_inode();
int get_free_block(int block_num, int* blocks_index);
void shutdown();