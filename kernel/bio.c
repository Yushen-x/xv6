// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

// kernel/bio.c
#define NBUK 13 // 桶的数量
#define hash(dev, blockno) ((dev * blockno) % NBUK) // 哈希函数

struct bucket {
  struct spinlock lock;
  struct buf head; // 当前桶的链表头
};


struct {
  // struct spinlock lock; // 旧的全局锁，可以保留或按需使用
  struct buf buf[NBUF];

  // a list of all buffers, sorted by how recently they were used.
  // a circular doubly-linked list.
  // struct buf head; //

  struct bucket buckets[NBUK]; // a cache has 13 buckets
} bcache;

// kernel/bio.c
void
binit(void)
{
  struct buf *b;
  struct buf *prev_b;
  // initlock(&bcache.lock, "bcache"); // 你的代码中保留了它

  // Initialize bucket locks and create a single list of all buffers
  // initially in the first bucket.
  for(int i = 0; i < NBUK; i++){
    initlock(&bcache.buckets[i].lock, "bcache.bucket");
    bcache.buckets[i].head.next = (void*)0; 
    if (i == 0){
      prev_b = &bcache.buckets[i].head;
      for(b = bcache.buf; b < bcache.buf + NBUF; b++){
        // The original 'prev' pointer is no longer needed for LRU.
        // b->prev = prev_b;
        prev_b->next = b;
        b->timestamp = ticks; 
        initsleeplock(&b->lock, "buffer");
        prev_b = b;
      }
      // Ensure the last buffer's next is null.
      prev_b->next = (void*)0;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
// kernel/bio.c
// kernel/bio.c
// kernel/bio.c
// kernel/bio.c
// kernel/bio.c
// kernel/bio.c
// kernel/bio.c
// kernel/bio.c
// kernel/bio.c
// kernel/bio.c
// kernel/bio.c
// kernel/bio.c
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int buk_id = hash(dev, blockno); 
  acquire(&bcache.buckets[buk_id].lock);  
  b = bcache.buckets[buk_id].head.next; 
  while(b){ 
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.buckets[buk_id].lock);
      acquiresleep(&b->lock);
      return b;   
    }
    b = b->next;
  }
  release(&bcache.buckets[buk_id].lock);

  // --- 以下是你写的寻找和窃取逻辑，我们只修正编译错误 ---
  int max_timestamp = 0; 
  int lru_buk_id = -1;
  int is_better = 0;
  struct buf *lru_b = 0;      // FIX: Correct pointer initialization
  struct buf *prev_lru_b = 0; // FIX: Correct pointer initialization
  struct buf *prev_b = 0;     // FIX: Correct pointer initialization

  for(int i = 0; i < NBUK; i++){
    prev_b = &bcache.buckets[i].head;
    acquire(&bcache.buckets[i].lock);
    while(prev_b->next){
      if(prev_b->next->refcnt == 0 && prev_b->next->timestamp >= max_timestamp){ 
        max_timestamp = prev_b->next->timestamp;
        is_better = 1;
        prev_lru_b = prev_b;
      }
      prev_b = prev_b->next;
    }

    if(is_better){
      if(lru_buk_id != -1)
        release(&bcache.buckets[lru_buk_id].lock);
      lru_buk_id = i;
    } else { // FIX: Added braces here for clarity, though not strictly required by compiler
      release(&bcache.buckets[i].lock);
    }
    is_better = 0; // FIX: Corrected the indentation. This line is now outside the if/else.
  }

  // It's possible no recyclable buffer was found if all are referenced.
  if (prev_lru_b == 0) {
     panic("bget: no buffers");
  }

  lru_b = prev_lru_b->next; 
  if(lru_b){
    prev_lru_b->next = prev_lru_b->next->next;
    release(&bcache.buckets[lru_buk_id].lock);
  }

  // FIX: DELETED the non-existent bcache.lock acquire call.
  
  acquire(&bcache.buckets[buk_id].lock);
  if(lru_b){
    lru_b->next = bcache.buckets[buk_id].head.next;
    bcache.buckets[buk_id].head.next = lru_b;
  }

  b = bcache.buckets[buk_id].head.next; 
  while(b){ 
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.buckets[buk_id].lock);
      // FIX: DELETED the non-existent bcache.lock release call.
      acquiresleep(&b->lock);
      return b;   
    }
    b = b->next;
  }

  if (lru_b == 0)
    panic("bget: no buffers 2");

  lru_b->dev = dev;
  lru_b->blockno = blockno;
  lru_b->valid = 0;
  lru_b->refcnt = 1;
  release(&bcache.buckets[buk_id].lock);
  // FIX: DELETED the non-existent bcache.lock release call.
  acquiresleep(&lru_b->lock);
  return lru_b;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
  releasesleep(&b->lock);
  int buk_id = hash(b->dev, b->blockno);
  acquire(&bcache.buckets[buk_id].lock);
  b->refcnt--;
  if(b->refcnt == 0)
    b->timestamp = ticks; 
  release(&bcache.buckets[buk_id].lock);
}

void
bpin(struct buf *b) {
  int buk_id = hash(b->dev, b->blockno);
  acquire(&bcache.buckets[buk_id].lock);
  b->refcnt++;
  release(&bcache.buckets[buk_id].lock);
}

void
bunpin(struct buf *b) {
  int buk_id = hash(b->dev, b->blockno);
  acquire(&bcache.buckets[buk_id].lock);
  b->refcnt--;
  release(&bcache.buckets[buk_id].lock);
}


