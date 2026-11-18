# B.2 Working With Files

> 原書 p.784.e154–784.e156（Appendix B: Hitchhiker's Guide to Linux）

## 這節在講什麼

Table B.1 的檔案操作指令速查（`mkdir`、`ls`、`cat`、`less`、`cp`、`mv`、`cd`、`pwd`、`chmod`、`rm`、`rmdir`）。

對已經在用命令列的人來說，這張表可以跳過。這裡只記三件**容易記錯或有實際風險**的事：檔名規則、權限的八進位表示法、以及 `rm -rf` 沒有回頭路。

## 關鍵點

### 為什麼還是要用命令列

> "Although Linux GUIs have a file manager, developers are most productive getting around the file system at the command line."
> — p.784.e154

中文解釋：理由在後面幾節會看得更清楚——**命令列的東西可以組合、可以寫進腳本、可以用萬用字元批次處理**（B.3 的 pipe 和 B.5 的腳本）。GUI 檔案管理員做不到。

### 檔名規則

> "For best results, stick with filenames consisting of letters, numbers, and underscores. Avoid special characters, particularly spaces. Linux is case sensitive, so readme and README are two different files."
> — p.784.e154

中文解釋：**避開空格是實務上最重要的一條**。原因是 shell 用空格分隔參數，`cat my file.txt` 會被當成兩個檔案。真的有空格的話要加引號或跳脫字元，寫腳本時很容易出錯。

**大小寫敏感**對從 Windows 過來的人是常見的坑——`Makefile` 和 `makefile` 是不同的檔案。

### 權限：三組 rwx，用八進位表示

> "The permissions are 10 characters. The first character is a d for a directory or - for a regular file. The next characters indicate if a file is readable, writable, and executable for the user, group, and world (all users)."
> — p.784.e156

中文解釋：`-rw-rw-r--` 的讀法是 `[類型][擁有者 rwx][群組 rwx][其他人 rwx]`。

> "chmod changes the mode, aka permissions. It takes 3 octal digits to indicate the permission for user, group, and world. Each digit has three bits indicating read (4), write (2), and execute (1), listed as rwx."
> — p.784.e156

中文解釋：**三個八進位數字 = 三組三個位元**。記法很簡單：
- 讀 = 4、寫 = 2、執行 = 1
- `chmod 640 README3` → 擁有者 6（4+2，讀寫）、群組 4（唯讀）、其他人 0（不可存取）

常見的幾個值：
| 值 | 意思 | 用在哪 |
| --- | --- | --- |
| 644 | 擁有者讀寫、其他人唯讀 | 一般檔案 |
| 755 | 擁有者全部、其他人讀+執行 | **可執行檔、目錄** |
| 600 | 只有擁有者能讀寫 | 私鑰、密碼檔 |
| 700 | 只有擁有者能用 | 私人目錄 |

**一個常被忽略的細節**：

> "A user must have execute permission (x) on a directory to cd into it and execute permission on a program to run it."
> — p.784.e156

中文解釋：**目錄的「執行」權限意思是「可以進入」**，不是「可以執行」。所以目錄通常是 755 而不是 644——沒有 x 就 `cd` 不進去。這是第 23 章寫腳本時（`chmod +x myScript.sh`）用到的同一個機制。

### `rm -rf` 是不可逆的

> "Adding the -r option to rm makes it run recursively on all the contents of a directory. Adding -f forces rm to remove files without prompting. Join these together to remove a directory and all its contents. ... Use rm -rf cautiously; Linux does not have an undelete command (unless you are using version control as described in Appendix C)."
> — p.784.e156

中文解釋：**「Linux 沒有還原指令」**——這句話值得記住。沒有資源回收筒，刪掉就是刪掉。

書上給的唯一保險是**版本控制**（附錄 C 的 Git）。這也是為什麼第 22 章那個測試腳本 `/.profile` 裡雖然用了 `rm -rf myDir`，但那是在一個全自動、可重建的環境裡。

配合 B.3 會講的萬用字元 `*`，`rm -rf *` 可以在一秒內清空整個目錄——書上在 Table B.2 特別標了「Use with caution!」。

### `rmdir` 只能刪空目錄

> "rmdir removes a directory, but the directory must be empty."
> — p.784.e156

中文解釋：**這其實是一個安全機制**。`rmdir` 刪不掉非空目錄，等於逼你確認「裡面沒有你還要的東西」。想無腦刪就得明確用 `rm -rf`——多打幾個字，也多一次思考的機會。

### 幾個常用路徑符號

| 符號 | 意思 |
| --- | --- |
| `.` | 目前目錄 |
| `..` | 上一層目錄（`cd ..` 往上一層） |
| `~` | 自己的家目錄 |
| `~lynn` | 使用者 `lynn` 的家目錄 |

中文解釋：`cd` 不帶參數就回家目錄，等同 `cd ~`。

### `less` 這個名字

> "less displays the contents of a file. If it is a long file, it gives you one screenful at a time. (less is an improved version of the older more command; remember 'less is more.')"
> — p.784.e155

中文解釋：**「less is more」是雙關**——一方面 `less` 是 `more` 的改良版（可以往回捲），一方面呼應建築師 Mies van der Rohe 的名言。按 `q` 離開。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| case sensitive | 大小寫敏感 |
| working directory | 工作目錄，`pwd` 顯示的那個 |
| user / group / world | 擁有者／群組／所有人，權限的三個對象 |
| `rwx` | read / write / execute |
| octal permission | 八進位權限（讀 4、寫 2、執行 1） |
| `chmod` | change mode，改權限 |
| `/usr/sbin/groupadd` | 建立群組的指令 |
| recursive (`-r`) | 遞迴處理目錄下所有內容 |
| force (`-f`) | 不詢問直接執行 |

## 所以呢

三件實際會踩到的事：**檔名別用空格**、**目錄的 x 權限是「可進入」不是「可執行」**（所以目錄用 755）、**`rm -rf` 沒有還原**——唯一的保險是版本控制。

下一節 B.3 是萬用字元、串流重導向、和其他常用指令。
