# Net

* We need understand how `uart`works.
    * Here is a call graph that input/output goes through uart:
       * `kernel/printf` which calls `consputc` which calls `uartput_sync`. 
         In user space, `user/printf` which calls `sys_write` which calls `filewrite` which calls
         `consolewrite`which calls `uartputc`
       * `user/sh` which calls `getcmd` which calls `gets`which calls `sys_read` which calls
          `fileread` which calls `consoleread` which reads characters from the buffer.