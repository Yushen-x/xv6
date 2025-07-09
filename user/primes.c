#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// 筛子进程的核心函数
// left_pipe_read_end 是它从左边邻居接收数字的管道读取端
void sieve(int left_pipe_read_end) {
    int p, n;

    // 尝试从左边管道读取第一个数。这个数一定是素数。
    // 如果read返回的字节数不等于一个整数的大小，说明上游管道已关闭，没有数字了。
    if (read(left_pipe_read_end, &p, sizeof(p)) != sizeof(p)) {
        close(left_pipe_read_end);
        exit(0); // 正常退出
    }

    // 打印这个素数
    printf("prime %d\n", p);

    // 创建一个新的管道，用于和右边的邻居通信
    int right_pipe[2];
    pipe(right_pipe);

    // 创建子进程，这个子进程将成为右边的邻居（下一个筛子）
    if (fork() == 0) {
        // --- 这是子进程的代码 ---
        // 1. 它不需要往右管道里写，所以关闭写端
        close(right_pipe[1]);
        // 2. 它也完全不需要和左管道打交道了，关闭
        close(left_pipe_read_end);
        // 3. 递归！它把自己变成了下一个筛子，从新创建的右管道的读端读取数字
        sieve(right_pipe[0]);
    } else {
        // --- 这是父进程（当前筛子）的代码 ---
        // 1. 它不需要从右管道里读，所以关闭读端
        close(right_pipe[0]);

        // 2. 循环地从左管道读取剩下的数字
        while (read(left_pipe_read_end, &n, sizeof(n)) == sizeof(n)) {
            // 如果这个数字不能被当前的素数p整除，
            // 就把它通过右管道传递给下一个筛子
            if (n % p != 0) {
                write(right_pipe[1], &n, sizeof(n));
            }
        }

        // 3. 左管道已经没数据了，可以关闭了
        close(left_pipe_read_end);
        // 4. 所有该传递的数字都传递完了，关闭右管道的写端
        //    这会通知下游进程：“我这里也结束了”
        close(right_pipe[1]);
        // 5. 等待自己的子进程（右邻居）完全结束后再退出
        wait(0);
        exit(0);
    }
}

// 主函数，整个程序的入口
int main(int argc, char *argv[]) {
    int p[2];
    pipe(p);

    // 创建第一个子进程
    if (fork() == 0) {
        // --- 这是第一个子进程的代码 ---
        // 1. 它作为管道的第一个筛子，不需要往初始管道里写
        close(p[1]);
        // 2. 开始筛选工作
        sieve(p[0]);
    } else {
        // --- 这是主进程（main）的代码 ---
        // 1. 它作为数字生成器，只需要往管道里写，不需要读
        close(p[0]);

        // 2. 把 2 到 35 所有的整数都写入管道
        for (int i = 2; i <= 35; i++) {
            write(p[1], &i, sizeof(i));
        }
        
        // 3. 所有数字都写完了，关闭管道的写端
        close(p[1]);

        // 4. 等待第一个子进程完成它以及它所有子孙进程的工作
        wait(0);
        exit(0);
    }
    
    return 0;
}
