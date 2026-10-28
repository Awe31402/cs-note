# 把數字加 1 的 Turing machine。tape 用 dict 模擬無限長紙帶。
import sys
def run(s, handle_overflow):
    tape = dict(enumerate(s)); head = len(s) - 2; state = 'S1'
    while state != 'exit':
        sym = tape.get(head, ' ')
        if state in ('S0', 'S1'):
            if sym == '$':
                if state == 'S1' and handle_overflow:   # 課本：pre-exit
                    tape[head] = '1'; state = 'pre-exit'; head -= 1
                else:                                   # 影片：碰到 $ 就結束
                    state = 'exit'
            else:
                c = int(state[1]); d = int(sym) + c
                tape[head] = str(d % 10); state = 'S%d' % (d // 10); head -= 1
        elif state == 'pre-exit':
            tape[head] = '$'; state = 'exit'
    return ''.join(tape[i] for i in sorted(tape))
for s in ['$73469$', '$999$']:
    print(f"{s:8} video→ {run(s, False):8} book→ {run(s, True)}")
