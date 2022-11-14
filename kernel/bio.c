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
#include <limits.h>
#include <stddef.h>
#define NBUCKET 13
#define NNBUF 15
struct {
  struct spinlock lock;
  struct buf buf[NNBUF*NBUCKET];
  struct spinlock bucketlk[NBUCKET];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head[NBUCKET];
} bcache;
extern uint ticks;
void
binit(void)
{
  struct buf *b;

  
  for(int i=0;i<NBUCKET;++i){
    initlock(&bcache.bucketlk[i],"bcache.bucket");
      // Create linked list of buffers
    // bcache.head[i].prev = &bcache.head[i];
    // bcache.head[i].next = &bcache.head[i];
  }
  initlock(&bcache.lock, "bcache");
  for(b = bcache.buf; b < bcache.buf+NNBUF*NBUCKET; b++){
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    b->ts=ticks;
    initsleeplock(&b->lock, "buffer");
    // bcache.head.next->prev = b;
    // bcache.head.next = b;
    // printf("ts:%d\n",ticks);
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bucket_num=blockno%NBUCKET;
  // acquire(&bcache.lock);
  acquire(&bcache.bucketlk[bucket_num]);
  // Is the block already cached?
  for(int i=bucket_num;i<NNBUF*NBUCKET;i+=NBUCKET){
    b=&bcache.buf[i];
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      // release(&bcache.lock);
      release(&bcache.bucketlk[bucket_num]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  struct buf *lru_b=NULL;
  uint minn_ts=INT_MAX;
  for(int i=bucket_num;i<NNBUF*NBUCKET;i+=NBUCKET){
    // acquire(&bcache.lock[i%NBUCKET]);
    b=&bcache.buf[i];
    if(b->refcnt==0&&b->ts<minn_ts){
      minn_ts=b->ts;
      lru_b=b;
    }
    // release(&bcache.lock[i%NBUCKET]);
  }
  if(lru_b){
    lru_b->dev = dev;
    lru_b->blockno = blockno;
    lru_b->valid = 0;
    lru_b->refcnt = 1;
    lru_b->ts=ticks;
    // lru_b->next = bcache.head[bucket_num].next;
    // lru_b->prev = &bcache.head[bucket_num];
    // bcache.head[bucket_num].next->prev = lru_b;
    // bcache.head[bucket_num].next = lru_b;
    // release(&bcache.lock);
    release(&bcache.bucketlk[bucket_num]);
    acquiresleep(&lru_b->lock);
    // printf("%d\n",blockno);
    return lru_b;
  }
  // // Not cached.
  // // Recycle the least recently used (LRU) unused buffer.
  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     // release(&bcache.lock);
  //     release(&bcache.bucketlk[bucket_num]);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
  release(&bcache.bucketlk[bucket_num]);
  panic("bget: no buffers");
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
  // printf("%d\n",b->blockno);
  if(!holdingsleep(&b->lock))
    panic("brelse");
  releasesleep(&b->lock);
  // int bucket_num=b->blockno%NBUCKET;
  // // acquire(&bcache.lock);
  // // printf("bb%d\n",b->blockno);
  // acquire(&bcache.bucketlk[bucket_num]);
  b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head[bucket_num].next;
  //   b->prev = &bcache.head[bucket_num];
  //   b->ts=ticks;
  // }
  // release(&bcache.bucketlk[bucket_num]);
  
  // release(&bcache.lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.bucketlk[b->blockno%NBUCKET]);
  b->refcnt++;
  release(&bcache.bucketlk[b->blockno%NBUCKET]);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.bucketlk[b->blockno%NBUCKET]);
  b->refcnt--;
  release(&bcache.bucketlk[b->blockno%NBUCKET]);
}


