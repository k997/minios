#include "fs.h"

void fs_init()
{

    format_all_partition();
    char default_part[IDE_NAME_LEN] = "sdb0";
    mount_partition(default_part);

}