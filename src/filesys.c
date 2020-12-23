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
    memcpy(&super_block_buf, buf, BLOCK_SIZE);
    return 0;
}


/**
 * @brief 根据inode_id读取inode块, 并且将inode_id的值存放到inode_buf中
 * @return 读取失败返回NULL, 成功返回inode_id对应的inode
 */
inode* read_inode_block_from_disk(int inode_id)
{
    int inode_block_id = inode_id / INODE_NUMS_EACH_BLOCK + 1;
    if(read_block_from_disk(inode_block_id) != 0)
    {
        printf("fail to read inode %d\n", inode_id);
        return NULL;
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
    {
        return 0;
    }
    
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
 * @brief 初始化文件系统
 * @return 成功初始化返回0
 */
void filesys_init()
{
    read_spblock_from_disk();
    if(super_block_buf.magic_num == SYS_MAGIC_NUM)
        return ; 
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
        write_spblock_to_disk();

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
    return;
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
        printf("fail to shutdown the file system\n");
    }
    exit(0);
}


int get_free_inode()
{
    read_spblock_from_disk();
    if(super_block_buf.free_inode_count==0)
    {
        printf("No more inodes\n");
        return -1;
    }

    int mask;
    for(int i=0; i<32; i++)
    {
        mask = 0x80000000;
        for(int j=0; j<32; j++)
        {
            if(mask & super_block_buf.inode_map[i])
            {
                mask >>= 1;
                continue;
            }
            else
            {
                super_block_buf.inode_map[i] |= mask;
                super_block_buf.free_inode_count -= 1;
                write_spblock_to_disk();
                return i*32+j;
            }    
        }
    }
}


int get_free_block(int block_num, int* blocks_index)
{
    read_spblock_from_disk();
    if(super_block_buf.free_block_count < block_num)
    {
        printf("No enough free blocks\n");
        return -1;
    }

    int mask;
    for(int i=0; i<128; i++)
    {
        mask = 0x80000000;
        for(int j=0; j<32; j++)
        {
            if(mask & super_block_buf.block_map[i])
            {
                mask >>= 1;
                continue;
            }
            else
            {
                super_block_buf.block_map[i] |= mask;
                super_block_buf.free_block_count -= 1;
                *blocks_index = i*32+j;
                block_num -= 1;
                if(block_num ==0)
                {
                    write_spblock_to_disk();
                    return 0;
                }
                else
                {
                    blocks_index ++;
                }  
            }
            
        }
    }
}


int find_prev_path(char *path, char *name)
{
    int i=0;
    int j=0;
    int inode_id=0;

    i = path[0]=='/' ? 1:0;
    while(path[i] != '\0')
    {
        if(path[i]!='/')
        {
            name[j++] = path[i];
        }

        if(path[i]=='/' && path[i+1]=='\0')
        {
            name[j] = '\0';
            break;
        }

        if(path[i]=='/'&&path[i+1]!='\0')
        {
            name[j] = '\0';
            j=0;
            inode* inode_tmp = read_inode_block_from_disk(inode_id);
            int success = 0;
            for(int k=0; k<inode_tmp->size; k++)
            {
                read_dir_table_from_disk(inode_tmp->block_point[k]);
                for(int l=0; l<8; l++)
                {
                    if(dir_table[l].valid==DIR_VALID
                        && dir_table[l].type==TYPE_FOLDER
                        && !strcmp(dir_table[l].name, name))
                        {
                            inode_id = dir_table[l].inode_id;
                            success = 1;
                            k = inode_tmp->size;
                        }
                }
            }

            if(!success)
            {
                printf("Directory doesn't exist\n");
                return -1;
            }
        }

        i++;
    }
    return inode_id; 
}


int find_cur_path(char *path, char *name)
{
    int i=0;
    int j=0;
    int inode_id=0;

    i = path[0]=='/' ? 1:0;
    while(path[i] != '\0')
    {
        if(path[i]!='/')
        {
            name[j++] = path[i];
        }

        if(path[i]=='/' || path[i+1]=='\0')
        {
            name[j] = '\0';
            j=0;
            inode* inode_tmp = read_inode_block_from_disk(inode_id);
            int success = 0;
            for(int k=0; k<inode_tmp->size; k++)
            {
                read_dir_table_from_disk(inode_tmp->block_point[k]);
                for(int l=0; l<8; l++)
                {
                    if(dir_table[l].valid==DIR_VALID
                        && dir_table[l].type==TYPE_FOLDER
                        && !strcmp(dir_table[l].name, name))
                        {
                            inode_id = dir_table[l].inode_id;
                            success = 1;
                            k = inode_tmp->size;
                        }
                }
            }

            if(!success)
            {
                printf("Directory doesn't exist\n");
                return -1;
            }
        }

        i++;
    }
    return inode_id; 
}


void ls(char *path)
{
    char name[121];
    memset(name, 0, 121);
    uint32_t inode_id = find_cur_path(path, name);
    inode* inode_path = read_inode_block_from_disk(inode_id);
    for(int i=0; i<inode_path->size; i++)
    {
        read_dir_table_from_disk(inode_path->block_point[i]);
        for(int j=0; j<8; j++)
        {
            if(dir_table[j].valid==DIR_VALID)
            {
                if(dir_table[j].name[0]=='\0')
                    continue;
                else
                    printf("%s\n",dir_table[j].name);         
            }
        }
    }
}


void mkdir(char *path)
{
    char name[121];
    memset(name, 0, 121);
    uint32_t inode_id = find_prev_path(path, name);
    if(inode_id<0 || name[0]=='\0')
        return;
    
    inode* inode_path = read_inode_block_from_disk(inode_id);
    //这里每个新目录都放到一个新的block里面,可以改进
    int block_id;
    get_free_block(1, &block_id);

    int success = 0;
    for(int i=1; i<6; i++)
    {
        if(inode_path->block_point[i] == 0)
        {
            inode_path->block_point[i] = block_id;
            inode_path->size++ ;
            write_inode_block_to_disk(inode_id);
            success = 1;
            break;
        }
    }

    if(!success)
    {
        printf("no more block pointer for dir %s\n", path);
        return;
    }

    uint32_t inode_new_id = get_free_inode();

    read_dir_table_from_disk(block_id);
    dir_table[0].inode_id = inode_new_id;
    dir_table[0].valid = DIR_VALID;
    dir_table[0].type = TYPE_FOLDER;
    strcpy(dir_table[0].name, name);
    write_dir_table_to_disk(block_id);

    inode* inode_new = read_inode_block_from_disk(inode_new_id);
    inode_new->size += 1;
    inode_new->file_type = TYPE_FOLDER;
    inode_new->link += 1;
    inode_new->block_point[0] = 0;
    write_inode_block_to_disk(inode_new_id);
}


void torch(char *path)
{

}
void copy(char *dest, char *src)
{

}