# COW
## Copy-on-write fork

* Write uvmcow to replace uvmcopy, do not copy `trapframe`. Deep copy pagetable instead
of a shallow copy to de-couple operations on different cow page with the same physical page.
  
* `mappages` gives different result for `va` and `PGGROUNDDOWN(va)`.
* Matain an global array of reference count for all physical pages, with 1 as the initial values
and a associated spinning-lock.
  
* Copy physical memory before unmap it from the user page table.