幫我 review src/user/ 裡的 *.c ，然後從這些 code 出發整理一下幾件事情：
1) 重構 + 調整 code 以及相關的 code (*.h or *.S)，特別是 src/user/lib/.c 裡的分類
2) 重構完之後，要透過 make 確認可以正常編譯; 
3) 確認編譯後，幫我 *.c 加入適度的註解與說明，提升程式碼的可讀性. 

開始之前，請先開 branch，切換過去之後再開始，完成 1),2),3) 後請停在 branch 等我 review.


---

✦ 我已經完成了 src/user/ 目錄下的代碼重構與優化，並整理在 refactor/user-space-cleanup 分支中。

  核心重構項目說明：

   1. 系統呼叫整合 (syscall.h, syscall.c)：
       * 將所有分散在核心與各處的系統呼叫編號整合至 src/user/include/syscall.h 中，定義為 SYS_* 宏，方便統一管理。
       * 優化 syscall() 封裝函式，確保所有使用者空間的系統呼叫都透過這個通用介面進入核心。

   2. 函式庫分類優化 (Lib Classification)：
       * unistd.h / unistd.c：回歸 POSIX 標準，僅保留行程管理 (fork, exec, wait)、檔案 I/O (open, read) 與基本系統控制。
       * simpleui.h / simpleui.c：將所有與 GUI 視窗管理相關的系統呼叫（如
         create_gui_window）以及繪圖工具函式移至此處，落實介面與邏輯分離。
       * net.h / net.c：修正了錯誤的 Header Guard，並將 sys_ping 等網路相關系統呼叫封裝在此。
       * stdio.h, stdlib.h, string.h：補足了缺失的標頭檔宣告，並統一編碼風格。

   3. 編譯驗證與修復：
       * 執行 make 重新編譯所有 User Apps。
       * 針對 cat.c, paint.c, viewer.c, status.c 等應用程式中因重構產生的隱含宣告警告 (Implicit declaration)
         進行修正，補上正確的 #include。
       * 目前所有應用程式與核心皆可正常編譯並產生 myos.iso。

   4. 可讀性提升：
       * 在 src/user/lib/ 與 src/user/include/ 的所有檔案中加入了詳細的註解 (Doxygen
         風格)，說明函式參數、回傳值以及底層運作機制。

  目前代碼已在 refactor/user-space-cleanup 分支就緒，請檢查。
