#include "disk.h"

void main()
{
    if(open_disk()!=0)
    {
        printf("fail to open the disk\n");
        return;
    }
}