
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

// Remove a block with given dev and blockno
// from the given table slot.
// Necessary locks must be acquired before calling this function
static void
remove_block(uint slot, struct buf *block) {
    struct buf *b;
    for (b = bcache_table.table_slots[slot]; b != 0; b = b->next) {
        if (b != block)
            continue;

        if (b->prev != 0)
            b->prev->next = b->next;
        if (b->next !=0 )
            b->next->prev = b->prev;

        // If we are removing head
        if (b == bcache_table.table_slots[slot]) {
            bcache_table.table_slots[slot] = b->next;
        }

        return;
    }

    // Wrong slot
    panic("bcache: remove failed");
}


// Insert a block into a given slot.
// Necessary locks must be acquired before calling this function
static void
insert_block(uint slot, struct buf *block) {
    // Not a empty slot
   if (bcache_table.table_slots[slot] != 0)
       bcache_table.table_slots[slot]->prev = block;

    block->next = bcache_table.table_slots[slot];
    block->prev = 0;
   bcache_table.table_slots[slot] = block;
}


static struct buf*
bget(uint dev, uint blockno) {
    struct buf *b;
    acquire(&bcache.lock);
    int slot = (int) blockno % NBUCKET;
    acquire(&bcache_table.locks[slot]);

    // Is the block cached?
    for (b=bcache_table.table_slots[slot]; b != 0; b=b->next) {
        if(b->dev == dev && b->blockno == blockno){
            b->refcnt++;
            release(&bcache.lock);
            release(&bcache_table.locks[slot]);
            acquiresleep(&b->lock);
            return b;
        }
    }
    release(&bcache.lock);
    release(&bcache_table.locks[slot]);

    // Not cached.
    // Recycle the least recently used unused buffer.
    // The recycle step is serial since we are holding the global bcache.lock
    uint least_ticks = ticks;
    int eviction_i = -1;
    lock_all();
    for (int i = 0; i < NBUF; i++) {
        if ((bcache.buf[i].refcnt == 0) && (bcache.buf[i].timestamp <= least_ticks)) {
            eviction_i = i;
            least_ticks = bcache.buf[i].timestamp;
        }
    }

    if (eviction_i == -1)
        panic("bget: no buffers");
    b = &bcache.buf[eviction_i];
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;

    // Move the block to the new hashing slot if it now hashes to a different slot
    int current_slot = bcache.slot_id[eviction_i];
    if (current_slot != slot) {
        if (current_slot != -1) {  // Remove from the old slot if there is one
            remove_block(current_slot, b);
        }
        // Insert to the head of the new slot
        insert_block(slot, b);
        bcache.slot_id[eviction_i] = slot;
    }
    release_all();
    acquiresleep(&b->lock);
    return b;
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
brelse(struct buf *b) {
    if(!holdingsleep(&b->lock))
        panic("brelse");

    releasesleep(&b->lock);
    acquire(&bcache.lock);
    b->refcnt--;
    if (b->refcnt == 0) {
        b->timestamp = ticks;
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


