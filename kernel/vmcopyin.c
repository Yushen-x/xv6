#include "param.h"
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "spinlock.h"
#include "proc.h"

//
// This file contains copyin_new() and copyinstr_new(), the
// replacements for copyin and coyinstr in vm.c.
//

static struct stats {
  int ncopyin;
  int ncopyinstr;
} stats;

int
statscopyin(char *buf, int sz) {
  int n;
  n = snprintf(buf, sz, "copyin: %d\n", stats.ncopyin);
  n += snprintf(buf+n, sz, "copyinstr: %d\n", stats.ncopyinstr);
  return n;
}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int
copyinstr_new(char *dst, uint64 srcva, uint64 max)
{
  stats.ncopyinstr++;
  struct proc *p = myproc();
  int got_null = 0;
  
  for(int i = 0; i < max; i++){
    // 在循环的每一步都检查地址！
    if(srcva + i >= p->sz) {
      return -1;
    }
    
    char c = *(char*)(srcva + i);
    dst[i] = c;
    if(c == 0){
      got_null = 1;
      break;
    }
  }
  
  if(!got_null)
    return -1;
    
  return 0;
}

int
copyin_new(char *dst, uint64 srcva, uint64 len)
{
  stats.ncopyin++;
  struct proc *p = myproc();
  

  if (srcva >= p->sz || srcva + len > p->sz || srcva + len < srcva) {
    return -1;
  }
  
  memmove(dst, (void*)srcva, len);
  return 0;
}
