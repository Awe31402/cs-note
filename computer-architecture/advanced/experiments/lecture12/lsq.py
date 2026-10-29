# lsq.py：第 12 支的三個小實驗
#  1. 漏掉 broadcast：dispatch 和 broadcast 同一個 cycle，指令會不會永遠等下去？加 dispatch buffer 之後呢？
#  2. before(j)、after(j) 的位元式子（課本 式 4.1–4.4）跟一格一格走的結果一樣嗎？（試遍所有 head、tail、j）
#  3. 照 表 4.2 的規則跑 圖 4.25 的例子，最後用「選最右邊的 1」找出要停在哪裡
from itertools import product

# ---------- 1. 漏掉 broadcast ----------
def race(fix):
    # cycle 1：consumer 讀 rename table，p1 的 avlbl = 0
    avlbl, ready = 0, None
    seen = avlbl
    # cycle 2：consumer 正在寫進 window；同一個 cycle producer broadcast p1、設 avlbl = 1
    avlbl = 1
    ready = seen                      # 寫進 window 的是舊的 0，broadcast 沒被聽到
    dispatch_buf, tag_log = ['p1'], ['p1']
    # cycle 3：如果有修補，就拿這個 cycle 記下的 tag 跟 dispatch buffer 比一次
    if fix and set(dispatch_buf) & set(tag_log):
        ready = 1
    return ready                      # 之後再也不會有人 broadcast p1
print('1. 沒修補：ready =', race(False), '（永遠在等）')
print('   有 dispatch buffer：下一個 cycle ready =', race(True))

# ---------- 2. before / after ----------
def prec(i, N): return (1 << i) - 1
def bmap(i, N): return 1 << i
def neg(x, N): return ~x & ((1 << N) - 1)
def before(j, h, t, N):
    if j >= h: return neg(prec(h, N), N) & prec(j, N)          # 式 4.1
    return neg(prec(h, N), N) | prec(j, N)                       # 式 4.2
def after(j, h, t, N):
    if j <= t: return neg(prec(j, N), N) & neg(bmap(j, N), N) & (prec(t, N) | bmap(t, N))   # 式 4.3
    return (neg(prec(j, N), N) | prec(t, N) | bmap(t, N)) & neg(bmap(j, N), N)              # 式 4.4
def walk(h, t, N):                     # 從 head 一格一格走到 tail
    q, i = [h], h
    while i != t: i = (i + 1) % N; q.append(i)
    return q
for N in (8, 16):
    bad = cnt = 0
    for h, t in product(range(N), repeat=2):
        q = walk(h, t, N)
        for k, j in enumerate(q):
            want_b = sum(1 << x for x in q[:k]); want_a = sum(1 << x for x in q[k + 1:])
            cnt += 1
            bad += (before(j, h, t, N) != want_b) + (after(j, h, t, N) != want_a)
    print(f'2. N={N}：試了 {cnt} 組 (head, tail, j)，式子跟一格一格走不一樣的次數：{bad}')

# ---------- 3. 圖 4.25 ----------
#  index: 0  1  2  3  4  5  6     （0 最早）
#  type : L  S  S  L  L  S  L
#  addr : -  X  -  A  B  ?  C     X、A、B、C 都是同一個位址 a；index 2 是別的位址
q = [('L', 'b'), ('S', 'a'), ('S', 'c'), ('L', 'a'), ('L', 'a'), ('S', None), ('L', 'a')]
N, h, t = len(q), 0, len(q) - 1
vec = lambda f: sum(1 << i for i, e in enumerate(q) if f(e))
store = vec(lambda e: e[0] == 'S'); load = vec(lambda e: e[0] == 'L')
match = vec(lambda e: e[1] == 'a'); resvd = vec(lambda e: e[1] is not None)
def first_one(v):                      # 最早的 1：「選左邊」的 select 樹（這裡 bit 0 在左）
    return (v & -v).bit_length() - 1 if v else None
def last_one(v):                       # 最晚的 1：「選右邊」的 select 樹
    return v.bit_length() - 1 if v else None
bits = lambda v: ''.join('1' if v >> i & 1 else '0' for i in range(N))
j = 1                                  # store X 剛算出位址
aft = after(j, h, t, N)
stop = aft & store & (match | neg(resvd, N))
s = first_one(stop)
win = aft & prec(s, N) if s is not None else aft
fwd = win & load & match
print('3. store X（index 1）算出位址 a：')
print('   after(1)               =', bits(aft))
print('   之後、同位址或沒解析的 store =', bits(stop), '→ 停在 index', s)
print('   範圍內、同位址的 load     =', bits(fwd), '→ 轉送給 index', [i for i in range(N) if fwd >> i & 1])
j = 6                                  # load C 算出位址 a
bef = before(j, h, t, N)
stop = bef & store & (match | neg(resvd, N))
s = last_one(stop)
print('   load C（index 6）算出位址 a：before(6) 裡同位址或沒解析的 store =', bits(stop), '→ 最晚的是 index', s,
      '：', '沒解析，等' if not resvd >> s & 1 else '轉送')
