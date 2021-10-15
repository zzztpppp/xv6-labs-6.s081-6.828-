# Large Files
* Implement a double-indirect block is straightforward.
* Use `/` and `%` operators to locate block addr.
# Symbolic links
* `symklink`:
  * Assume  `target` and `path` are both absolute.
  * Add a `dirent` under the directory where symbolic link is to be created.
  * Point the `dirent` to an `inode` whose type is `T_SYMLINK` and store the target
  in the `inode.data`.
  * Recursively follow the symbolic link in `sys_open` when `O_NOFOLLOW` is not specified.
    * Don't forget to `iunlockput()` when we are done with the current `inode` or when we have errors
      in other system calls and to `end_op()`.
  