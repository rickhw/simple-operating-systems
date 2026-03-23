
我們之前有修改 keyboard.c，讓他支援 shift 輸入 double quote。不過我發現目前 user app 的 argument 似乎都無法處理 double quote. 也就是他會把 第一個 double quote 當作字串，也寫入檔案裡面。例如 `write file1 "hello"`，實際寫入的內容是 `"hello"`. 然後接下來我輸入 `ls` 就會出現很明顯是某些結構 offset 跑掉，然後造成整個畫面大亂 (如圖)。

但是只要我下 `rm file1` 後，再輸入 `ls` 則會看到正常的結果。

上述我在 write file 的時候，content 只要沒有 double quote，結果都是正常的。
