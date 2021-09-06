# Utrhead
* When to switch threads? Since there is no 'timer interrupt' in user thread.
  * It's issued by the callee thread itself...once the thread wants to 
    switch to another thread, it calls `thread_yield` to give up running.
    
* What to save in the context when the thread is created?
  * Mannully set `ra` and `sp` for newly created threads, where ra is the thread main function pointer 
  and sp is the thread stack pointer. Since here thread main function pointer has no arguments.
    
* Stack grows from a higer address down to a lower address, so need to set `context.sp` to `sp + STACK_STIZE`
in `create_thread`.