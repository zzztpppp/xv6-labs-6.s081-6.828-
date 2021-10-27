//
// Support functions for system calls that involve file descriptors.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fcntl.h"
#include "file.h"
#include "stat.h"
#include "proc.h"

struct devsw devsw[NDEV];
struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;


struct {
    struct spinlock lock;
    struct vma vt[NOFILE];
} vtable;

void
vtableinit(void) {
    initlock(&vtable.lock, "vtable");
    for (int i = 0; i < NOFILE; i++)
        vtable.vt[i].free = 1;
}


void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
}

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  struct file ff;

  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);

  if(ff.type == FD_PIPE){
    pipeclose(ff.pipe, ff.writable);
  } else if(ff.type == FD_INODE || ff.type == FD_DEVICE){
    begin_op();
    iput(ff.ip);
    end_op();
  }
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int
filestat(struct file *f, uint64 addr)
{
  struct proc *p = myproc();
  struct stat st;
  
  if(f->type == FD_INODE || f->type == FD_DEVICE){
    ilock(f->ip);
    stati(f->ip, &st);
    iunlock(f->ip);
    if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
      return -1;
    return 0;
  }
  return -1;
}

// Read from file f.
// addr is a user virtual address.
int
fileread(struct file *f, uint64 addr, int n)
{
  int r = 0;

  if(f->readable == 0)
    return -1;

  if(f->type == FD_PIPE){
    r = piperead(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
      return -1;
    r = devsw[f->major].read(1, addr, n);
  } else if(f->type == FD_INODE){
    ilock(f->ip);
    if((r = readi(f->ip, 1, addr, f->off, n)) > 0)
      f->off += r;
    iunlock(f->ip);
  } else {
    panic("fileread");
  }

  return r;
}

// Write to file f.
// addr is a user virtual address.
int
filewrite(struct file *f, uint64 addr, int n)
{
  int r, ret = 0;

  if(f->writable == 0)
    return -1;

  if(f->type == FD_PIPE){
    ret = pipewrite(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
      return -1;
    ret = devsw[f->major].write(1, addr, n);
  } else if(f->type == FD_INODE){
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * BSIZE;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op();
      ilock(f->ip);
      if ((r = writei(f->ip, 1, addr + i, f->off, n1)) > 0)
        f->off += r;
      iunlock(f->ip);
      end_op();

      if(r != n1){
        // error from writei
        break;
      }
      i += r;
    }
    ret = (i == n ? n : -1);
  } else {
    panic("filewrite");
  }

  return ret;
}

// Return the vma that contains this address if any.
struct vma *
vma_at(uint64 addr, int clear) {
    struct proc *p = myproc();
    struct vma *v;
    for (int i = 0; i < NOFILE; i++) {
        v = p->vmatable[i];
        if (v != 0 && ( addr >= v->addr && addr <= v->addr + v->length)) {
            if (clear)
                p->vmatable[i] = 0;
            return v;
        }
    }
    return 0;
}


int
munmap(uint64 addr, uint64 length) {
    struct vma *v;
    // Addr must be a multiple of the page size
    if ((addr & (PGSIZE - 1)) != 0)
        return -1;

    // All pages containing a part of the
    // indicated range are unmapped. Assume
    // the specified ranges are all in the same
    // vma and will not punch a hole int he middle of
    // the area.
    if ((v = vma_at(addr, 0)) == 0)
        return -1;

    // Write dirty data back to file if MAP_SHARED
    // The dirty bit check is ignored.
    filewrite(v->file, addr, (int)length);

    // Shrink vma in the unit of page
    length = PGROUNDUP(length);
    if (addr == v->addr)    // At the start of the vma.
        v->addr += length;
    v->length -= length;

    uvmunmap(myproc()->pagetable, addr, length / PGSIZE, 1);

    // The whole area is removed.
    // Close the file, clear the vma.
    if (length <= 0) {
        fileclose(v->file);
        acquire(&vtable.lock);
        v->free = 1;
        release(&vtable.lock);
        myproc()->vmatable[v->idx]  = 0;
    }
    return 0;
}

uint64
mmap(uint64 addr, int length, int prot, int flags, int fd, int offset) {

    struct file *f;
    struct vma *v;
    struct proc *p = myproc();
    int i;
    uint64 oldsz;

    f = p->ofile[fd];
    oldsz = p->sz;

    // Not allowed if prot contains PROT_WRITE and flags MAP_SHARED but with file mode O_RDONLY
    if (f->writable == 0 && (prot & PROT_WRITE) && (flags == MAP_SHARED))
        return -1;

    // Find a empty vma
    acquire(&vtable.lock);
    for (i = 0; i < NOFILE; i++) {
        if (vtable.vt[i].free)
            break;
        if (i == NOFILE - 1)
            return -1;
    }
    vtable.vt[i].free = 0;
    release(&vtable.lock);
    v = &vtable.vt[i];
    // Put the vma at process's vma table.
    for (i = 0; i < NOFILE; i++) {
        if (p->vmatable[i] == 0) {
            p->vmatable[i] = v;
            v->idx = i;
            break;
        }
        if (i == NOFILE - 1)
            panic("Too much mapped files for the process!");
    }
    // Use the vma
    filedup(f);
    v->file = f;
    v->addr = PGROUNDUP(p->sz);
    v->length = length;
    v->flags = flags;
    v->permission = prot;
    v->offset = offset;

    // Arrange mapped region.
    growproc_lazy(length);
    return oldsz;
}
