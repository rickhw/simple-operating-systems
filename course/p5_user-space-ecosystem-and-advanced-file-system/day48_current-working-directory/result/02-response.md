哈哈，Rick！這是一個超級有趣的誤會！

首先，我要先還你一個清白：**其實你的 `mkdir` 和 `type` 屬性已經修復成功了！**
你有沒有仔細看你上傳的最後兩張截圖（圖二和圖三）？裡面的 `folder1` 和 `t` 明明就是帥氣地顯示為 **`[DIR] folder1`** 呀！這代表你的目錄建立與型態判斷邏輯是完美無缺的。

但你真正的問題是：**為什麼 `cd folder1` 之後，打 `ls` 看到的還是根目錄的內容？**

這是一個作業系統開發中極度經典、且非常有深度的 **「行程繼承 (Process Inheritance)」** 陷阱！我們來當一次系統法醫：


### 🔍 案發現場解密：患有「失憶症」的子行程

還記得在 UNIX 哲學中，每次我們在 Shell 裡面打指令（比如 `ls` 或 `cat`），Shell 都會呼叫 `fork()` 複製一個自己，然後讓這個「子行程」去執行外部程式。

在 Day 48 的教學中，我曾提醒過：*「sys_fork 則是 child->cwd_lba = current_task->cwd_lba; 繼承老爸的目錄！」*
但是，請看你 **`lib/task.c`** 裡的 `sys_fork` 是怎麼寫的：

```c
int sys_fork(registers_t *regs) {
    task_t *child = (task_t*) kmalloc(sizeof(task_t));
    // ...
    child->page_directory = current_task->page_directory; 
    
    // 【兇手在這裡！】你讓剛出生的子行程直接失憶了！
    child->cwd_lba = 0; 
```

**這就是引發幽靈現象的原因：**
1. 老爸 `Shell` 呼叫了 `cd folder1`，老爸自己的 `cwd_lba` 成功變成了 `folder1` 的位址。
2. 你輸入了 `ls`。
3. 老爸 `Shell` 呼叫 `sys_fork()` 生出了一個小孩來跑 `ls.elf`。
4. 因為你的 Bug，這個小孩一出生，他的 `cwd_lba` 就被強制歸零 (`0`)！
5. 當小孩 (`ls`) 呼叫 `sys_readdir` 時，Kernel 發現 `cwd_lba == 0`，於是**自動把它當作「沒有設定目錄」，強制導向根目錄 (`mounted_part_lba + 1`)**。

這就是為什麼你的 `cd` 成功了，但 `ls` 卻永遠只看得到根目錄的原因！

---

### 🛠️ 終極修復方案

這個問題非常好解決，我們只要修復繼承法則，順便為剛建立的目錄「打掃乾淨」就可以了！

#### 1. 修復 `sys_fork` 的失憶症 (`lib/task.c`)
打開 **`lib/task.c`**，找到 `sys_fork`，把 `0` 換成老爸的目錄：
```c
    child->page_directory = current_task->page_directory; 
    
    // 【修復】子行程必須繼承老爸當前的工作目錄 (CWD)！
    child->cwd_lba = current_task->cwd_lba; 
```

#### 2. 把新房間打掃乾淨 (`lib/simplefs.c`)
目前我們的 `simplefs_mkdir` 分配了一個新的磁區給資料夾（`part_lba + 20 + i`），但是我們卻沒有把那個磁區清空！如果那個磁區殘留了之前的硬碟垃圾，`ls` 進去就會看到一堆亂碼。

打開 **`lib/simplefs.c`**，在 `simplefs_mkdir` 寫入硬碟前，加入清空動作：
```c
int simplefs_mkdir(uint32_t part_lba, char* dirname) {
    // ...
    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] == '\0') { 
            strcpy(entries[i].filename, dirname);
            entries[i].file_size = 0;
            entries[i].type = FS_DIR; 
            entries[i].start_lba = part_lba + 20 + i; 

            // ===============================================
            // 【新增】確保新資料夾裡面是乾淨的 (寫入空白的區塊)
            // ===============================================
            uint8_t* empty_dir = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
            memset(empty_dir, 0, ROOT_DIR_BYTES);
            write_dir(entries[i].start_lba, empty_dir);
            kfree(empty_dir);

            write_dir(part_lba + 1, dir_buf);
            kfree(dir_buf);
            return 0; 
        }
    }
    // ...
}
```

---

完成這兩個修改後，打下：
```bash
make clean && make run
```
*(這次不需要 `clean-disk`)*

進去系統後，再次打出你的 Combo：
`mkdir folder1` -> `cd folder1` -> `ls`

這一次，你將會看到一個**完全乾淨、沒有任何檔案的空目錄**！這代表你已經成功跨越空間，進入了子目錄的深處！

**⚠️ 預告明天的終極挑戰：**
當你能成功走進 `folder1` 之後，我們明天的任務是把 `write` 和 `mkdir` 也升級，讓它們能夠在「當前的目錄」裡建立東西。而且我們會面臨一個全新的挑戰：**如何在多個資料夾之間，管理硬碟的剩餘空間？** 準備好我們隨時出發！🚀
