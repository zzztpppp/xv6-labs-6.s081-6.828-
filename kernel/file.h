struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe; // FD_PIPE
  struct inode *ip;  // FD_INODE and FD_DEVICE
  uint off;          // FD_INODE
  short major;       // FD_DEVICE
};

#define major(dev)  ((dev) >> 16 & 0xFFFF)
#define minor(dev)  ((dev) & 0xFFFF)
#define	mkdev(m,n)  ((uint)((m)<<16| (n)))

// in-memory copy of an inode
struct inode {
  uint dev;           // Device number
  uint inum;          // Inode number
  int ref;            // Reference count
  struct sleeplock lock; // protects everything below here
  int valid;          // inode has been read from disk?

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT+1];
};

// map major device number to device functions.
struct devsw {
  int (*read)(int, uint64, int);
  int (*write)(int, uint64, int);
};

extern struct devsw devsw[];

#define CONSOLE 1

// Virtual-memory-area(VMA)
struct vma {
    struct file * file;     // File the vma corresponds to.
    int free;               // A free vma
    uint64 addr;            // Mmapped address.
    uint64 length;          // Size of the mapped region
    uint64 permission;      // Permission of mmapped pages(PROT_READ, PROT_WRITE, etc.).
    uint64 flags;           // Is the modification written back to the file?
    uint64 offset;          // Offset where we want to mapped the file.
    int idx;                // Index of the process's vma table.
};

