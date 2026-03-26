在 task.c 裡的 sys_exec 裡有一段用到 simplefs_find, 第一個 lba 應該是傳 current_task->cwd_lba or 1?

```c
int sys_exec(registers_t *regs) {
    char* filename = (char*)regs->ebx;
    char** old_argv = (char**)regs->ecx;

    fs_node_t* file_node = simplefs_find(current_task->cwd_lba, filename);
    if (file_node == 0) { return -1; }

    uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
    vfs_read(file_node, 0, file_node->length, buffer);
```
