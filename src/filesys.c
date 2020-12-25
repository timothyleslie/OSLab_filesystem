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
        // init super_block
        super_block_buf.magic_num = SYS_MAGIC_NUM; //180110318
        super_block_buf.free_block_count = 4062; //4096-32-1-1
        super_block_buf.free_inode_count = 1023; //1024-1
        super_block_buf.dir_inode_count = 1;
        memset(super_block_buf.block_map, 0, 128);
        memset(super_block_buf.inode_map, 0, 32);
        super_block_buf.block_map[0] = ~0;
        super_block_buf.block_map[1] = 0xc0000000;
        super_block_buf.inode_map[0] = 0x80000000;
        write_spblock_to_disk();

        // init inode block
        inode* root_inode = read_inode_block_from_disk(0);
        root_inode->size = 1;
        root_inode->file_type = TYPE_FOLDER;
        root_inode->link = 0;
        root_inode->block_point[0] = 33;
        write_inode_block_to_disk(0);

        //init root data block
        read_dir_table_from_disk(33);
        dir_table[0].inode_id = 0;
        dir_table[0].valid = DIR_VALID;
        dir_table[0].type = TYPE_FOLDER;
        strcpy(dir_table->name, ".");
    }
    return;
}


/**
 * @brief 关闭文件系统
 * @return 
 */
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


/**
 * @brief 获取空闲inode
 * @return 成功初始化返回inode的id,失败返回-1
 */
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


/**
 * @brief 获取空闲块, block_num为要获取的块数, 获得的block_id存到block_index中
 * @return 成功返回0, 失败返回-1
 */
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


/**
 * @brief 找到上一级目录的inode_id
 * @return 成功初始化返回inode_id, 失败返回-1
 */
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
                // printf("Directory doesn't exist\n");
                name[j] = '\0';
                return -1;
            }
        }

        i++;
    }
    name[j] = '\0';
    return inode_id; 
}


/**
 * @brief 找到当前目录的inode_id
 * @return 成功初始化返回inode_id, 失败返回-1
 */
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
                name[j] = '\0';
                return -1;
            }
        }

        i++;
    }
    name[j] = '\0';
    return inode_id; 
}


/**
 * @brief 找到file的inode_id
 * @return 成功初始化返回inode_id, 失败返回-1
 */
int find_cur_file(char *path, char *name)
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

        if(path[i]=='/' &&path[i+1]!='\0')
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
                // printf("Directory %s doesn't exist\n", name);
                return -1;
            }
        }

        // 最后一级
        if(path[i+1] == '\0')
        {
            name[j] = 0;
            inode* inode_tmp = read_inode_block_from_disk(inode_id);
            int success = 0;
            for(int k=0; k<inode_tmp->size; k++)
            {
                read_dir_table_from_disk(inode_tmp->block_point[k]);
                for(int l=0; l<8; l++)
                {
                    if(dir_table[l].valid==DIR_VALID
                        && dir_table[l].type==TYPE_FILE
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
                name[j] = '\0';
                return -1;
            }
        }

        i++;
    }
    name[j] = '\0';
    return inode_id; 
}


/**
 * @brief 打印出path中的文件和文件夹
 * @return 
 */
