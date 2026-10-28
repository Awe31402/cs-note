"""課本 §13.2 的編碼：把 logical bit 變成每個 bit period 兩個半格的 physical 電壓（H／L／I=idle），
再用一個「時脈慢了 2%」的接收端解回來。接收端每看到一次電壓變化（transition）就重新對齊自己的取樣時間，
兩次變化之間只能靠自己的（不準的）時脈數半格。看哪種編碼在長串相同 bit 時會讀錯。"""
import random

def encode(bits, scheme):
    out, level = [], 'L'
    for b in bits:
        if scheme == 'active-high': out += ['H', 'H'] if b else ['L', 'L']
        elif scheme == 'RZ':         out += ['H', 'I'] if b else ['L', 'I']
        elif scheme == 'Manchester': out += ['L', 'H'] if b else ['H', 'L']   # 1：0→1、0：1→0
        elif scheme == 'NRZI':                                                # 1：轉換、0：不變
            if b: level = 'H' if level == 'L' else 'L'
            out += [level, level]
    return out

def stuff(bits, k=6):
    """每 k 個 0 之後硬塞一個 1（我們的 NRZI 裡 1 才有 transition）"""
    out, run = [], 0
    for b in bits:
        out.append(b); run = run + 1 if b == 0 else 0
        if run == k: out.append(1); run = 0
    return out

def unstuff(bits, k=6):
    out, run, skip = [], 0, False
    for b in bits:
        if skip: skip = False; run = 0; continue
        out.append(b); run = run + 1 if b == 0 else 0
        if run == k: skip = True
    return out

def receive(wave, drift):
    """傳送端每半格 1.0 個單位；接收端以為每半格是 1+drift。
    每次電壓改變就重新對齊；之後每隔（自己的）半格取一個樣本。"""
    edges = [0.0] + [i for i in range(1, len(wave)) if wave[i] != wave[i - 1]] + [len(wave)]
    samples = []
    for a, b in zip(edges, edges[1:]):                # 兩次變化之間：這段電壓不變
        n = round((b - a) / (1 + drift))              # 接收端數出幾個半格
        samples += [wave[int(a)]] * max(n, 1)
    return samples

def decode(samples, scheme):
    pairs = [samples[i:i + 2] for i in range(0, len(samples) - 1, 2)]
    if scheme == 'active-high': return [1 if p[0] == 'H' else 0 for p in pairs]
    if scheme == 'RZ':          return [1 if p[0] == 'H' else 0 for p in pairs]
    if scheme == 'Manchester':  return [1 if p == ['L', 'H'] else 0 for p in pairs]
    if scheme == 'NRZI':
        out, prev = [], 'L'
        for p in pairs: out.append(int(p[0] != prev)); prev = p[0]
        return out

random.seed(1)
msg = [random.randint(0, 1) for _ in range(200)] + [1] * 100 + [0] * 100 + [random.randint(0, 1) for _ in range(200)]
print(f'訊息：600 個 bit（中間有連續 100 個 1 和連續 100 個 0），接收端時脈慢 2%')
for scheme in ('active-high', 'RZ', 'Manchester', 'NRZI'):
    wave = encode(msg, scheme)
    got = decode(receive(wave, 0.02), scheme)
    trans = sum(wave[i] != wave[i - 1] for i in range(1, len(wave)))
    ok = got[:len(msg)] == msg and len(got) == len(msg)
    print(f'  {scheme:12s} 電壓變化 {trans:4d} 次 | 解出 {len(got):3d} 個 bit | {"全對" if ok else "錯了"}')
s = stuff(msg)
wave = encode(s, 'NRZI'); got = unstuff(decode(receive(wave, 0.02), 'NRZI'))
trans = sum(wave[i] != wave[i - 1] for i in range(1, len(wave)))
print(f'  NRZI+塞位元   電壓變化 {trans:4d} 次 | 多傳 {len(s) - len(msg)} 個 bit | 解出 {len(got):3d} 個 bit | {"全對" if got == msg else "錯了"}')
