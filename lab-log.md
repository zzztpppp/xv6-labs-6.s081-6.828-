# mmap
* Add a vma table as a member of `struct proc`.
* Add a branch in `usertrap` that hanles page fault of mmapped pages.
    * `scause` 12 means load page fault, 13 store page fault.
    * Read at most 4096 bytes(a page) of content of the mapping file.
    * Update corresponding offsets
* How to find a chunk in the vm with given length?
    * Grow the `proc` size without actually allocating the physical pages, which is deferred to
      the page fault handler.
* Write mmapped region back to the file in `munmap`
* Unmap all regions in the process' vma when `exit`
* Will `mmap` interfere with `sbrk` and hence interfere with `umalloc`?
    * No, `sbrk` grows the user space from which previous `mmap`/`sbrk` left, and 
      `umalloc` accept newly allocated memory chunks despite it's contiguous with current heap space or not.
      It just add the new chunk to the free list.
* Do we need to shrink the `proc->sz` after `munmap`?
    * It's ok if we don't. Since `munmap` frees the physical memory, holding over some chunks of user would not
      be a big deal.
* Does `munmap` just unmap the underlying physical memory  or it also alter the `vma.addr` and `vma.length`?
    * from the `munmap` linux man page: the further access to a ummapped address will cause an invalidate memory
      reference. So we need to modify the corresponding vma.
* Use `readi` instead of `fileread` since we don't want to update offset because we may not access the file
sequentially.
* How to distinguish between page faults on invalid pages and pages with wrong permissions. 
    * Not a concern here.
* Silence the `panic` in `uvmcopy` and `uvmunmap` and defer everything to `pagefault` 
  