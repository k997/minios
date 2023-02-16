#include "fs.h"
#include "debug.h"
#include "string.h"

static char *path_parse(char *path, char *name_buf)
{

    if (path[0] == '/')
    {
        // 连续多个 '/'
        while (*(++path) == '/')
            ;
    }
    // 路径 "/a/b"" 之间的的一段
    while (*path != '/' && *path != 0)
    {
        *name_buf++ = *path++;
    }
    // path 全部解析完成
    if (path[0] == 0)
    {
        return NULL;
    }

    // 返回未解析完的部分
    return path;
}

uint32_t path_depth(char *path)
{
    ASSERT(path != NULL);
    char *p = path;
    char name[MAX_FILE_NAME_LEN];
    uint32_t depth = 0;

    p = path_parse(p, name);
    while (name[0])
    {
        depth++;
        memset(name, 0, MAX_FILE_NAME_LEN);
        if (p)
            p = path_parse(p, name);
    }
    return depth;
}
/* 搜索文件 path，若找到则返回其 inode 号，否则返回-1 */
int search_file(const char *path, path_search_record *record)
{
    // 根目录
    if (!strcmp(path, "/") || !strcmp(path, "/.") || !strcmp(path, "/.."))
    {
        record->parent_dir = &root_dir;
        record->f_type = FS_DIRECTORY;
        record->searched_path[0] = 0; // 搜索路径置空
        return 0;
    }

    {
        uint32_t path_len = strlen(path);
        ASSERT(path[0] == '/' && path_len > 1 && path_len < MAX_PATH_LEN);
    }

    char name_buf[MAX_FILE_NAME_LEN];
    dir_entry dir_entry_buf;
    dir *cur_dir = &root_dir;         // 从根目录开始查找
    uint32_t parent_dir_inode_nr = 0; // 从根目录 inode_nr
    char *subpath = path_parse((char *)path, name_buf);

    while (name_buf[0])
    {

        ASSERT(strlen(record->searched_path) < MAX_PATH_LEN); // searched_path 不能超过 MAX_PATH_LEN
        // 记录查找过的路径
        strcat(record->searched_path, "/"); // 路径分隔符
        strcat(record->searched_path, name_buf);

        bool found = search_dir_entry(cur_part, cur_dir, name_buf, &dir_entry_buf);
        if (!found)
            return -1; /* 查找失败，返回 -1
                       找不到目录项时，要留着 parent_dir 不要关闭， 若是创建新文件的话需要在 parent_dir 中创建
                       */
        // 当前 sub path 查找成功，继续查找
        memset(name_buf, 0, MAX_FILE_NAME_LEN);
        if (subpath)
            subpath = path_parse(subpath, name_buf);

        if (dir_entry_buf.f_type == FS_DIRECTORY)
        {
            parent_dir_inode_nr = cur_dir->inode->i_nr;       // 当前路径下查找 subpath 成功，设置当前路径为父路径
            dir_close(cur_dir);                               // 关闭 dir 防止内存泄漏
            cur_dir = dir_open(cur_part, dir_entry_buf.i_nr); // 当前路径更新为 subdir
            record->parent_dir = cur_dir;
            continue;
        }
        else if (dir_entry_buf.f_type == FS_FILE)
        {
            record->f_type = FS_FILE;
            return dir_entry_buf.i_nr;
        }
    }
    /* 执行到此，必然是遍历了完整路径,
并且pathname 的最后一层路径不是普通文件，而是和 file 同名的目录
此时record->parent_dir 指向同名目录，必须返回该目录的上一级目录 */
    dir_close(record->parent_dir);

    record->parent_dir = dir_open(cur_part, parent_dir_inode_nr);
    record->f_type = FS_DIRECTORY;
    // 返回 file 同名目录的 i_nr
    return dir_entry_buf.i_nr;
}