
確定一下 simplefs 裡的 vfs_getcwd ，current_task 現在看起來是拿不到的？

```bash
lib/simplefs.c: In function 'vfs_getcwd':
lib/simplefs.c:437:28: error: 'current_task' undeclared (first use in this function); did you mean 'current_dir'?
  437 |     uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
      |                            ^~~~~~~~~~~~
      |                            current_dir
lib/simplefs.c:437:28: note: each undeclared identifier is reported only once for each function it appears in
make: *** [lib/simplefs.o] Error 1
```


---
