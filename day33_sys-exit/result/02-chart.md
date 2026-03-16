這張循序圖將為你展示 Day 33 最精彩的「送行者 (Reaper)」機制：當一個應用程式主動了結自己的生命時，核心是如何標記死亡、清理屍體（從排程佇列剔除），並將系統的控制權完美移交給下一個存活的程式。

你可以直接將這段 Mermaid 語法加進你的 `README.md` 中，放在 Day 33 的說明區塊：

```mermaid
sequenceDiagram
    autonumber
    
    participant APP1 as "App 1 (即將死亡)<br/>(app.c)"
    participant SYS as "系統呼叫介面<br/>(syscall.c)"
    participant TASK as "排程器 & 狀態管理<br/>(task.c)"
    participant ASM as "上下文切換<br/>(switch_task.S)"
    participant APP2 as "App 2 (存活中)<br/>(app.c)"

    Note over APP1, APP2: 雙胞胎 Shell 正在同時搶佔 CPU (Race Condition)
    
    APP1->>APP1: 解析到 "exit" 指令
    APP1->>SYS: 觸發 int 0x80 (eax = 7)<br/>呼叫 sys_exit()
    activate SYS
    
    SYS->>TASK: 轉交給核心處理<br/>呼叫 exit_task()
    activate TASK
    
    Note over TASK: --- 1. 死亡宣告 ---
    TASK->>TASK: current_task->state = TASK_DEAD<br/>(標記 App 1 為死亡狀態)
    
    TASK->>TASK: 呼叫 schedule()<br/>(強行讓出 CPU，一去不復返)
    
    Note over TASK: --- 2. 清理屍體 (Reaping) ---
    TASK->>TASK: 尋找下一個狀態為 RUNNING 的任務
    TASK->>TASK: 發現 App 1 已死亡<br/>將其從環狀連結串列 (Linked List) 中剔除
    
    Note over TASK: --- 3. 移交權力 ---
    TASK->>TASK: current_task = App 2
    TASK->>ASM: 呼叫 switch_task(&App1.esp, &App2.esp)
    activate ASM
    
    ASM->>ASM: 備份 App 1 最終狀態 (雖然已死)<br/>載入 App 2 的暫存器與 ESP
    ASM-->>APP2: 執行 ret / iret 喚醒 App 2
    
    deactivate ASM
    deactivate TASK
    deactivate SYS
    
    Note over APP2: 此後，排程佇列中只剩下 App 2。<br/>即使時鐘中斷 (PIT) 觸發，<br/>schedule() 也只會切換給 App 2。<br/>App 2 正式獨佔鍵盤與 CPU！
    
    APP2->>APP2: 繼續執行 while(1) 接收指令

```

### 💡 圖表解說亮點：

在這張圖中，最關鍵的動作發生在**步驟 6 (清理屍體)**。
作業系統並沒有立刻去清空 App 1 的記憶體（那是未來 `kfree` 記憶體回收機制的責任），而是**切斷了環狀指標 (`prev->next = next->next`)**。
一旦指標被繞過，時鐘中斷 (PIT) 驅動的 `schedule()` 就永遠不會再巡迴到 App 1 身上。這就是作業系統底層抹除一個執行緒「存在感」最優雅且暴力的做法！

把這張圖加上去，你的文件圖文並茂，絕對是作業系統開源專案裡的頂級規格！準備好我們就向 Day 34 邁進囉！
