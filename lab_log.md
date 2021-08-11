# Lazy lab

## lazy allocation
* Create a new condition branch in `usertrap` that handles `scause` is 13 or 15
* Let it panic if `stval` is beyond `sz` or is less than `sp`
* A page is unmapped if `pte`pointer returned by `walk` is 0, meaning `pte` in one of first two levels of `pagetable` is 0,
or the `pte`pointer is none-zero but the content it points to has no `PTE_V`, meaning the `pte` in the third level of 
  the `pagetable` is 0. We need to ignore both cases when `uvmunmap` and `uvmcopy` to make
  lazy allocation work.
  
* Address range is not a concern, unlike in `copyout` and `copyin`, since it is limited by
caller.
  
## Tests

* Just continue when we encouter an unmapped page in `uvmcopy`
* Eagerly de-allocate memories.
* If in the right address range, `va < sz && va > sp`, `copyout` `copyin` and `copyinstr` should
allocate physical pages to unmapped `va`.