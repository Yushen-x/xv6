#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h" // 包含 MAXARG 的定义

int main(int argc, char *argv[]) {
    char buf[512];      // 用于存储从标准输入读取的一行
    char *p = buf;      // 指向 buf 当前要写入的位置
    char *x_argv[MAXARG]; // 用于传递给 exec 的参数数组
    int x_argc = 0;

    // 1. 准备传递给 exec 的参数
    // xargs 后面的命令和参数是基础参数
    for (int i = 1; i < argc; i++) {
        x_argv[x_argc++] = argv[i];
    }

    // 2. 循环读取标准输入，一次一个字符
    while (read(0, p, 1) > 0) {
        // 如果读到换行符，说明一行已经结束
        if (*p == '\n') {
            *p = 0; // 将换行符替换为字符串结束符，完成一行的构建

            // 将从标准输入读到的这一行作为新的参数
            x_argv[x_argc] = buf;
            x_argv[x_argc + 1] = 0; // exec 的参数列表必须以空指针结尾

            // 3. 创建子进程来执行命令
            if (fork() == 0) {
                // 子进程
                exec(x_argv[0], x_argv);
                // 如果 exec 成功，这里永远不会被执行
                fprintf(2, "xargs: exec failed\n");
                exit(1);
            } else {
                // 父进程
                wait(0); // 等待子进程执行完毕
            }
            
            // 重置缓冲区指针，为读取下一行做准备
            p = buf;
        } else {
            // 如果不是换行符，继续读取下一个字符
            p++;
        }
    }
    
    // 处理最后一行可能没有换行符的情况
    if (p > buf) {
        *p = 0;
        x_argv[x_argc] = buf;
        x_argv[x_argc + 1] = 0;
        if (fork() == 0) {
            exec(x_argv[0], x_argv);
            fprintf(2, "xargs: exec failed\n");
            exit(1);
        } else {
            wait(0);
        }
    }

    exit(0);
}
