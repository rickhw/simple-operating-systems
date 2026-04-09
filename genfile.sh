#!/bin/bash

# 檢查是否提供了路徑參數
if [ -z "$1" ]; then
    echo "使用方式: $0 <目錄路徑>"
    exit 1
fi

TARGET_DIR="$1"

# 檢查目錄是否存在
if [ ! -d "$TARGET_DIR" ]; then
    echo "錯誤: 目錄 $TARGET_DIR 不存在。"
    exit 1
fi

# 使用 find 指令找出所有檔案並遍歷
# -type f 確保只抓取檔案，排除目錄
find "$TARGET_DIR" -type f | while read -r FILE; do
    # 取得檔案副檔名（轉小寫）用於 Markdown 語法高亮
    EXT="${FILE##*.}"

    # 輸出分隔線與路徑
    echo "---"
    echo "$FILE"

    # 輸出程式碼區塊開頭
    echo '```'"$EXT"

    # 輸出檔案內容
    cat "$FILE"

    # 輸出程式碼區塊結尾
    echo '```'
done
