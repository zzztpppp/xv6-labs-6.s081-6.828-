
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

#define NBUCKET 13   // 13 hash-table buckets


static char lock_names[NBUCKET * 16];    // Lock names for each hash table slots

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  int slot_id[NBUF];    // Which hash-table slot is each buf cached, -1 means it doesn't belong to any slot.

} bcache;

struct {
    struct buf *table_slots[NBUCKET];
    struct spinlock locks[NBUCKET];

} bcache_table;


// Lock all slot locks in bcache_table
void
lock_all(void) {
   int i;
   acquire(&bcache.lock);
   for (i = 0; i < NBUCKET; i++) {
       acquire(&bcache_table.locks[i]);
   }
}

// Release all slot locks in bcache_table
void
release_all(void) {
    int i;
    release(&bcache.lock);
    for (i = 0; i < NBUCKET; i++) {
        release(&bcache_table.locks[i]);
    }
}

void
binit(void) {
    int i;

    initlock(&bcache.lock, "bcache");    // Preserve the old global lock for debug purposes.
    // Initialize table slot locks
    // and slots.
    for (i = 0; i < NBUCKET; i++) {
        bcache_table.table_slots[i] = 0;
        snprintf(lock_names + i * 16, 16, "bacahe_%d", i);
    }

    // Initialize buffer timestamp and slot ids.
    for (i = 0; i < NBUF; i++) {
        initsleeplock(&bcache.buf[i].lock, "buffer");
        bcache.buf[i].timestamp = ticks;
        bcache.slot_id[i] = -1;
    }
}

static struct buf*
bget(uint dev, uint blockno) {
    struct buf *b;
    acquire(&bcache.lock);
    uint slot = blockno % NBUCKET;
    acquire(&bcache_table.locks[slot]);

    // Is the block cached?
    for (b=bcache_table.table_slots[slot]; b != 0; b=b->next) {
        if(b->dev == dev && b->blockno == blockno){
            b->refcnt++;
            b->timestamp = ticks;
            release(&bcache.lock);
            release(&bcache_table.locks[slot]);
            acquiresleep(&b->lock);
            return b;
        }
    }

    // Not cached.
    // Recycle the least recently used unused buffer.
    // The recycle step is serial since we are holding the global bcache.lock
    uint least_ticks = ticks;
    int eviction_i = -1;
    lock_all();
    for (int i = 0; i < NBUF; i++) {
        if ((bcache.buf[i].refcnt == 0) && (bcache.buf[i].timestamp < least_ticks)) {
            eviction_i = i;
            least_ticks = bcache.buf[i].timestamp;
        }
    }
    if (eviction_i == -1)
        panic("bget: no buffers");
    // Move the block to the new hashing slot if it now hashes to a different slots
    if (bcache.slot_id[eviction_i] != slot) {
        if (bcache.slot_id[eviction_i] != -1) {

        }
    }
    release_all();


    acquiresleep(&b->lock);
    return b;
}


// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  acquire(&bcache.lock);

  // Is the block already cached?
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
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

  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  
  release(&bcache.lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


