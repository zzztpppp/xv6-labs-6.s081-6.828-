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
void *steal(int cid);

int next_to_steal;  // Next cpu to steal from

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem {
  struct spinlock lock;
  struct run *freelist;
  int nfree;     // Number of free blocks
};

// A memory list per CPU to reduce lock contention
struct kmem kmems[NCPU];

void
kinit()
{
  // Initialize locks for each per-cpu free-list
  for (int i=0; i < NCPU; i++) {
      initlock(&kmems[i].lock, "kmem");    // Name all cpu locks for 'kmem' for now.
      kmems[i].nfree = 0;
  }
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

// Free the page of physical memory pointed at by v,
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

  push_off();    // Disable interrupts to avoid cpu switch
  int cid = cpuid();
  acquire(&kmems[cid].lock);
  r->next = kmems[cid].freelist;
  kmems[cid].freelist = r;
  kmems[cid].nfree++;
  release(&kmems[cid].lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int cid = cpuid();
  acquire(&kmems[cid].lock);
  r = kmems[cid].freelist;
  if(r) {
      kmems[cid].freelist = r->next;
      kmems[cid].nfree--;
  }
  else {
      steal(cid);
  }
  release(&kmems[cid].lock);
  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// Steal memory blocks from other cpus to the cpu with given cid
void *
steal(int cid) {
    static int rover;    // Next to search
    int old_rover = rover;
    struct run *goods = 0;

    // Next fit search
    for (rover; rover < NCPU;rover++) {
        if (kmems[rover].nfree > 0){}

    }
}