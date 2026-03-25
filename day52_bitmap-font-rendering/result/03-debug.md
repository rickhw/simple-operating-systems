
跑起來後，看起來字有被畫出來了，解析度看起來是 800x600 沒錯。
但是字的對應是錯的，kernel.c 印出來的是 `=== OS Subsystems Ready ===`

```c
kprintf("=== OS Subsystems Ready ===\n\n");
```

但是圖形輸出的小寫看起來都加一了？
字型定義位置是否偏掉了？
