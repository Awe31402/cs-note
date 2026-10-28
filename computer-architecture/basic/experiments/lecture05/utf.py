# 各種字元在 UTF-8 / UTF-16 / UTF-32 下的長度與位元組
for ch in ['a', 'é', 'Ж', 'क', '中', '😀', '\U0010FFFF']:
    b = ch.encode('utf-8')
    print(f"U+{ord(ch):06X} utf8={len(b)}B {' '.join(f'{x:08b}' for x in b):40} "
          f"utf16={len(ch.encode('utf-16-le'))}B utf32={len(ch.encode('utf-32-le'))}B")
print("ord('A'), ord('a'), ord('0') =", ord('A'), ord('a'), ord('0'))
try:
    bytes([0xF8, 0x88, 0x80, 0x80, 0x80]).decode('utf-8')
except UnicodeDecodeError as e:
    print("5-byte sequence:", e.reason)
