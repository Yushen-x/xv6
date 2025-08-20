// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// --- COW (Copy-On-Write) 引用计数所需变量 ---
struct spinlock ref_lock;
unsigned int ref_counts[(PHYSTOP - KERNBASE) / PGSIZE]; 

// 将物理地址转换为引用计数数组的索引
static inline unsigned int pa_to_ref_idx(void *pa) {
    return ((uint64)pa - KERNBASE) / PGSIZE;
}
// --- 变量定义结束 ---


void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&ref_lock, "ref_counts"); // 初始化引用计数的锁
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
    // 当分配新页面时，将其引用计数初始化为1
    acquire(&ref_lock);
    ref_counts[pa_to_ref_idx(r)] = 1;
    release(&ref_lock);
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}


// --- COW (Copy-On-Write) 引用计数所需函数 ---

// 增加物理页的引用计数
void
add_ref(void *pa)
{
  acquire(&ref_lock);
  ref_counts[pa_to_ref_idx(pa)]++;
  release(&ref_lock);
}

// 获取物理页的引用计数
int
get_ref(void *pa)
{
  acquire(&ref_lock);
  int count = ref_counts[pa_to_ref_idx(pa)];
  release(&ref_lock);
  return count;
}

// 减少物理页的引用计数
void
decr_ref(void *pa)
{
  acquire(&ref_lock);
  ref_counts[pa_to_ref_idx(pa)]--;
  release(&ref_lock);
}

// 用于 COW 的特殊 kfree 函数
// 它会先减少引用计数，只有当计数为0时才真正释放内存
void
kfree_cow(void *pa)
{
  if (get_ref(pa) > 0) {
    decr_ref(pa);
  }

  if (get_ref(pa) == 0) {
    kfree(pa);
  }
}
