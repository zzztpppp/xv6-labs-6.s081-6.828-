# Net

* We need understand how `uart`works.
    * Here is a call graph that input/output goes through uart:
       * `kernel/printf` which calls `consputc` which calls `uartput_sync`. 
         In user space, `user/printf` which calls `sys_write` which calls `filewrite` which calls
         `consolewrite`which calls `uartputc`
       * `user/sh` which calls `getcmd` which calls `gets`which calls `sys_read` which calls
          `fileread` which calls `consoleread` which reads characters from the buffer.
* Which CPU deals with uart interrupt?
    * It depends on the policy that is configured with PLIC. For example, the PLIC may
    ask the first hart that are able to take the interrupt to do the thing.
* Does the busy-loop in upartput_sync is thread unsafe?
    * No, there is no other kernel threads are executing the same code for uart interrupt 
    will not serve until this one is dealt.
* Does writing to the corresponding register of the e1000 causes the transmission immediately?
  Then when does the `cmd` part take effect?