## RISC-V assembly
* It seems like there is no compiled code for calling `f(x)` in the argument of the  call to `printf`, instead
the compiler directly put the result of `f(x)` as a immediate number to `a1` as the second
  argument to `printf`.
  
## Backtrace
* Return address lives next to the beginning of the frame. Previous frame address
lives next to the return address.
  
* Instructions are 32-bits wide, addres are 64-bits.
* Function calls stack frame are contiguous and are in one page.

## Alarm
### Test0
* Add a state to record function pointer, time interval to call the function and time
elapsed since last call.
  
* Store the function pointer in p->frame->epc to invoke rather than in p->frame->ra, the
former tells where to return after the systemcall while the latter is just a state in the
  user code irrelevant to our system call. 
  
### Test1
* Add another trapframe `sigreturn_trapframe` to store a copy the trapframe once we invoke our handler.
And resume `sigreturn_trapframe` once we are ready to return. Since a complete process of a handler call
  involves entering and exiting kernel twice, at the second exit we want it exit to where
  the first entry begins, but the information(stored in the trapframe) was replaced by the 
  that of the second entry. So to resume, we need a copy.
  
### Test2
* Add one more state in proc which indicates whether a timer interrupt has returned and is
initially 1. Set this to 0 where we are about to call the handler and to 1 once we are about
  to return in `sigreturn`.