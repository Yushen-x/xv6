#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[]) {
    char line_buf[512];     // 用于存储从标准输入读取的一行
    char *p = line_buf;
    char *exec_argv[MAXARG]; // 用于构建传递给 exec 的参数数组
    int i;

    // 检查 xargs 至少要跟一个命令
    if (argc < 2) {
        fprintf(2, "usage: xargs <command> [initial-args...]\n");
        exit(1);
    }

    // 1. 预先准备好要执行的命令及其初始参数
    //    这些参数是 xargs 后面跟着的，即 argv[1], argv[2], ...
    //    我们将它们复制到新的参数数组 exec_argv 中。
    for (i = 1; i < argc; i++) {
        exec_argv[i - 1] = argv[i];
    }

    // 2. 循环从标准输入（fd=0）读取内容，一次一个字符，直到读完
    while (read(0, p, 1) > 0) {
        // 如果读到换行符，说明一行已经结束
        if (*p == '\n') {
            *p = 0; // 将换行符替换为字符串结束符，完成一行的构建

            // 将从标准输入读到的这一行，作为最后一个参数追加到 exec_argv
            exec_argv[argc - 1] = line_buf;
            // exec 的参数列表必须以空指针结尾
            exec_argv[argc] = 0;

            // 3. 创建子进程来执行命令
            if (fork() == 0) {
                // 子进程中：
                // 执行命令。第一个参数是可执行程序名（即 argv[1]），
                // 第二个参数是完整的参数列表。
                exec(exec_argv[0], exec_argv);
                
                // 如果 exec 成功，这里永远不会被执行
                fprintf(2, "xargs: exec %s failed\n", exec_argv[0]);
                exit(1);
            } else {
                // 父进程中：
                wait(0); // 等待子进程执行完毕
            }
            
            // 重置缓冲区指针，为读取下一行做准备
            p = line_buf;
        } else {
            // 如果不是换行符，继续将字符存入缓冲区
            p++;
        }
    }
    
    // 这个分支处理最后一行可能没有换行符的情况
    if (p > line_buf) {
        *p = 0;
        exec_argv[argc - 1] = line_buf;
        exec_argv[argc] = 0;
        if (fork() == 0) {
            exec(exec_argv[0], exec_argv);
            fprintf(2, "xargs: exec %s failed\n", exec_argv[0]);
            exit(1);
        } else {
            wait(0);
        }
    }

    exit(0);
}
