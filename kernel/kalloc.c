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

uint8 *reference_count;
struct spinlock reference_lock;


struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;


uint64
refcnt_init() {
    uint64 pa_start, npages;
    pa_start = PGROUNDUP((uint64) end);
    npages = ((uint64) PHYSTOP - pa_start) / PGSIZE;

    // We use a byte to count references of a pa, since the max number of processes is 64, number of
    // reference will not overflow. This actually takes more space than it needed, but its fine.
    uint64 size = 8 * npages;
    printf("size is %d\n", size);
    reference_count = (uint8 *)end;

    // Initialize reference count to 1 so that freerange can run and decrement reference count.
    memset(reference_count, 1, size);

    return size;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&reference_lock, "refcnt");
  // Initialize reference count array
  uint64 size = refcnt_init();

  // Free memory now begins after reference_count
  freerange(end + size / sizeof(char), (void*)PHYSTOP);
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

  // Decrement reference count when kfree a page with multiple references.
  uint8 count = decrement_reference((uint64)pa);
  if (count > 0) {
    return;
  }

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
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r) {
      increment_reference((uint64)r);
      memset((char *) r, 5, PGSIZE); // fill with junk
  }
  return (void*)r;
}


// Returns the position that page of a pa at in reference_count
int
page_at(uint64 pa) {
   return PGROUNDDOWN(pa - (uint64) end) / PGSIZE;
}

// Increment reference count of the physical page of a
// given pa.
int
increment_reference(uint64 pa) {
    uint8 cnt;
    acquire(&reference_lock);
    cnt = ++reference_count[page_at(pa)];
    release(&reference_lock);
    return cnt;
}

int
decrement_reference(uint64 pa) {
    uint8 cnt;
    acquire(&reference_lock);
    cnt = --reference_count[page_at(pa)];
    release(&reference_lock);
    return cnt;
}
