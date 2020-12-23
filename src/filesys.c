#include "filesys.h"
#include "disk.h"

char buf[BLOCK_SIZE];

/**
 * @brief 根据数据块号读取磁盘块, 读取内容存放到buf中
 * @return 读取失败返回-1, 成功返回0
 */
int read_block_from_disk(int block_id)
{
    int device_blocks[2];
    device_blocks[0] = block_id*2;
    device_blocks[1] = block_id*2 + 1;
    if(!disk_read_block(device_blocks[0],buf) && !disk_read_block(device_blocks[1],buf+DEVICE_BLOCK_SIZE))
    {
        return 0;
    }
    printf("fail to read block %d\n", block_id);
    return -1;
}


/**
 * @brief 读取超级块, 存放到super_block_buf中
 * @return 读取失败返回-1, 成功返回0
 */
int read_spblock_from_disk()
{

    if(read_block_from_disk(SUPER_BLOCK_INDEX)!=0)
    {
        printf("fail to read super block\n");
        return -1;
    }
    memcpy(&super_block_buf, buf, sizeof(BLOCK_SIZE));
    return 0;
}


/**
 * @brief 根据inode_id读取inode块, 并且将inode_id的值存放到inode_buf中
 * @return 读取失败返回-1, 成功返回0
 */
inode* read_inode_block_from_disk(int inode_id)
{
    int inode_block_id = inode_id / INODE_NUMS_EACH_BLOCK + 1;
    if(read_block_from_disk(inode_block_id) != 0)
    {
        printf("fail to read inode %d\n", inode_id);
        return -1;
    }
    memcpy(inode_buf, buf, BLOCK_SIZE);
    return &(inode_buf[inode_id%INODE_NUMS_EACH_BLOCK]);
}


/**
 * @brief 从磁盘中读取目录块,存放到dir_table中
 * @return 读取失败返回-1, 成功返回0
 */
int read_dir_table_from_disk(int block_id)
{
    if(read_block_from_disk(block_id) != 0)
    {
        printf("fail to read block %d\n", block_id);
        return -1;
    }
    memcpy(dir_table, buf, BLOCK_SIZE);
}


/**
 * @brief 将buf的内容写入到磁盘中
 * @return 写入成功返回0, 失败返回-1
 */
int write_block_to_disk(int block_id)
{
    int device_block[2];
    device_block[0] = block_id*2;
    device_block[1] = block_id*2 + 1;
    if(!disk_write_block(device_block[0], buf) && !disk_write_block(device_block[1], buf+DEVICE_BLOCK_SIZE))
    {
        return 0;
    }
    return -1;
}


/**
 * @brief 将超级块写入到磁盘中
 * @return 写入成功返回0, 失败返回-1
 */
int write_spblock_to_disk()
{
    memcpy(buf, &super_block_buf, BLOCK_SIZE);
    if(!write_block_to_disk(SUPER_BLOCK_INDEX))
        return 0;
    printf("fail to write superblock to disk\n");
    return -1;
}


/**
 * @brief 将inode块写入到磁盘中
 * @return 写入成功返回0, 失败返回-1
 */
int write_inode_block_to_disk(int inode_id)
{
    int block_id = inode_id/INODE_NUMS_EACH_BLOCK + 1;
    memcpy(buf, inode_buf, BLOCK_SIZE);
    if(!write_block_to_disk(block_id))
        return 0;
    printf("fail to write superblock to disk\n");
    return -1;
}


/**
 * @brief 将dir_table块写入到磁盘中
 * @return 写入成功返回0, 失败返回-1
 */
int write_dir_table_to_disk(int block_id)
{
    memcpy(buf, dir_table, BLOCK_SIZE);
    if(!write_block_to_disk(block_id))
        return 0;
    printf("fail to write superblock to disk\n");
    return -1;
}


/**
 * @brief 
 * @return 
 */
int filesys_init()
{
    read_spblock_from_disk();
    if(super_block_buf.magic_num == SYS_MAGIC_NUM)
        return 0; 
    else
    {
        super_block_buf.magic_num = SYS_MAGIC_NUM;
        super_block_buf.free_block_count = 4062;
        super_block_buf.free_inode_count = 1023;
        super_block_buf.dir_inode_count = 1;
        memset(super_block_buf.block_map, 0, 128);
        memset(super_block_buf.inode_map, 0, 32);
        super_block_buf.block_map[0] = ~0;
        super_block_buf.block_map[1] = 0xc0000000;
        super_block_buf.inode_map[0] = 0x80000000;

        inode* root_inode = read_inode_block_from_disk(0);
        root_inode->size = 1;
        root_inode->file_type = TYPE_FOLDER;
        root_inode->link = 0;
        root_inode->block_point[0] = 33;
        write_inode_block_to_disk(0);

        read_dir_table_from_disk(33);
        dir_table[0].inode_id = 0;
        dir_table[0].valid = DIR_VALID;
        dir_table[0].type = TYPE_FOLDER;
        strcpy(dir_table->name, ".");
    }
    return 0;
}


void shutdown()
{
    printf("shutdown the file system ...\n");
    if(close_disk() >= 0)
    {
        printf("success to shutdown the file system\n");
    }
    else
    {
        pritnf("fail to shutdown the file system\n");
    }
    
}