void ls(char *path)
{
    char name[121];
    memset(name, 0, 121);

    //找到path文件夹对应的inode_id
    int inode_id = find_cur_path(path, name);
    if(inode_id < 0)
    {
        printf("Folder %s is not exist\n", name);
        return;
    }

    printf(".\n");
    printf("..\n");

    //根据inode_id读取path文件夹对应的inode
    inode* inode_path = read_inode_block_from_disk(inode_id);

    //遍历inode_path的block_point指向的block
    //打印block中存储的文件和文件夹的名字
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


/**
 * @brief 创建新的文件夹
 * @return 成功则返回文件夹的inode_id,否则返回-1
 */
int mkdir(char *path)
{
    char name[121];
    memset(name, 0, 121);

    // 检查文件夹是否已经存在
    if(find_cur_path(path, name) >= 0)
    {
        printf("Folder %s is already exist\n", path);
        return -1;
    }

    memset(name, 0, 121);
    int inode_id = find_prev_path(path, name);
    if(inode_id < 0)
    {
        printf("Folder %s doesn't exist\n", name);
        return -1;
    }
    if(inode_id<0 || name[0]=='\0')
        return -1;
    
    // 找到上一级目录对应的inode
    inode* inode_path = read_inode_block_from_disk(inode_id);
    int block_id;
    int success = 0;
    for(int i=1; i<6; i++)
    {
        if(inode_path->block_point[i] == 0)
        {
            //如果有空闲的block_point，则为目标文件夹申请block
            //并且让空闲的block_point指向目标文件夹的block
            get_free_block(1, &block_id);
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
        return -1;
    }

    //为目标文件夹申请inode
    int inode_new_id = get_free_inode();

    //设置目标文件夹的block
    read_dir_table_from_disk(block_id);
    dir_table[0].inode_id = inode_new_id;
    dir_table[0].valid = DIR_VALID;
    dir_table[0].type = TYPE_FOLDER;
    strcpy(dir_table[0].name, name);
    write_dir_table_to_disk(block_id);

    //设置目标文件夹的inode
    inode* inode_new = read_inode_block_from_disk(inode_new_id);
    inode_new->size += 1;
    inode_new->file_type = TYPE_FOLDER;
    inode_new->link += 1;
    inode_new->block_point[0] = 0;
    write_inode_block_to_disk(inode_new_id);
    return inode_new_id;
}


/**
 * @brief 创建文件
 * @return 成功初始化返回文件的inode_id, 失败返回-1
 */
int touch(char *path)
{
    char name[121];
    memset(name, 0, 121);

    // 检查文件是否已经存在
    if(find_cur_file(path, name) >= 0)
    {
        printf("file %s is already exist\n", path);
        return -1;
    }

    memset(name, 0, 121);
    int inode_id = find_prev_path(path, name);
    if(inode_id < 0)
    {
        printf("File %s doesn't exist\n", name);
        return -1;
    }
    if(inode_id<0 || name[0]=='\0')
        return -1;
    
    inode* inode_path = read_inode_block_from_disk(inode_id);

    int block_id;
    int success = 0;
    for(int i=1; i<6; i++)
    {
        //如果有空闲的block_point，则为目标文件申请block
        //并且让空闲的block_point指向目标文件的block
        if(inode_path->block_point[i] == 0)
        {
            get_free_block(1, &block_id);
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
        return -1;
    }

    //为目标文件申请inode
    int inode_new_id = get_free_inode();

    //设置目标文件的block
    read_dir_table_from_disk(block_id);
    dir_table[0].inode_id = inode_new_id;
    dir_table[0].valid = DIR_VALID;
    dir_table[0].type = TYPE_FILE;
    strcpy(dir_table[0].name, name);
    write_dir_table_to_disk(block_id);

    //设置目标文件的inode
    inode* inode_new = read_inode_block_from_disk(inode_new_id);
    inode_new->size += 1;
    inode_new->file_type = TYPE_FILE;
    inode_new->link += 1;
    inode_new->block_point[0] = 0;
    write_inode_block_to_disk(inode_new_id);
    return inode_new_id;
}


/**
 * @brief 将src文件复制到dest文件中
 * @return 成功初始化返回inode_id, 失败返回-1
 */
void copy(char *dest, char *src)
{
    char src_name[121];
    int src_inode_id = find_cur_file(src, src_name);
    if(src_inode_id < 0)
    {
        printf("%s is not exist\n", src_name);
        return;
    }
    inode* tmp_inode = read_inode_block_from_disk(src_inode_id);
    inode src_inode = *tmp_inode;
    if(src_inode.file_type != TYPE_FILE)
    {
        printf("%s is not a file\n", src_name);
        return ;
    }

    char dest_name[121];
    memset(dest_name, 0, 121);
    int dest_inode_id;
    // 检测dest文件是否已经存在,如果没有则新建一个
    if((dest_inode_id = find_cur_file(dest, dest_name)) < 0)
    {
        dest_inode_id = touch(dest);
    }
    
    //获取dest文件的inode
    char tmp[BLOCK_SIZE];
    inode dest_inode;
    memcpy(&dest_inode, read_inode_block_from_disk(dest_inode_id), sizeof(inode));

    //复制src_inode的内容给dest_inode
    dest_inode.size = src_inode.size;
    dest_inode.link = src_inode.link;
    dest_inode.file_type = TYPE_FILE;

    //遍历src_inode的block, 复制给dest_inode
    for(int i=0; i<6; i++)
    {
        if(src_inode.block_point[i] != 0)
        {
            int dest_block_index;
            read_block_from_disk(src_inode.block_point[i]);
            memcpy(tmp, buf, BLOCK_SIZE);
            get_free_block(1, &dest_block_index);
            memcpy(buf, tmp, BLOCK_SIZE);
            write_block_to_disk(dest_block_index);
            
            dest_inode.block_point[i] = dest_block_index;
        }
        else
        {
            dest_inode.block_point[i] = 0;
        }
    }
    write_inode_block_to_disk(dest_inode_id);
}