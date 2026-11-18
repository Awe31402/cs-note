# B.1 Connecting to a Linux Server

> 原書 p.784.e153–784.e154（Appendix B: Hitchhiker's Guide to Linux）
> 涵蓋 B.1.1 X2Go、B.1.2 Changing Your Password
> 註：這幾頁的頁眉誤植為「APPENDIX A Wally Synopsis」，實際內容是附錄 B

## 這節在講什麼

附錄 B 是給「習慣 Windows 或 Mac 的工程師」的 Linux 入門。對已經在用 Linux 的人來說大部分可以跳過，但 B.1 有一個**產業現實**值得知道：**商用邏輯合成工具只在 Linux 上跑**，而且通常是跑在遠端伺服器上，不是自己的筆電。

## 關鍵點

### 為什麼非 Linux 不可

> "Linux is a powerful, free, and open development environment and is the only environment supported by commercial logic synthesis tools."
> — p.784.e153

中文解釋：**「唯一支援的環境」**——這不是偏好問題。Synopsys Design Compiler（第 6 章）、Cadence、Siemens Questa 這些 EDA 工具只有 Linux 版。

### 典型的工作環境：server + client

> "In a typical design environment, engineers work on a Linux server. The server has all the computer-aided design (CAD) tools installed so that users don't have to install them locally. It also has plenty of RAM (random access memory), hard drive space, and processors to support many users and to facilitate sharing files between users. Users access the server from a client computer, such as a desktop or laptop running Windows, MacOS, or Linux."
> — p.784.e153

中文解釋：**這個分工是產業標準**，理由有三個：
1. **授權**——EDA 工具的 license 很貴，集中在伺服器上共用（第 23 章提過 Vivado 的 `XILINXD_LICENSE_FILE` 就是指向 license server）
2. **資源**——合成和模擬要吃大量 RAM 與 CPU（第 23 章：Vivado 要 90 GiB 磁碟；第 22 章：Linux 開機模擬要 20 小時）
3. **共用檔案**——團隊要看同一份設計

客戶端只是一個顯示終端，可以是「宿舍、機場、甚至有 Wi-Fi 的山頂」。

### 遠端圖形界面的選擇

> "The Linux graphical user interface (GUI) windowing system is called X11, or simply X. To run X applications on the server and have them appear on a laptop, the user needs a program such as VNC (Virtual Network Computing) or X2Go that transmits pixels from the server to the client. For casual users, your best choice is the one supported by your company or university. RealVNC's VNC Connect is used widely but has a monthly subscription fee for commercial use. X2Go is not as fast but is free and fairly easy to use."
> — p.784.e153

中文解釋：**關鍵區別是「傳畫素」而不是「傳 X 協定」**。書上在邊欄明確說：

> "It is also possible to connect to a server with X-forwarding to your laptop using: ssh -Y hostname, but this is too sluggish for serious graphical use. ssh is sufficient and simple for terminal-based work."
> — p.784.e154（邊欄）

中文解釋：**`ssh -Y` 對終端機工作夠用，但跑 GUI 太慢**。原因是 X11 協定設計於 1980 年代的區域網路，每個繪圖動作都要來回一趟，延遲一高就變成幻燈片。VNC/X2Go 在伺服器端渲染好再傳壓縮過的畫素，對高延遲網路友善得多。

**實務判斷**：只是編譯、跑模擬、看 log → `ssh` 就好。要開 Questa 波形視窗或 Vivado → 需要 VNC/X2Go。

（邊欄：許多新的 Linux 發行版改用 Wayland 而非 X11，需要不同的連線方式，例如 RDP。）

### X2Go 的設定要點

> "To use X2Go, the system administrator needs to confirm that the x2goserver is running on the Linux server. The user then downloads the client (x2goclient) from x2go.org. Mac users must also download and install the X windows server XQuartz from xquartz.org. ... Set Session type to XFCE, which gives a lightweight desktop environment."
> — p.784.e153

中文解釋：**選 XFCE 是刻意的**——輕量桌面環境的繪圖動作少，遠端連線才順。GNOME/KDE 那種有動畫特效的桌面在遠端會很卡。

### 工作階段可以中斷後接回

> "The session on the server will open. You can suspend your session and reopen it later and come back to the same windows and programs you had been using. Long jobs will continue to run on the server even when your session is suspended."
> — p.784.e154

中文解釋：**這是遠端桌面相對 `ssh` 的最大優勢**——關掉筆電、隔天在別的地方接回來，視窗和程式都還在。用 `ssh` 的話，連線斷了跑到一半的程式就被殺掉（除非用 `screen` 或 `tmux`）。

### 改密碼

> "$ passwd / Changing password for lynn / (current) UNIX password: / New password: / Retype new password: / passwd: all authentication tokens updated successfully"
> — p.784.e154

中文解釋：`passwd` 就一個指令。

書上也說明了這個附錄的排版慣例：

> "In this appendix, the command prompt is represented as $. Your system may have a longer prompt. Only type the blue command after the prompt"
> — p.784.e154

中文解釋：`$` 是提示符號，不要跟著打。

（邊欄推薦延伸閱讀：[Schotts19]，有免費 PDF 或付費印刷版；edX、Coursera、Udemy 上也有 Linux 課程。書上坦承「網路上有無數 Linux 指令的資訊，但我們還沒找到一個簡潔介紹硬體開發者所需 Linux 功能的來源」——這就是寫這個附錄的理由。）

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| CAD tools | computer-aided design，此處指 EDA 工具 |
| server / client | 跑工具的遠端主機／使用者的本機電腦 |
| X11 / X | Linux 的圖形視窗系統 |
| X-forwarding (`ssh -Y`) | 把 X 協定經 ssh 轉送，遠端跑 GUI 但很慢 |
| VNC | Virtual Network Computing，傳畫素的遠端桌面 |
| X2Go | 免費的遠端桌面方案，較慢但易用 |
| XQuartz | macOS 上的 X server |
| XFCE | 輕量桌面環境，適合遠端連線 |
| Wayland | X11 的後繼者，許多新發行版採用 |
| RDP | Remote Desktop Protocol |
| suspend session | 暫停工作階段，之後可接回同樣的視窗 |

## 所以呢

這節的實用結論只有三點：**EDA 工具只有 Linux 版**、**遠端 GUI 要用 VNC/X2Go 而不是 `ssh -Y`**、**遠端桌面的工作階段可以中斷後接回**。接下來 B.2 和 B.3 是檔案操作與常用指令的速查表。
