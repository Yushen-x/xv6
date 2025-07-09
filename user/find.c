#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// 从路径中提取文件名
char* fmtname(char *path) {
    static char buf[DIRSIZ+1];
    char *p;

    // 找到路径中最后一个'/'字符后面的部分
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // 如果文件名太长，截断
    if(strlen(p) >= DIRSIZ)
        return p;
    
    // 将文件名复制到缓冲区并返回
    memmove(buf, p, strlen(p));
    buf[strlen(p)] = 0; // 添加字符串结束符
    return buf;
}

// 递归查找的核心函数
// path: 当前要查找的目录路径
// filename: 要查找的目标文件名
void find(char *path, char *filename) {
    char buf[512], *p;
    int fd;
    struct dirent de; // 目录项结构
    struct stat st;   // 文件状态结构

    // 尝试打开路径。如果失败，说明这不是一个目录或无法访问
    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    // 尝试获取文件状态。如果失败，说明路径有问题
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // 根据文件类型进行处理
    switch(st.type){
    // case T_FILE: 这部分逻辑在下面处理，因为find可能直接被一个文件路径调用
    //     if(strcmp(fmtname(path), filename) == 0){
    //         printf("%s\n", path);
    //     }
    //     break;
    
    case T_DIR:
        // 如果路径太长，无法拼接新的文件名，则报错退出
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
            printf("find: path too long\n");
            break;
        }
        // 构建新的路径前缀
        strcpy(buf, path);
        p = buf+strlen(buf);
        *p++ = '/';

        // 读取目录下的每一个条目
        while(read(fd, &de, sizeof(de)) == sizeof(de)){
            if(de.inum == 0) // 空的目录项，跳过
                continue;

            // 把当前目录项的名字拼接到路径后面
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0; // 添加字符串结束符

            // 再次获取拼接后完整路径的文件状态
            if(stat(buf, &st) < 0){
                printf("find: cannot stat %s\n", buf);
                continue;
            }

            // 如果是文件类型，且名字匹配，则打印路径
            if(st.type == T_FILE && strcmp(fmtname(buf), filename) == 0) {
                printf("%s\n", buf);
            }
            // 如果是目录类型，且不是 "." 或 ".."，则递归查找
            else if(st.type == T_DIR && strcmp(fmtname(buf), ".") != 0 && strcmp(fmtname(buf), "..") != 0) {
                find(buf, filename);
            }
        }
        break;
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if(argc != 3){
        // find命令需要两个参数：路径和文件名
        fprintf(2, "Usage: find <directory> <filename>\n");
        exit(1);
    }
    
    find(argv[1], argv[2]);
    
    exit(0);
}
