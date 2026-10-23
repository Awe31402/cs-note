# Real Time Systems 第三講完整筆記：時間限制的建模

> **課程**：Real Time Systems — Lecture 3：Modeling（播放清單第 3 支）
> **講者**：Smruti R. Sarangi（IIT Delhi）；投影片與課本為 Rajib Mall（IIT Kharagpur）
> **來源**：YouTube 錄影逐字稿（37:39，自動字幕）＋ 課本 *Real-Time Systems*（Rajib Mall；IIT Kharagpur「Version 2 CSE」講義版 PDF）Ch1 §6–8（PDF p.16–27）
> **標記說明**：
> - `【口頭】`：老師在課堂上口頭補充、課本沒有的內容
> - `【Q&A】`：學生主動提問與老師回答（本講沒有）
> - `【課本補充】`：課本有、但這堂課沒細講的內容
> - `【原始碼】`：Linux v6.2.12 原始碼節錄（`檔名:行號`），用來對照觀念
> - `【實測】`：在本機（Ubuntu、kernel 6.5.0、x86-64）實際執行的結果
> - `⚠️`：逐字稿缺字、或課堂／課本／原始碼說法不一致之處
> - `[mm:ss]`：影片時間點

---

## 目錄

- [0. 本講地圖](#0-本講地圖)
- [1. 硬、堅定、軟即時系統](#1-硬堅定軟即時系統)
  - [1.1 Hard real-time](#11-hard-real-time)
  - [1.2 Firm real-time](#12-firm-real-time)
  - [1.3 Soft real-time](#13-soft-real-time)
  - [1.4 【課本補充】非即時任務、時間範圍](#14-課本補充非即時任務時間範圍)
- [2. 任務的種類：週期、偶發、非週期](#2-任務的種類週期偶發非週期)
- [3. 事件：stimulus 與 response](#3-事件stimulus-與-response)
- [4. 時間限制的分類](#4-時間限制的分類)
  - [4.1 Performance vs Behavioral](#41-performance-vs-behavioral)
  - [4.2 三個 D：Delay、Deadline、Duration](#42-三個-ddelaydeadlineduration)
  - [4.3 SS、SR、RS、RR](#43-sssrrsrr)
  - [4.4 電話系統的例子](#44-電話系統的例子)
  - [4.5 分類樹](#45-分類樹)
- [5. 用 EFSM 建模](#5-用-efsm-建模)
  - [5.1 為什麼要建模](#51-為什麼要建模)
  - [5.2 FSM 與 EFSM 的畫法](#52-fsm-與-efsm-的畫法)
  - [5.3 Deadline 的模型（SS、RS、SR、RR）](#53-deadline-的模型ssrssrrr)
  - [5.4 Delay 的模型：alarm 反而是好事](#54-delay-的模型alarm-反而是好事)
  - [5.5 Duration 的模型：拆成 delay 與 deadline](#55-duration-的模型拆成-delay-與-deadline)
  - [5.6 【原始碼】Linux 的計時器：set timer 與 timer alarm](#56-原始碼linux-的計時器set-timer-與-timer-alarm)
  - [5.7 【實測】用 timerfd ＋ poll 跑 EFSM](#57-實測用-timerfd--poll-跑-efsm)
- [6. 課後練習](#6-課後練習)
- [7. 一頁速查表](#7-一頁速查表)
- [8. 與後續講次的連結](#8-與後續講次的連結)
- [附：本講一句話總結](#附本講一句話總結)

---

## 0. 本講地圖

1. 依「錯過 deadline 的後果」分三類：hard、firm、soft。
2. 時間限制是加在**事件**上的。事件分 stimulus（環境 → 系統）和 response（系統 → 環境）。
3. 時間限制分兩大類（performance、behavioral）× 三個 D（delay、deadline、duration）。
4. 用 **EFSM**（擴充有限狀態機）把它們畫出來：轉移上多了「set timer」和「timer alarm」。

---

## 1. 硬、堅定、軟即時系統

[00:32]–[05:01]

任務有 deadline。依**錯過 deadline 的後果**分三類。

### 1.1 Hard real-time

- 錯過 deadline → **系統失效**。
- Deadline 可以是微秒、毫秒，甚至秒。
- 很多是安全關鍵的：飛機機載電腦、工業控制、國防設備、飛彈、機器人。

### 1.2 Firm real-time

- 偶爾錯過沒關係，**系統不失效**。
- 但 deadline 之後的結果**直接丟掉**：效用（utility）在 deadline 之後**掉到 0**。

```
utility
100% ┤■■■■■■■■■┐
     │         │
   0 ┼─────────┴──────── response time
               deadline
```

- 例：視訊會議。每秒 30 張 → 每 33 ms 一張。某張遲到就丟掉，可能有點抖動（jitter），繼續播下一張。
- 也包括遙測、衛星監視等：偶爾漏一張沒關係。

### 1.3 Soft real-time

- 錯過 deadline，系統也不失效。三類裡**最寬容**。
- 效用在 deadline 之後**慢慢降到 0**，不是一下子掉到 0。系統表現變差而已。

```
utility
100% ┤■■■■■■■■■╮
     │          ╲__
   0 ┼─────────────╲____ response time
               deadline
```

- 另一種定義：**機率式**要求。例如「99% 的 deadline 要趕上」；甚至可以再規定沒趕上的那 1% 裡，有多少可以晚多少。
- 例：鐵路或機票訂位（預期 10 秒、實際 30 秒，乘客不爽但沒事）、網頁瀏覽（等兩分鐘不舒服，但不會出大事）。所有互動式程式都是如此。

| | Hard | Firm | Soft |
|---|---|---|---|
| 錯過 deadline | 系統失效 | 結果丟掉，系統不失效 | 結果變差，系統不失效 |
| deadline 後的效用 | — | 立刻 0 | 慢慢降到 0 |
| 例子 | 飛機、飛彈、機器人 | 視訊會議、衛星追蹤 | 訂位、網頁 |

### 1.4 【課本補充】非即時任務、時間範圍

（p.16–19）

- 課本定義的是**任務**的分類（hard / firm / soft real-time **task**）。一個系統裡可以混著不同類的任務。
- 硬即時任務**不需要越快完成越好**，只要在 deadline 前完成。「提早完成沒有獎勵」——這點在排程章節很重要。
- 硬即時不一定安全關鍵。課本註腳：有些電腦遊戲有硬即時任務，失效只是讓玩家有點煩。
- 典型的時間範圍：

| 類別 | 時間範圍 |
|---|---|
| Hard | 幾微秒到幾毫秒 |
| Firm | 幾毫秒到幾百毫秒 |
| Soft | 零點幾秒到幾秒 |
| 非即時（non-real-time） | 幾分鐘、幾小時、甚至幾天（例：e-mail、批次工作、薪資單列印） |

⚠️ 老師說 hard real-time 的 deadline 可以到「秒」，課本說通常是微秒到毫秒。兩者並不衝突：課本講的是「通常」。

---

## 2. 任務的種類：週期、偶發、非週期

[05:01]–[06:30]（第一講講過，這裡複習）

| 種類 | 說明 |
|---|---|
| **週期（periodic）** | 每隔固定時間重複一次。大多數感測器任務是這樣 |
| **偶發（sporadic）** | 隨機時間出現，但兩次之間**有最小間隔** |
| **非週期（aperiodic）** | 和偶發一樣，但最小間隔可以是 **0**，完全沒限制 |

---

## 3. 事件：stimulus 與 response

[06:30]–[09:01]

時間限制是**針對事件**定義的。

- 事件可以發生在某個**瞬間**，也可以**持續一段時間**（例如持續 2 ms）。
- 依誰產生，分兩種：

| 種類 | 誰產生 | 例子 |
|---|---|---|
| **Stimulus event**（刺激事件） | 環境產生，經感測器進到系統。可非週期、也可週期 | 非週期：使用者按電話按鍵。週期：每 p ms 讀一次化工廠溫度 |
| **Response event**（回應事件） | 系統對刺激產生回應，作用在環境上 | 溫度超過 100°C → 關掉（或打開）加熱器 |

【課本補充】（p.19）有些作者認為「持續一段時間的事件」不是基本事件，可以拆成「開始」和「結束」兩個事件（例如「按下按鈕」和「放開按鈕」）。課本為了方便，仍把它當成一種特殊的事件。

---

## 4. 時間限制的分類

### 4.1 Performance vs Behavioral

[09:01]–[10:02]

| 類別 | 加在哪裡 | 意思 |
|---|---|---|
| **Performance constraint**（效能限制） | 系統的 **response** 上（輸出端） | 外人只看得到回應，回應是系統真正的輸出 |
| **Behavioral constraint**（行為限制） | 環境產生的 **stimulus** 上（輸入端） | 限制環境（使用者）要怎麼做 |

【課本補充】（p.20、p.27）behavioral 限制確保**環境守規矩**；performance 限制確保**電腦表現夠好**。如果環境不守 behavioral 限制，performance 限制就不一定保證得了。

### 4.2 三個 D：Delay、Deadline、Duration

[10:02]–[15:01]

老師說：這一講最重要的就是記住「三個 D」。

用 t(e) 表示事件 e 發生的時間。

| 限制 | 公式 | 意思 |
|---|---|---|
| **Delay**（延遲） | t(e₂) − t(e₁) ≥ d | e₂ **至少**要在 e₁ 之後 d 才發生。太早 → 錯誤 |
| **Deadline**（期限） | t(e₂) − t(e₁) ≤ d | e₂ **最多**在 e₁ 之後 d 內發生 |
| **Duration**（持續時間） | 一個事件持續多久 | 分最小型（至少持續 x）、最大型（最多持續 x） |

```
Delay:     e1 |<----- ≥ d ----->| e2        （偶發任務的最小間隔就是這個）
Deadline:  e1 |<----- ≤ d ----->| e2
Duration:  |====== 事件持續中 ======|      （開始到結束的長度）
```

- Delay 和 deadline 剛好相反：一個規定**最小**間隔，一個規定**最大**間隔。
- Deadline 例：e₁ = 從感測器讀到資料（還沒處理），e₂ = 處理完成。要求 t(e₂) − t(e₁) ≤ d，就是「收到資料後 d 時間內處理完」。
  - 【口頭】老師一開始把 e₂ 說成「deadline 到了」，後來更正為「處理完成」[12:01]。
- Duration 例【口頭】：按 Word 的存檔鍵，程式卡住，直到存好才恢復。從按下到恢復這段時間可以加最小或最大限制。
- 實際上 delay 和 deadline 最常用；duration 比較少用，但事件不是一個點、而是一段時間時很有用。

### 4.3 SS、SR、RS、RR

Delay 和 deadline 牽涉兩個事件，每個可以是 S（stimulus）或 R（response），所以有四種組合。

**判斷 performance 還是 behavioral：看第二個事件（被限制的那個）**

- 第二個是 R → 限制系統 → **performance**
- 第二個是 S → 限制環境 → **behavioral**

| 組合 | 第二個事件 | 類別 |
|---|---|---|
| SS | stimulus | behavioral |
| RS | stimulus | behavioral |
| SR | response | performance |
| RR | response | performance |

### 4.4 電話系統的例子

[15:01]–[23:03]（課本 p.21–22；用老式市話，因為步驟聽得見）

| 例子 | 種類 | 類別 |
|---|---|---|
| 撥完一個數字後，**5 秒內**要撥下一個，否則出現 idle tone | SS deadline | behavioral |
| 撥號音（dial tone）出現後，**30 秒內**要撥第一個數字，否則進入 idle 狀態、出現 idle tone | RS deadline | behavioral |
| 拿起話筒後，系統要在 **2 秒內**給撥號音，否則一直嗶嗶響直到放回話筒 | SR deadline | performance |
| A 打給 B：B 那邊開始響鈴（ring tone）後，**2 秒內**要讓 A 聽到回鈴音（ring back tone），否則掛斷 | RR deadline | performance |
| 撥完一個數字後，下一個數字**至少隔 1 秒**才能撥，否則嗶嗶響直到放回話筒 | SS delay | behavioral |
| 按鈕按住 **<15 秒** → 接本地總機；**15–30 秒** → 接國際總機；**>30 秒** → 放開後給撥號音 | duration | — |

【口頭】

- SR 沒撥號音，代表電話接不上最近的 PSTN（公共交換電話網）機房。手機撥號也有類似步驟，只是我們聽不到。
- SS delay 的意思：不能同時按兩個鍵，或連按太快。
- Duration：旅館房間電話的服務鍵就是這種。15、30 秒只是舉例，實際上沒人會按那麼久。老師的筆電電源鍵：短按螢幕刷新，長按關機。

⚠️ 老師在 [31:30] 講 RR 的 EFSM 時說它「again behavioral」。但他在 [17:32] 和課本 p.22 都說 RR 是 **performance**（兩個事件都是 response）。以 performance 為準。

⚠️ 課本 p.25 delay 例子寫成「不早於 **10 ms**」（Fig.25 也是 10 ms），課堂和課本 p.22 是 **1 秒**。

### 4.5 分類樹

課本 Fig.19（p.23）：

```
                    Timing Constraints
                 ┌──────────┴──────────┐
          Performance              Behavioral
        ┌─────┼──────┐          ┌─────┼──────┐
      Delay Deadline Duration  Delay Deadline Duration
      RR SR  SR RR              RS SS  SS RS
```

[23:30]–[24:01] 老師的總結：performance 限制的第二個字母都是 R；behavioral 限制的第二個字母都是 S。

---

## 5. 用 EFSM 建模

### 5.1 為什麼要建模

[24:01]–[24:32]

- 能**形式化地描述（formally specify）**系統。
- 描述好了，就能理解、分析，甚至**自動產生程式碼**。
- 更深入理解即時行為。

課本 p.23 補充：模型也能用來做驗證（verification）。

### 5.2 FSM 與 EFSM 的畫法

[24:32]–[26:30]

- 老師說這個方法可追溯到 1985 年 ⚠️（逐字稿為 "the surati"，應是課本參考文獻 [1] B. Dasarathy, *IEEE TSE* 1985）。
- **狀態**由某個屬性的值決定。例：電梯的狀態由「運動方向」決定 → 上、下、停。
- **轉移（transition）**標成 `事件 / 動作`。
- **EFSM（Extended FSM）** = 一般 FSM ＋ 兩個新東西：
  - 動作可以是 **set timer(時間)**
  - 事件可以是 **timer alarm**（計時器到期）

```
        e1 / set timer(20 ms)
  (s1) ──────────────────────▶ (s2)
```

意思：在 s1 時發生 e1 → 設 20 ms 計時器 → 進入 s2。

**規則**【口頭】[31:01]：一個狀態有多條出邊時，**先發生的那個事件**決定走哪條。

### 5.3 Deadline 的模型（SS、RS、SR、RR）

四種 deadline 的模型長得一樣：「起點事件 / set timer(d)」→ 等待狀態 → 兩條出邊：

```
                          目標事件
                       ┌──────────▶ (下一個正常狀態)
 起點事件 / set timer(d) │
 ──────────────▶ (等待) ─┤
                       └──────────▶ (錯誤狀態)
                        timer alarm / 錯誤動作
```

| 種類 | 起點事件 / 動作 | 等待狀態 | 目標事件 → 狀態 | alarm → 動作、狀態 |
|---|---|---|---|---|
| SS [26:30] | 第一個數字 / set timer(5 s) | Await Second Digit | 下一個數字 / set timer(5 s) → Await Next Digit | idle tone → Await Caller On-hook |
| RS [28:02] | 撥號音 / set timer(30 s) | Await First Digit | 第一個數字 → Await Second Digit | idle tone → Idle |
| SR [30:00] | 拿起話筒 / set timer(2 s) | Await Dial Tone | 撥號音 → Await First Digit | 嗶嗶聲 → Await Receiver On-hook |
| RR [31:30] | B 響鈴 / set timer(2 s) | Await Ring-back Tone | 回鈴音 → Await First Digit | 掛斷 → Await Receiver On-hook |

【口頭】RS 的例子：題目沒講數字和數字之間的限制，所以老師把「撥第一個數字後」那條邊的動作留空 [29:00]。

⚠️ 課本 p.24 的 SS 文字寫「**5 milli seconds**」，接著又說計時器設 **20 ms**，Fig.21 標 **5 ms**；課本 p.22 和課堂都是 **5 秒**。SR 的文字也寫「**20 seconds**」，但 Fig.23 和課堂是 **2 秒**。課本這兩處前後不一致，以課堂為準。

### 5.4 Delay 的模型：alarm 反而是好事

[32:30]–[34:34]

SS delay：撥完一個數字，下一個至少隔 1 秒。

```
                              timer alarm
                         ┌──────────────▶ (Await Next Digit)   ← 好事：1 秒平安過去
 第一個數字 / set timer(1 s) │
 ──────────────────▶ (Await Next Event)
                         └──────────────▶ (Await Caller On-hook)
                          下一個數字 / 嗶嗶聲                   ← 太快了
```

和 deadline 反過來：

| | Deadline | Delay |
|---|---|---|
| timer alarm 先到 | 壞事（太晚了） | **好事**（已經等夠久） |
| 目標事件先到 | 好事 | **壞事**（太早了） |

### 5.5 Duration 的模型：拆成 delay 與 deadline

[34:34]–[36:30]

- Duration 限制**沒辦法直接**畫。
- 做法：把它換成等價的 delay 和 deadline 限制（針對事件的開始和結束），再照前面方式畫。
- 課本 Fig.26：加兩個中間狀態 Await Event1、Await Event2。

```
按下 / set alarm(15 s)
 ──▶ (Await Event1) ── 放開 ──────────────────────▶ (本地總機)
          │
          └ alarm / set alarm(15 s)
             ──▶ (Await Event2) ── 放開 ───────────▶ (國際總機)
                      │
                      └ alarm
                         ──▶ (Await Button Release) ── 放開 / 撥號音 ──▶ (Dial Tone)
```

### 5.6 【原始碼】Linux 的計時器：set timer 與 timer alarm

EFSM 的「set timer」和「timer alarm」，在 Linux kernel 裡最直接的對應是高精度計時器 `hrtimer`：

`include/linux/hrtimer.h:118`

```c
struct hrtimer {
	struct timerqueue_node		node;
	ktime_t				_softexpires;
	enum hrtimer_restart		(*function)(struct hrtimer *);
	struct hrtimer_clock_base	*base;
	u8				state;
	u8				is_rel;
	u8				is_soft;
	u8				is_hard;
};
```

`include/linux/hrtimer.h:65`

```c
enum hrtimer_restart {
	HRTIMER_NORESTART,	/* Timer is not restarted */
	HRTIMER_RESTART,	/* Timer must be restarted */
};
```

對照：

| EFSM | hrtimer |
|---|---|
| set timer(d) | 設好到期時間（`node` 放進依到期時間排序的 timerqueue），啟動 |
| timer alarm 事件 | 到期時呼叫 `function` callback |
| 到期後再設一次（duration 模型的第二個 15 s） | callback 回傳 `HRTIMER_RESTART` 並更新到期時間 |

使用者空間可以用 `timerfd`：把計時器變成一個檔案描述子，到期時變成「可讀」。這樣「等事件或 alarm，誰先到走誰」就是一次 `poll()`。

### 5.7 【實測】用 timerfd ＋ poll 跑 EFSM

寫一個 C 程式：子行程照劇本在指定時間送出事件（按鍵）到 pipe；父行程是 EFSM，用 `timerfd_settime()` 當 set timer，用 `poll()` 同時等 pipe 和 timerfd。時間縮小 10 倍（課堂 1 秒 = 這裡 100 ms）。

核心：

```c
static void set_timer(int ms) {        /* action: set timer(ms) */
  struct itimerspec it = { .it_value = { ms / 1000, (ms % 1000) * 1000000L } };
  timerfd_settime(tfd, 0, &it, NULL); }
/* wait for the first of {event, timer alarm}; returns event char or 'A' */
static char next(void) {
  struct pollfd p[2] = { { ev, POLLIN }, { tfd, POLLIN } };
  poll(p, 2, -1);
  if (p[0].revents & POLLIN) { char c; if (read(ev, &c, 1) == 1) return c; return 'E'; }
  unsigned long long n; read(tfd, &n, sizeof n); return 'A'; }
```

輸出：

```
== SS deadline, 3rd gap 700ms > 500ms
    0.2 ms  digit  -> set timer(500), Await Next Digit
  200.5 ms  digit  -> set timer(500), Await Next Digit
  500.7 ms  digit  -> set timer(500), Await Next Digit
 1000.9 ms  ALARM  -> idle tone, Await Caller On-hook
== SS delay, 2nd gap 30ms < 100ms
    0.3 ms  digit  -> set timer(100), Await Next Event
  100.5 ms  ALARM  -> (good) Await Next Digit
  150.4 ms  digit  -> set timer(100), Await Next Event
  180.6 ms  digit too early -> beeping, Await Caller On-hook
== duration: hold 1000ms (=10 s)
    0.2 ms  press  -> set alarm(1500), Await Event1
 1000.5 ms  release -> Local Operator
== duration: hold 2200ms (=22 s)
    0.2 ms  press  -> set alarm(1500), Await Event1
 1500.4 ms  ALARM  -> set alarm(1500), Await Event2
 2200.5 ms  release -> International Operator
== duration: hold 3500ms (=35 s)
    0.3 ms  press  -> set alarm(1500), Await Event1
 1500.4 ms  ALARM  -> set alarm(1500), Await Event2
 3000.7 ms  ALARM  -> Await Button Release
 3500.5 ms  release -> Dial Tone
```

看到什麼：

- SS deadline：第 4 個數字晚了 700 ms（> 500 ms），計時器在 500.7 + 500 = 1000.9 ms 到期 → idle tone。
- SS delay：第 2 個間隔只有 30 ms（< 100 ms），數字比 alarm 先到 → 嗶嗶聲。第 1 個間隔 150 ms 時，alarm 先到，是好事。
- Duration：按住 10 / 22 / 35「秒」分別走到三個不同狀態，和課本 Fig.26 一樣。
- 計時器的誤差約 0.2–0.9 ms（一般 Linux，沒有 PREEMPT_RT）。

---

## 6. 課後練習

[36:30]–[37:39] 老師出了兩題，要用本講的方法建 EFSM（這裡只列題目，不附答案）：

1. 化學反應爐溫度超過門檻 T°C 時，加熱器要在 10 ms 內關掉，否則整座工廠停機。
2. 找出並建模時間限制：洗衣機按下開始鍵後開始攪拌，直到預設計時器到期或使用者按停止鍵。

課本 p.27–30 另有很多練習（例如防空系統、太空船、電話系統的 EFSM），不在本筆記範圍。

---

## 7. 一頁速查表

### 7.1 三類即時

| | 錯過 deadline | 效用 |
|---|---|---|
| Hard | 系統失效 | — |
| Firm | 結果丟掉 | 立刻 0 |
| Soft | 表現變差 | 慢慢降 |

### 7.2 事件

Stimulus（環境 → 系統，感測器）／Response（系統 → 環境，致動器）。

### 7.3 時間限制

- Performance：限制 response（第二個字母 R：SR、RR）。
- Behavioral：限制 stimulus（第二個字母 S：SS、RS）。
- 三個 D：

| | 公式 |
|---|---|
| Delay | t(e₂) − t(e₁) ≥ d |
| Deadline | t(e₂) − t(e₁) ≤ d |
| Duration | 事件持續時間的上限或下限 |

### 7.4 EFSM

- 轉移：`事件 / 動作`；新動作 `set timer(d)`，新事件 `timer alarm`。
- 多條出邊：誰先發生走誰。
- Deadline：目標事件先到 = 好，alarm 先到 = 壞。
- Delay：alarm 先到 = 好，目標事件先到 = 壞。
- Duration：拆成 delay ＋ deadline，加中間狀態。
- Linux 對應：`hrtimer`（kernel）、`timerfd` ＋ `poll()`（使用者空間）。

---

## 8. 與後續講次的連結

| 本講觀念 | 後面在哪裡用到 |
|---|---|
| **週期 / 偶發 / 非週期任務** | **第 4 支**：Basics of Scheduling |
| **Deadline、硬即時任務「提早完成沒有獎勵」** | **第 5–9 支**：clock-driven、EDF、RMA |
| **Soft / firm 任務、非週期任務** | **第 8 支**：RMA 的延伸（非週期任務的處理） |
| **計時器（set timer / timer alarm）** | **第 18 支**：Hardware, Timers, and OSes |
| **POSIX 即時擴充（計時器、排程）** | **第 21 支**：POSIX standards |
| **Firm 即時的視訊會議、封包延遲** | **第 27–30 支**：Real Time Communication |

---

## 附：本講一句話總結

> **即時任務依錯過 deadline 的後果分成 hard（系統失效）、firm（結果作廢、效用立刻歸零，例：視訊會議丟幀）和 soft（效用慢慢下降，例：訂位、網頁，也可以用「99% 要趕上」這種機率式要求）；時間限制加在事件上，事件分環境產生的 stimulus 和系統產生的 response，限制分為管 response 的 performance 限制（SR、RR）和管 stimulus 的 behavioral 限制（SS、RS），每類再分三個 D：delay（t(e₂)−t(e₁) ≥ d）、deadline（≤ d）和 duration（事件持續多久）。這些限制可以用 EFSM 建模：在轉移上加 set timer 動作和 timer alarm 事件，誰先發生走誰——deadline 是 alarm 先到為壞，delay 則是 alarm 先到為好，duration 要拆成 delay 和 deadline 加中間狀態；實測用 Linux 的 timerfd ＋ poll 就能照圖跑出這些狀態機。**

---

*筆記依據 Smruti R. Sarangi「Real Time Systems」Lecture 3 錄影逐字稿（YouTube 自動字幕，37:39）、Rajib Mall *Real-Time Systems*（IIT Kharagpur Version 2 CSE 講義版）Ch1 §6–8 整理。原始碼為 Linux v6.2.12；實測為本機（Ubuntu、kernel 6.5.0、x86-64）結果。*
