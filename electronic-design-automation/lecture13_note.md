# 設計自動化（一）第十三支影片完整筆記：Pin assignment——floorplanning 的一部分（ch5 pin assignment）

> **課程**：Electronic Design Automation (1)（清大 CSR 5302）— Lecture 12 | ch5. Pin_Assignment（播放清單第 13 支）
> **講者**：李炯霆（JT Li），清大半導體學院，2026 春，NTHU OCW 錄影
> **來源**：YouTube 錄影（20:08，無字幕，whisper 轉錄）＋ 投影片截圖（「Pin Assignment — Part of Floorplanning Problem」，JT Li）＋ 課本 Wang, Chang, Cheng（eds.）*Electronic Design Automation* Ch10（floorplanning 相關）
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿辨識不清、或影片／投影片／課本說法不一致之處
> - `[mm:ss]`：影片時間點
>
> ⚠️ 播放清單標題寫「Lecture 12」，但在播放清單是第 13 支；檔名依播放清單順序。投影片封面日期寫 02/11/2025（應是沿用去年的投影片）。課本沒有 pin assignment 專節。

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 為什麼要 pin assignment](#1-為什麼要-pin-assignment)
- [2. Pedram：floorplanning、pin assignment 與 global routing 交互影響](#2-pedramfloorplanningpin-assignment-與-global-routing-交互影響)
- [3. 用 global routing 決定 pin：building block layout](#3-用-global-routing-決定-pinbuilding-block-layout)
- [4. Bus-driven floorplanning 與 pin assignment](#4-bus-driven-floorplanning-與-pin-assignment)
- [5. 商用工具與 PCB／FPGA I/O planning](#5-商用工具與-pcbfpga-io-planning)
- [6. 最難的情況：multiply instantiated block](#6-最難的情況multiply-instantiated-block)
- [7. 參考論文](#7-參考論文)
- [8. 【實測】](#8-實測)
- [9. 一頁速查表](#9-一頁速查表)
- [10. 與前後講次的連結](#10-與前後講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

```
soft macro 的形狀還沒定 → pin 在邊界上的位置也還沒定
   → 依和鄰居的連線（fly line）放 pin，減少 crossing
   → Pedram：拆開 hierarchy、用 channel connectivity graph 做 global routing，決定 pin 在哪個邊、哪個 slot
   → 兩階段：先 global routing + 粗略 pin assignment，再做 channel 的 final pin assignment
   → bus：bus pin 要翻轉（flip）讓 MSB 對齊、避免 twist；bus 交叉要改 floorplan
   → 最難：同一個 block 被 instantiate 很多次（MIB），pin 位置必須一樣
```

【口頭】老師先講完 global routing 再回來講 pin assignment，因為有些方法要用到 global routing 的結果。

---

## 1. 為什麼要 pin assignment

[00:00]–[02:48]　投影片封面、Pin Assignment / Planning

- **Pin assignment 是 floorplanning 問題的一部分。**
- **soft macro 的 pin 常常還沒決定。** 怎麼 assign 它們的 I/O pin？
  - 依和**相鄰 macro 的連接**；
  - 看 **fly line（飛線）**——希望**減少 crossing**。
  - 【口頭】中間虛線的是 soft macro。一開始 pin 都放在中心，再看它連到的 pin 在哪個方向：連到上面的就往上面的邊走；pin 不一定在邊界的中間。像圖上某個 pin 的飛線和另一條 net 交叉，就該換位置。
- **有挑戰的問題：soft macro 被 instantiate 很多次時怎麼辦？**
  - 【口頭】例如一顆 CPU 在晶片上 duplicate 好幾次，這顆 CPU 的 pin 位置要怎麼設計？（[§6](#6-最難的情況multiply-instantiated-block)）

---

## 2. Pedram：floorplanning、pin assignment 與 global routing 交互影響

[02:48]–[08:54]　投影片 Floorplanning with Pin Assignment（Pedram，FP-pinAssign.pdf）、Figure 3

- 情境：A、B、C 已經擺好、pin 也定了（r1、r2、p4、p5…）；**hierarchical cell D** 還沒 floorplan，裡面有 D1、D2、D3 三個還沒成形的「porous cell」，各有 pin（p1、q1、p2、p3、q2）。
  - 【口頭】D 裡的 pin 要接到下面的 C，自然希望 pin 放到下面；q1、q2 之間也有連線，D3 擺下面的話 q2 會被拉上去、q1 被拉下來。
- **Pedram 的提議：floorplanning、pin assignment、global routing 在階層式的步驟中互相影響。**
- 做法：
  1. **拆掉 D 的階層**，把 D1、D2、D3 露出來。
  2. 依大小決定它們的形狀，擺好。
  3. 把每個 block 的邊界往外延伸，產生一些長方形區域（垂直的就只往上下延伸），建 **channel connectivity graph** 來做 **global routing**。
  4. D1 裡的 p1、q1 位置還沒定，但根據設計可以決定可能的 **pin slot**（圖上 D1 邊界的黑點 b1、b2…）。
  5. 依 global routing 的結果：p1 要往下接，就放在 D1 的**下面**；q1 接到 q2，往那個方向走；p2 也跟著要接到下面的 p5。
- **用 global routing graph 先決定 pin 要放在哪個邊界、再放到哪個 pin slot**，讓 net 變短。
- 投影片 Figure 3：final pin assignment 之前的 partial floorplan，標出 **vertical bottleneck**、**horizontal bottleneck**（routing 的瓶頸區域）。
- **重建階層**：把 D 的階層放回來時，D 對外的 pin 怎麼決定？
  - 如果某個 net 只有一個連接點出來，就把 pin 放在那裡。
  - 投影片「Possible option」：也可以在 D 的邊界上**複製**好幾個 pin（因為 p1–p2–p3 的線是在 D 的邊界裡面）。
  - 【口頭】**取決於你怎麼定義 D 的邊界**；邊界縮小一點，p1 就要有 pin 出去，但這樣就要改 D 這個 module 的上層 pin，要留意。

---

## 3. 用 global routing 決定 pin：building block layout

[08:54]–[11:54]　投影片 Pin Assignment with Global Routing for VLSI Building Block Layout、Fig. 1 floor connection graph

- 論文：Pin assignment with global routing for VLSI building block layout。
- **Floor connection graph** $G_r = (V_r, E_r)$：每個 channel 一個節點，再連到 module 的中心；邊分成 **assignment edge**、**channel edge**，以及穿過 module 的 **feedthrough edge**。黑點是 pin，灰色線是一條 net 的 routing。
  - 【口頭】當時這比較 sophisticated，連 feedthrough 都考慮進去。根據 net 的走法，決定 pin 要 assign 到 module 的右邊或左邊。
- **第一階段**：不只決定 global route，也決定**粗略的 pin assignment**（pin 大致在哪個邊），同時決定 block 的形狀和位置。
- **第二階段**：算 channel 的 **final pin assignment**。

---

## 4. Bus-driven floorplanning 與 pin assignment

[11:54]–[16:00]　投影片 Bus Driven Floorplanning With Pin Assignment（2012）、Fig. 5、Bus Ordering & Resolving Overlapping、Fig. 11–12

- 黑色方塊代表 **MSB（most significant bit）**。
- (a) 一條水平 bus 連兩個 bus pin；(b) 不考慮 bus pin 方向的 bus planning；(c) **bus twisting**（MSB 對到另一邊的 LSB，線交叉）；(d) 沒有 twisting。
  - 【口頭】bus pin 的方向沒考慮的話，MSB 跟 MSB 會交叉——bus 扭掉了。這時要把 bus pin **flip**，讓 MSB 對齊。
- **Diagonal bus**（兩個 module 斜對角）：一邊放右邊、一邊放下面時中間會 twist；把一邊 flip 過來就不 twist，但某些 bit 的線長會長一點——要權衡。
  - 【口頭】轉彎的地方通常要換 layer，所以會有 via，flip 之後 via 的方向也變了。
- **Fig. 5：bus pin flipping**——同時解掉直線上的 twist 和轉彎（**turning node**）處的 twist；代價是線長多一點。投影片：「Making sure MSBs are…」對齊。
- **Bus ordering 與重疊**：
  - Fig. 11：(a) bus ordering（$b_1 = \{m_2, m_3\}$、$b_2 = \{m_1, m_4\}$）；(b) **bus crossing**（$b_1 = \{m_2, m_5\}$、$b_2 = \{m_1, m_6\}$，一個要往上一個要往下，無法決定誰在上面）；(c) **ordering constraint graph** 裡 $b_1$、$b_2$ 互相指向形成 cycle → 要打破它，例如把 module m5、m6 對調。
  - Fig. 12：(a) **module moving**——bus $b_1$ 要讓 m3 往下移 $h_3$；(b) bus overlapping；(c) 把 m2 往上、對齊 m1、m5，兩條 bus 就不重疊。
  - 【口頭】也就是 bus 的 pin assignment 和 floorplan 有關（同[第 9 支](lecture09_note.md) §14 的 bus 限制）。

---

## 5. 商用工具與 PCB／FPGA I/O planning

[16:00]–[18:12]　投影片 Siemens Hierarchical Flow for Advanced Node SoCs、FPGA I/O Planning、FPGA I/O Optimization For PCB Placement & Routing

- **Siemens** 2022-11-03 的 webinar「Hierarchical Flow for Advanced Node SoCs」，會教：**pin assignment**、**timing budgeting**、partition 的 top-level 整合、in-hierarchy optimization（iHO）。
  - 【口頭】timing budgeting 和 floorplanning 也有關。一個做法是先用 **global routing** 決定 pin 要 assign 到哪個邊界，再依更細的 routing 結果把 pin 定下來。
- **FPGA I/O planning**（Siemens Xpedition，用來解釋 floorplanning 裡的 pin-out planning）：
  - FPGA 的擺放是 I/O 最佳化的決定性因素；可以有或沒有 PCB layout 資料；**直接在 net line 上做 pin swapping 和解開（unravelling）**；結果匯入 Xpedition layout。
  - Multi-FPGA optimization：同時最佳化多顆 FPGA，增加 routability、降低製造成本、考慮 FPGA 的 break-out。
  - 【口頭】左邊 pin assignment 沒做好，線有很多 crossing；調整後 crossing 大大減少。
- **FPGA I/O optimization for PCB placement & routing**：投影片註：**3DIC 的 design planning 也一樣**。圖：U100 的 pin 和 net 的對照表。
  - 【口頭】工具可以很快算出有多少 crossing，再想辦法減少。

---

## 6. 最難的情況：multiply instantiated block

[18:12]–[19:45]　投影片 Pin Assignment In MIB – More Complicated

- 如果要 assign pin 的 module 被 **multiply instantiated（MIB）**呢？
- 對 MIB 放 pin 時，pin placement 演算法要決定一個**對所有 instance 都好**的位置，確保**每個 instance 的 pin 位置完全一樣**。
- 圖：中間兩個虛線 block 是同一個 module，紅色 pin 的那個和綠色 pin 的那個各自連到不同的地方。
- 【口頭】要考慮這兩個 instance 一起決定 pin；決定之後還要看對整體的影響；有時 instance 還會被 **flip**，更麻煩。

---

## 7. 參考論文

[19:45]–[20:03]　投影片 Papers On Pin Assignment

- Pedram 的 FP-pinAssign.pdf
- Pin assignment with global routing for VLSI building block layout（ResearchGate）
- Springer 的章節（10.1007/0-306-47509-X_6）
- IIT Kharagpur 的 floorplanning 投影片、一份 IEP lecture 投影片
- IC Compiler II Design Planning User Guide
- 【口頭】歡迎有興趣的同學多探討。

---

## 8. 【實測】

程式：`experiments/lecture13/pins.py`，輸出：`experiments/lecture13/output.txt`。

一個 4 × 4 的 soft macro，邊界上有 12 個 pin slot；4 個 pin 各自連到右、下、左、上四個鄰居的 pin。

```
naive slots : wirelength 44, fly-line crossings 4
optimised   : wirelength 28, fly-line crossings 0, slots ((4, 2), (2, 0), (0, 3), (2, 4))
MIB: sum of private optima 27; best identical pin placement 35 slots ((4, 2), (2, 0))
```

- 把 4 個 pin 都擠在上面和右上角時，總線長 44、飛線交叉 4 次；窮舉所有 slot 指派、最小化線長後，每個 pin 自然跑到朝向它鄰居的那一邊，線長 28，**交叉也同時降到 0**——對應投影片「依和鄰居的連線放 pin、減少 crossing」。
- **MIB**：同一個 block 有兩個 instance，各自的 pin 連到不同地方。各自最佳化的線長合計 27；但兩個 instance 必須用**相同**的 pin slot，最好也只能做到 35（多 30%）——這就是投影片說 MIB 更複雜的原因：一個位置要同時對所有 instance 妥協。

---

## 9. 一頁速查表

| 主題 | 重點 |
|---|---|
| 為什麼 | soft macro 形狀未定，pin 位置也未定；pin 位置影響線長、crossing、routability |
| 直覺做法 | 看 fly line，pin 放在朝向連接對象的邊，減少 crossing |
| Pedram | 拆 hierarchy → 擺子 block → channel connectivity graph 做 global routing → 決定邊與 pin slot → 重建 hierarchy |
| 兩階段 | global routing + 粗略 pin assignment（同時定 block 形狀位置）→ channel 的 final pin assignment |
| Bus | bus pin flip 讓 MSB 對齊、避免 twist（直線、轉角）；代價是線長；bus crossing 要改 floorplan（換 module、移 module） |
| 工具 | Siemens hierarchical flow（pin assignment、timing budgeting）；FPGA／PCB I/O planning（pin swapping 減少 crossing；3DIC 同理） |
| MIB | 多個 instance 的 pin 位置必須相同；還可能 flip |

---

## 10. 與前後講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| floorplanning、bus 限制、soft macro | [第 9 支](lecture09_note.md) |
| global routing、channel connectivity graph | [第 11](lecture11_note.md)–[12 支](lecture12_note.md) |
| pin access（cell 層級） | [第 12 支](lecture12_note.md) §15 |
| channel routing（pin 固定在 channel 兩邊） | [第 14 支](lecture14_note.md) |

---

## 附：本講一句話總結

> **Soft macro 的形狀還沒定，所以它邊界上的 pin 位置也要在 floorplanning 時決定：最直覺的做法是看 fly line，把 pin 放在朝向連接對象的那一邊、減少 crossing（本機實測窮舉最小化線長後線長 44 → 28、交叉 4 → 0）；Pedram 的方法把 hierarchy 拆開、用 channel connectivity graph 做 global routing 來決定 pin 在哪個邊、哪個 slot，再重建 hierarchy，另一篇則先做 global routing 加粗略 pin assignment、再定 channel 的 final pin；bus 要把 pin flip 讓 MSB 對齊避免 twist，bus 交叉時要回頭調 floorplan；最難的是同一個 block 被 instantiate 很多次（MIB），pin 位置必須一樣，只能對所有 instance 妥協（實測多 30% 線長）。**

---

*筆記依據李炯霆「Electronic Design Automation (1)」Lecture 12 | ch5. Pin_Assignment 錄影（NTHU OCW，20:08；影片無字幕，以 whisper large-v3-turbo 轉錄）與投影片截圖整理。實測為本機（Intel i7-13650HX、Ubuntu、kernel 6.5.0、Python 3.10）結果。*
