# Advanced Computer Architecture 第七支影片完整筆記：Fetch 邏輯：一次抓多條、multi-way branch 與 IST（Chapter 3, Part 1/3）

> **課程**：Advanced Computer Architecture — Chapter 3: The Fetch and Decode Stages - Part I/III (fetch logic)（播放清單第 7 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）
> **來源**：**whisper 轉錄**（本機 `large-v3-turbo`；YouTube 自動字幕辨識成錯的語言，不能用）（42:36）＋ 課本 *Next-Gen Computer Architecture* v3.2 **Ch3** §3.1–3.2.1（p.61–68）
> **章號**：影片 Chapter 3 = 課本 Ch3。
> **標記說明**：
> - `【口頭】`：老師在影片裡口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這支影片沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（本講沒有）
> - `【實測】`：在本機（Intel i7-13650HX、Ubuntu、gcc 11.4、valgrind 3.18、Python 3.10）實際執行的結果
> - `⚠️`：逐字稿缺字、或影片／課本／實測說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 先修知識](#1-先修知識)
- [2. Fetch 與 decode 的理想狀況](#2-fetch-與-decode-的理想狀況)
- [3. 要的是頻寬，不是延遲](#3-要的是頻寬不是延遲)
- [4. 一次抓一條：沒時間看指令](#4-一次抓一條沒時間看指令)
- [5. 一次抓四條：multi-way branch](#5-一次抓四條multi-way-branch)
- [6. 三個問題](#6-三個問題)
- [7. Instruction status table（IST）](#7-instruction-status-tableist)
- [8. 為什麼用最低的 n 個 bit](#8-為什麼用最低的-n-個-bit)
- [9. Aliasing 與 tag](#9-aliasing-與-tag)
- [10. 為什麼 IST 有用](#10-為什麼-ist-有用)
- [11. 【課本補充】一次讀兩個 cache line](#11-課本補充一次讀兩個-cache-line)
- [12. 【實測】](#12-實測)
- [13. 一頁速查表](#13-一頁速查表)
- [14. 與後續講次的連結](#14-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

第 3 章是 OOO pipeline 的**前端**：fetch 和 decode。三講：

| 講 | 內容 |
|---|---|
| **本講（第 7 支）** | fetch 的邏輯、三個要預測的問題、第一個問題（是不是 branch）：**IST** |
| [第 8 支](lecture08_note.md) | 第二、三個問題：branch 跳不跳（branch predictor）、跳到哪（BTB、RAS） |
| [第 9 支](lecture09_note.md) | decode 時的最佳化（主要是 CISC） |

---

## 1. 先修知識

[00:58] 三樣：

1. bit 和布林邏輯（Basic [第 3 支](../basic/lecture03_note.md)）。
2. 第 2 章（OOO 的動機）：為什麼要 branch prediction、為什麼準確率要 **98% 以上**（[第 5 支 §12](lecture05_note.md#12-要填滿-windowbranch-一定要猜得很準)）。
3. RISC、CISC ISA 的取捨和格式。

---

## 2. Fetch 與 decode 的理想狀況

[02:34] 關鍵的狀態元件是 **PC**：指向下一條要從 **I-cache** 抓的指令。

- RISC 指令長度固定，通常 4 或 8 byte，這裡假設 4 byte。位址就是 PC、PC + 4、PC + 8……
- **Fetch width** > 1（一個 cycle 抓 2、3、4 條）：同時拿 PC、PC + 4、PC + 8……去 I-cache 讀。

[03:20] **Decode**：指令是一個「壓縮過的包裹」：用盡量少的 bit 表示資訊（不是一般意義的壓縮）。decoder 把它展開，例如把 16 位元的 immediate 展開成 32 位元。展開後的叫 **instruction packet**。

[04:54] 一次讀 4 條，就有 4 個**平行的 decoder**，產生 4 個 instruction packet 往下送。

[05:34] 這是**理想化**的看法。**Branch** 會讓控制流偏離：位址不再是乖乖的 PC、PC + 4……，可能跳到別的地方。一次抓 4 條，其中任何幾條都可能是 branch。

【課本補充】課本 p.62：instruction packet 的定義：在 pipeline 每個 stage 處理一條指令所需的一包資訊，在 decode 產生，通常包含來源和目的暫存器的編號、immediate、branch 和記憶體 offset、特殊旗標、控制執行單元的訊號。

---

## 3. 要的是頻寬，不是延遲

[06:19] 要盡量多抓**正確路徑**上的指令：branch 要猜對，才能一次給後面很多條。

[07:21] **Fetch 是 pipeline 的頭**。fetch 慢，整條 pipeline 就慢：就算程式的 ILP 很高，抓進來的指令不夠，也用不上。

> 我們不在乎 fetch 的**延遲（latency）**，在乎的是**頻寬（bandwidth）**。

【課本補充】課本 p.64：「鏈子的強度取決於最弱的那一環」：pipeline 每一環的強度，是它能送給下一級幾條指令。

---

## 4. 一次抓一條：沒時間看指令

[08:03] 最簡單：每個 cycle 抓 1 條。

- 抓第 i 條時，我們**只有它的 PC**。拿 PC 去 I-cache 讀內容，但這個 cycle 還**沒看到**、更沒 decode，**不知道它是不是 branch**。
- [08:46] 可是**下一個 cycle** 就要抓第 i + 1 條，現在就得決定位址：
  - 如果第 i 條是 branch：要預測它跳不跳、跳到哪。
  - 如果不是：PC + 4。
- [09:30] 第 i + 1 條也一樣，來不及看就得決定第 i + 2 條的位址。

[10:12] 用粗體大字說：

> **完全沒有時間看指令的內容。只知道它的 PC，就要預測下一條。**

---

## 5. 一次抓四條：multi-way branch

[10:54] 一個 cycle 抓 4 條，假設這 4 條（很可能）在正確的路徑上。現在要預測**下一組 4 條**的位址：

- 看這一組**最後一條**的 PC：下一組第一條是 PC + 4，或者（如果最後一條是 branch）它的 target。
- 然後下一條、再下一條……

[12:32] 難的是：每一條都可能是一般指令或 branch，它後面那條在正確路徑上又可能是一般指令或 branch……

[13:23] 4 條裡可能有好幾個 branch，所以有**很多個可能的 target、很多條可能的正確路徑**。一個 cycle 要做的不是 1 個預測，而是 **4 個**。

[14:04] 比較：

| Fetch width | 下一條的可能 |
|---|---|
| 1 | 只有兩種：PC + 4 或 target |
| 2 以上 | PC → PC + 4 → PC + 8，或 PC → target → target + 4，或 target 又是 branch…… 很多種 |

[16:16] 一組指令裡有好幾個 branch 時，從同一個起點出發、接在這組後面的指令有很多種可能，叫 **multi-way branch**。

- 一組裡**一個** branch：相對容易。
- **兩個以上**：後面那個 branch 的 PC 取決於前一個的預測，變成**循序**的，難得多。
- 【口頭】這個詞第 5 章會常用。

[17:44] 對策：**聰明的 compiler** 把 branch 拉開，讓連續 4 條（正確路徑上的）裡**最多一個** branch，減輕 predictor 的負擔，硬體也能簡單一點。

---

## 6. 三個問題

[18:29] 給一個 PC，要預測（課本 p.64）：

| 問題 | 內容 | 本講或下一講 |
|---|---|---|
| **Problem 1** | 這條是不是 branch？是哪一種？ | **本講** |
| **Problem 2** | 如果是 branch，taken 還是 not taken？ | [第 8 支](lecture08_note.md) |
| **Problem 3** | 如果 taken，target 是哪裡？ | [第 8 支](lecture08_note.md) |

[20:07] 做成一個**黑盒子 predictor**：輸入 PC，輸出預測（可以直接是 branch 的 target）。

---

## 7. Instruction status table（IST）

[20:55] 觀察：**同一個 PC 的指令類型永遠不變**。程式在跑的時候程式碼不會變。

[21:25] 做法：每次看到一個 branch，就**記住**它的 PC 和類型。下次看到同一個 PC，查一下有沒有記錄：

| 類型 | |
|---|---|
| 不是 branch | |
| 條件 branch | 依前一個 compare 的結果 |
| 無條件 branch | |
| 函式呼叫（call） | |
| 回傳（return） | |

[22:09] 5 種 → ⌈log₂ 5⌉ = **3 個 bit**。

[23:01] 硬體結構（「這個結構請非常注意看，整本書會一直用」）：

```
32 位元的 PC
     │
     └─ 取最低的 n 個 bit ──→ 2ⁿ 個 entry 的表 ──→ 類型（3 bit）
```

- [25:26] 這張表叫 **IST**（instruction status table）。老師說這是暫時的名字，之後加入更多資訊會改叫 **BTB**（[第 8 支](lecture08_note.md)）。
- [36:14] 執行（或 decode）時知道真的類型，就寫進對應的 entry；fetch 時讀出來當預測。

[23:51] 假設：PC 是 32 位元（實際多半 64 位元，換掉就好）。⚠️ 真的 RISC 處理器 PC 通常對齊 4 byte（最低 2 bit 永遠是 00），老師說為了簡單**不用**這個假設，讓 PC 可以是任意值；實作時一定要考慮對齊。（本機的 x86 指令長度不固定、本來就沒有對齊，見 [§12](#12-實測)。）

[27:48] 為什麼只用 10 個 bit（1024 entry）、不用全部 32 bit？2³² ≈ **40 億**個 entry：太大、太慢、太耗電、太佔面積，晶片放不下。

---

## 8. 為什麼用最低的 n 個 bit

[31:39] 為什麼不用最高的 n 個 bit，或中間的？

[32:16] 【口頭】數 1 到 100，看 27、28、29、30、31、32：**個位數**變得比十位數頻繁得多。隨機取 11–99 的數，個位數也比十位數多樣得多。

[33:42] 程式的兩個性質：

- **Spatial locality**：存取一個位址，很快就會存取附近的位址。
- **Temporal locality**：同樣的資料或指令會一再被存取。

[34:23] 程式大約 **90%** 的時間在迴圈裡，在一小塊程式碼裡跑來跑去：

- 這一小塊裡，**最高的幾個 bit 幾乎不變** → 用 MSB 當 index，大部分指令擠到同一個 entry，aliasing 超嚴重。
- 變化都在**最低的幾個 bit** → 用 LSB，分散得很平均，aliasing 少很多。

（本機實測：用中間或高位的 bit，準確率掉到 59%，比「一律猜不是 branch」還差，見 [§12](#12-實測)。）

---

## 9. Aliasing 與 tag

[29:29] 兩條指令的最低 n 個 bit 一樣，就會對到**同一個 entry**：

- 一條是 branch、一條不是：一下被這條蓋、一下被那條蓋，輪流猜錯。
- [30:48] 這叫 **aliasing** 或 **destructive interference**（破壞性干擾）。原因是表很小，只用 10 個 bit、不用全部 32 個。

[35:53] 解法：**disambiguate**（消除混淆）：在每個 entry 多存一些資訊。

[36:52] 把 PC 剩下的 32 − n 個 bit 也存進 entry，叫 **tag**：

```
PC = [ tag（32 − n bit） | index（n bit） ]
          │                    │
          │                    └─→ 選 entry
          └─→ 跟 entry 裡存的 tag 比；一樣才用這個 entry 的類型
```

[38:27] n 和 tag 都對上，就確定是**這一條**指令的記錄，不是別人的。

| 設計 | 好處 | 壞處 |
|---|---|---|
| 沒有 tag | 表很「瘦」 | aliasing |
| 完整 tag（n = 10 時 22 bit） | 沒有 aliasing | 每個 entry 胖很多 |
| 部分 tag（例如 10 bit） | 大幅減少 aliasing、比完整 tag 省空間 | 還有一點 aliasing |

[39:09] 有 tag 還多一個好處：**知道有沒有這條指令的記錄**。沒有的話怎麼猜？[39:59] 大約只有 **20%** 是 branch、80% 不是，所以猜「**不是 branch**」。

【課本補充】課本 p.66–67：課本用 22 bit tag ＋ 10 bit index，每個 entry 存 22 bit tag ＋ 1 個 status bit（只記是不是 branch）共 23 bit；§3.2.1（p.68）才擴充成 3 個 status bit。課本的 3 bit 編碼：000 不是 branch、001 條件 branch、010 無條件 branch、011 call、100 return。課本也指出：兩條指令的最低 10 bit 一樣，表示它們差 1024 的倍數，而兩條都很常被執行的機會不大，所以 aliasing 其實不常見。

---

## 10. 為什麼 IST 有用

[40:45] 因為 **temporal locality**：程式一直在迴圈裡跑同樣的指令（for、while……）。用 LSB 存住程式的一小塊，就能很準地預測指令類型。

[41:28] 下一講：預測 branch 跳不跳。會用類似的原理擴充 IST，而且會利用更多「程式通常怎麼寫」的特性。

---

## 11. 【課本補充】一次讀兩個 cache line

影片沒講，課本 §3.1（p.62–64）：

- I-cache 是一排 **cache line**（cache block），一次讀寫一整條，通常 **32 或 64 byte**（8–16 條 4 byte 的指令）。
- 要抓 4 條（16 byte），可能**跨兩條 cache line**，所以 I-cache 至少要能**同時讀兩條 line**（2 個 read port）。少於這個會嚴重限制 OOO pipeline 的 IPC。
- **Port**：cache 對外的介面；read port 只能讀、write port 只能寫；任何記憶體結構至少要一個 write port。
- 4 條裡第 2 條是 taken branch：第 1、2 條在 line i，第 3、4 條在 line j（target 所在）。但抓 line i 時，不知道哪條是 branch、跳不跳、跳到哪，所以不知道 j。只能送出 2 條，**吞吐量掉到 50%**。
- 所以要在**上一個 cycle** 就知道 j：這就是三個問題的由來。
- 4 條裡有**兩個** branch 時，需要同時讀 3 條 line，2 個 read port 不夠；這種情況機率低，可以請 compiler 避免，或加 read port。

---

## 12. 【實測】

`experiments/lecture07/`：

1. `work.c`：一個小程式，`qsort` 2 萬個亂數、再用 `snprintf` 轉字串。用 `-static -no-pie` 編譯，讓所有程式碼（包括 libc）都在固定位址。
2. 用 `valgrind --tool=lackey --trace-mem=yes` 記下**每一條被執行的指令的 PC**（照執行順序）。
3. `ist.py`：用 `objdump -d` 查每個 PC 的真正類型（`j*` 條件、`jmp`、`call`、`ret`、其他），再照課本的 IST 做：每條指令先**猜**它的類型，再用真的類型**更新**表。表有 1024 個 entry。

```
10068446 條動態指令、6640 個不同的 PC；類型：none 79.1%、cond 11.9%、call 3.8%、ret 3.8%、jmp 1.2%
IST 設計（1024 entry）                    全部指令猜對   branch 猜對
LSB 10 bit，沒有 tag                       97.38%      93.36%
LSB 10 bit，4 bit tag                      98.36%      92.54%
LSB 10 bit，8 bit tag                      98.44%      92.54%
LSB 10 bit，完整 tag                       98.44%      92.54%
中間 bit 6–15，沒有 tag                    59.28%       1.27%
高位 bit 14–23，沒有 tag                   59.13%       0.97%
高位 bit 14–23，完整 tag                   79.13%       0.00%
一律猜「不是 branch」                      79.13%       0.00%
```

- **20.9% 是 branch**（條件 11.9% ＋ call 3.8% ＋ ret 3.8% ＋ jmp 1.2%），跟影片 [39:59] 說的「大約 20%」一樣。
- **用 LSB 當 index 很有效**：沒有 tag 就 97.4%；加 tag 變 98.4%。4 bit tag 就幾乎跟完整 tag 一樣好，跟 [§9](#9-aliasing-與-tag) 說的「部分 tag 就能大幅減少 aliasing」一致。
- **用中間或高位的 bit 很糟**：59%，比「一律猜不是 branch」（79%）**還差**。這個程式的熱門程式碼集中在一小塊，最高的 bit 幾乎都一樣，大家擠在少數幾個 entry 裡互相覆蓋。就是 [§8](#8-為什麼用最低的-n-個-bit) 的道理。（「中間 bit 6–15」等於忽略最低 6 個 bit，一個 64 byte 範圍內的所有指令共用一個 entry，結果一樣糟。）
- **加 tag 後 branch 反而少猜對一點**（93.4% → 92.5%）：沒有 tag 時，aliasing 有時會「碰巧」猜對（例如兩個 branch 類型一樣）；有 tag 時，查不到就一律猜「不是 branch」。但整體還是 tag 比較好。
- 剩下的 1.6% 猜錯，主要是**第一次**看到的指令（還沒有記錄），以及 6640 個 PC 塞進 1024 個 entry 造成的互相擠掉。
- x86 指令長度不固定（1–15 byte），PC 本來就**沒有對齊**，正好是影片 [24:40] 說的「PC 可以是任意值」的情況。

---

## 13. 一頁速查表

| 主題 | 重點 |
|---|---|
| Fetch width | 一個 cycle 抓幾條；PC、PC + 4、PC + 8…… |
| Decode | 把「壓縮的包裹」展開成 instruction packet；多條就多個 decoder |
| 要什麼 | fetch 的**頻寬**（fetch 是 pipeline 的頭） |
| 最重要的一句 | 沒時間看指令內容，只有 PC |
| Multi-way branch | 一組指令裡多個 branch → 很多種可能的路徑；compiler 應把 branch 拉開 |
| 三個問題 | 是不是 branch？跳不跳？跳到哪？ |
| IST | PC 的最低 n bit 選 2ⁿ 個 entry；存 3 bit 類型；之後改名 BTB |
| 類型 | 不是 branch、條件、無條件、call、return |
| 為什麼 LSB | locality：熱門程式碼的高位 bit 幾乎不變 |
| Aliasing | 最低 n bit 相同的兩條擠同一個 entry |
| Tag | 存其餘的 bit；對上才算；查不到猜「不是 branch」（80%） |
| 為什麼有用 | temporal locality：90% 時間在迴圈裡 |
| 課本補充 | cache line 32–64 byte；I-cache 至少 2 個 read port |
| 實測 | 20.9% 是 branch；LSB 97.4%、加 tag 98.4%；高位 bit 只有 59% |

---

## 14. 與後續講次的連結

| 本講觀念 | 在哪裡 |
|---|---|
| 為什麼 predictor 要 98% 以上準 | [第 5 支 §12](lecture05_note.md#12-要填滿-windowbranch-一定要猜得很準) |
| Instruction packet（in-order 版） | [第 4 支](lecture04_note.md)、[Basic 第 30 支](../basic/lecture30_note.md) |
| Branch 跳不跳、target（把 IST 擴充成 BTB） | [第 8 支](lecture08_note.md) |
| Decode 時的最佳化 | [第 9 支](lecture09_note.md) |
| Cache line、spatial／temporal locality | [Basic 第 34 支](../basic/lecture34_note.md)、[第 21 支](lecture21_note.md)（Ch7） |
| x86 指令長度不固定 | [Basic 第 14 支](../basic/lecture14_note.md) |

---

## 附：本講一句話總結

> **OOO pipeline 的 fetch 是整條 pipeline 的頭，要的是頻寬而不是延遲：每個 cycle 要從 I-cache 抓好幾條正確路徑上的指令，但抓的當下只知道 PC、完全沒時間看指令內容，就得預測下一組的位址，一組裡有好幾個 branch 時還會變成很難預測的 multi-way branch；所以要解三個問題：是不是 branch、跳不跳、跳到哪，第一個用 IST 解決：拿 PC 最低的 n 個 bit 去查一張 2ⁿ 個 entry、每格 3 bit 記住類型的表（用 LSB 是因為程式有 locality，熱門程式碼的高位 bit 幾乎不變），兩條指令擠到同一格的 aliasing 可以用存其餘 bit 的 tag 解決，查不到就猜「不是 branch」（因為約 80% 都不是）。本機用 valgrind 記下 1000 萬條真實指令：21% 是 branch，用 LSB 的 IST 猜對 97.4%、加 4 bit tag 就到 98.4%，改用高位 bit 卻只剩 59%，比一律猜「不是 branch」還差。**

---

*筆記依據 Smruti R. Sarangi「Advanced Computer Architecture」Chapter 3: The Fetch and Decode Stages - Part I/III (fetch logic) 的 whisper 轉錄稿（本機 `large-v3-turbo`，42:36）、Sarangi *Next-Gen Computer Architecture* v3.2 Ch3 §3.1–3.2.1（p.61–68）整理。實測為本機（Intel i7-13650HX、Ubuntu、gcc 11.4、valgrind 3.18、Python 3.10）結果。*
