#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  // 1. 检查命令行参数数量
  // argc 是参数的个数，argv 是参数的字符串数组
  // 运行 "sleep 10"，argc 会是 2 (程序名 "sleep" 和参数 "10")
  if(argc != 2){
    // 如果参数不对，打印错误信息到标准错误输出
    fprintf(2, "Usage: sleep ticks\n");
    exit(1); // 以错误状态码 1 退出
  }

  // 2. 将字符串参数转换为整数
  // argv[1] 就是 "10" 这个字符串
  int ticks = atoi(argv[1]);

  // 3. 调用系统调用 sleep
  sleep(ticks);

  // 4. 正常退出
  exit(0);
}
