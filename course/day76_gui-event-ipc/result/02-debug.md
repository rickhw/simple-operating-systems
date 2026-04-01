結果如截圖，有幾個問題需要修正

1. Task Manager 的 Title 字母 (PID, Name, Mem, Kb, ACTION), 大小寫不一，應該 taskmgr.c 裡面的 8x8 字型庫有誤
2. 在 Task Manager 點某一個 PID Kill，例如 paint，實際結果是 Dynamic Paint Window 停止繪圖了，但是窗沒有不見。然後 ps 上的 process 也沒有不見.
