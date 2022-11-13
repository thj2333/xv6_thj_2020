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

#define NBUCKET 13

struct {
  struct spinlock lock[NBUCKET];
  struct buf buf[NBUF];
  struct buf hashbucket[NBUCKET];
  int visit[NBUCKET];
} bcache;

int myhash(int x){
  return (5*x)%NBUCKET;
}

void
binit(void)
{
  struct buf *b;
  int i;

  for(i=0;i<NBUCKET;i++){
    initlock(&bcache.lock[i], "bcache");
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
    bcache.visit[i]=0;
  }

  
  // Create linked list of buffers
  for(b = bcache.buf,i=0; b < bcache.buf+NBUF;i++,b++){
    int num = myhash(i);
    acquire(&bcache.lock[num]);
    b->next = bcache.hashbucket[num].next;
    b->prev = &bcache.hashbucket[num];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[num].next->prev = b;
    bcache.hashbucket[num].next = b;
    release(&bcache.lock[num]);
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int num = myhash(blockno);
  acquire(&bcache.lock[num]);

  // Is the block already cached?
  for(b = bcache.hashbucket[num].next; b != &bcache.hashbucket[num]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[num]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.hashbucket[num].prev; b != &bcache.hashbucket[num]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[num]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  bcache.visit[num] = 1;

  // Not buffer.
  // steal from other hashbucket
  for(int i=0;i<NBUCKET;i++){
    if(bcache.visit[i]==1) continue;
    if(i==num) continue;
    acquire(&bcache.lock[i]);
    for(b = bcache.hashbucket[i].prev; b != &bcache.hashbucket[i]; b = b->prev){
      if(b->refcnt == 0){
        // delete buf
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.lock[i]);
        bcache.visit[num] = 0;

        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        b->next = bcache.hashbucket[num].next;
        b->prev = &bcache.hashbucket[num];
        bcache.hashbucket[num].next->prev = b;
        bcache.hashbucket[num].next = b;
        release(&bcache.lock[num]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[i]);
  }
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
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int num = myhash(b->blockno);
  acquire(&bcache.lock[num]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[num].next;
    b->prev = &bcache.hashbucket[num];
    bcache.hashbucket[num].next->prev = b;
    bcache.hashbucket[num].next = b;
  }
  
  release(&bcache.lock[num]);
}

void
bpin(struct buf *b) {
  int num = myhash(b->blockno);
  acquire(&bcache.lock[num]);
  b->refcnt++;
  release(&bcache.lock[num]);
}

void
bunpin(struct buf *b) {
  int num = myhash(b->blockno);
  acquire(&bcache.lock[num]);
  b->refcnt--;
  release(&bcache.lock[num]);
}

