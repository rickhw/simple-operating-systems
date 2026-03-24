成功了，不過發現兩個有趣的問題：

1. 現在的 keyboard 其實不支援 Shfit + ' 的輸入，也就是無法呈現 double quote, 所以在使用 write 的時候 content 無法用 quote 處理
2. write 同一個檔名兩次 (ex: wirte file2 test; write file2 test)，再下 ls 之後，會出現不正常的 file size，如圖
