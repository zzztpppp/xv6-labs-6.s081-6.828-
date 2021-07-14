# PGTBL lab logs
## vmprint
An easy lab, only need to be careful about how to 
correcly render render those ".." level breakers.
* Use a recursive helper funcions to print sub-tables.
* Tell the recursive call which level we are at in order to 
  render level breaks correctly
  
## A kernel page table per-procss
* kstacks are allocated and mapped on global the kernel pagetable for each process at
procinit. Just add mapps accordingly when allocproc.



## Simplify
* Check return the value returned by uvmcreate when creating a new pagetable. This trouble me for days trying to pass
usertests.
  
* Switch to newly created kpagetable before freeing old_kpagetable in exec.c. Also trouble me for
days(cannot access memeory at 0x********)
  
* Unmap all pages when mapping error occurred in proc_kpagetable.
* Dealloc kpagetable pages when mapping error occurred in growproc.(failed sbrkfail
  usertests)
  
