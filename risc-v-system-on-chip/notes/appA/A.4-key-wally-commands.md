# A.4 Key Wally Commands

> 原書 p.784.e109、784.e127（Appendix A: Wally Synopsis）

## 這節在講什麼

Table A.10 的常用指令速查表，加上一段提交 pull request 的流程。這是整本書最實用的一頁——**把散在 23 章裡的指令集中在一起**。

## 關鍵點

### 前提假設

> "Table A.10 lists commonly used commands. They assume you have created an account with a username <yourgithubaccount> at github.com and forked openhwgroup/cvw to <yourgithubaccount>."
> — p.784.e109

中文解釋：標準的開源協作流程——**fork 到自己帳號，不是直接 clone 主倉庫**。這樣才能提交 PR。

### 指令分五類

**(1) 取得與建置**

```bash
git clone --recurse-submodules https://github.com/<yourgithubaccount>/cvw.git
```
**`--recurse-submodules` 不能漏**——A.1 提過 `addins/` 底下全是 submodule（SoftFloat、CoreMark、Embench、riscv-arch-test…），漏了就什麼測試都跑不了。

```bash
make --jobs                        # 在 cvw/：建所有測試（RISCOF、TestFloat、coverage）
make --jobs                        # 在 cvw/sim/：只重建 RISCOF 測試
make --jobs wally-riscv-arch-test  # 在 cvw/sim/：只重建自製測試（快很多）
```

**這三個的差別是「重建範圍」**。開發時改了自製測試，用第三個就好，不用等全部重建。

**(2) 檢查與回歸**

```bash
lint-wally                      # SystemVerilog 靜態檢查
regression-wally                # 跑所有測試案例
regression-wally --ccov          # 程式碼覆蓋率（第 13 章）
regression-wally --fcov          # 功能覆蓋率
regression-wally --testfloat     # TestFloat 測試（第 16.5 章）
regression-wally --nightly       # 完整版，要跑好幾小時
regression-wally --cache         # 快取效能驗證（第 21.3.1 章）
regression-wally --branch        # 分支預測器掃描（第 21.3.2 章，要半天）
```

**注意 `--nightly` 這個命名慣例**——業界標準做法是「commit 時跑快的回歸、每晚跑完整的」。

**(3) 單次模擬：`wsim`**

```bash
wsim rv32i arch32i --gui              # Questa GUI，看波形
wsim rv32i arch32i                    # Questa 批次模式（較快）
wsim rv32i arch32i --sim verilator    # 改用 Verilator
wsim rv64gc <path>.elf --lockstep     # 和 ImperasDV 逐指令比對
wsim rv64gc <path>.elf --fcov --lockstep  # 同時收集功能覆蓋率
wsim buildroot buildroot --args +INSTR_LIMIT=600000000 --lockstep  # 跑 Linux
```

**`wsim` 的參數結構是 `wsim <組態> <測試>`**——這對應 A.2 的 Table A.1（組態）和 A.3 的 Table A.7/A.8（測試）。三張表合起來就知道所有合法的組合。

**(4) Benchmark**

```bash
./coremark_sweep.py        # 在 cvw/benchmarks/coremark/
./embench_arch_sweep.py    # 在 cvw/benchmarks/embench/
```

兩者都會掃描多個組態，產生第 21 章那些表。

**(5) 合成**

```bash
make synth DESIGN=wallypipelinedcore TECH=sky130 CONFIG=synth_rv32e FREQ=330
```
在 `cvw/synthDC/`。四個參數對應第 6 章的 PPA 實驗：要合成哪個模組、哪個製程（`sky130`/`sky90`/`tsmc28psyn`）、哪個組態（A.2 Table A.5 的 `syn_*` 衍生組態）、目標頻率。

### 提交改動回主倉庫

> "$ git pull upstream main / $ git add <filenames> / $ git commit -m 'Describe edits' / $ git push"
> — p.784.e109

中文解釋：四步驟，關鍵在**第一步 `git pull upstream main`**——**先把上游的改動拉下來合併，再推自己的**。這樣 PR 才不會有衝突。

（`upstream` 是指向 `openhwgroup/cvw` 的 remote，`origin` 是自己的 fork。）

然後到 github.com/openhwgroup/cvw 按 New pull request 選自己的 fork。

## 名詞表

| 名詞 | 意思 |
| --- | --- |
| fork | 在 GitHub 上複製一份倉庫到自己帳號 |
| `--recurse-submodules` | clone 時一併取得所有 submodule |
| `lint` | 靜態檢查，找出可疑但語法合法的寫法 |
| ccov / fcov | code coverage（程式碼覆蓋率）／functional coverage（功能覆蓋率） |
| `--nightly` | 每晚跑的完整回歸（相對於 commit 時的快速回歸） |
| `wsim` | Wally 的模擬啟動腳本 |
| lockstep | 與 ImperasDV 參考模型逐指令比對 |
| `INSTR_LIMIT` | 模擬的指令數上限（Linux 不會自己結束） |
| upstream / origin | 主倉庫／自己的 fork |

## 所以呢

這張表的價值在於**把「想做某件事」對應到「打哪個指令」**：想看波形 → `wsim --gui`；想確認沒弄壞東西 → `regression-wally`；想量面積 → `make synth`；想跑 Linux → `wsim buildroot`。

最後一節 A.5 是方塊圖彙整。
