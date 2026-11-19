"""Render the phase roadmap timeline. Usage: python3 roadmap-timeline.py <out.png>"""
import sys
from PIL import Image, ImageDraw, ImageFont

FONT = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
BOLD = "/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc"
TC = 3  # face index: Traditional Chinese in the .ttc collection

phases = [
    ("P0", "環境與護欄", 6, "#8FB3D9"),
    ("P1", "MVP 每天能用", 12, "#2F5D8C"),
    ("P2", "後端與匯入", 12, "#4C7FB3"),
    ("P3", "多裝置同步", 16, "#4C7FB3"),
    ("P4", "語音", 8, "#4C7FB3"),
    ("P5", "AI 製卡・分析", 10, "#4C7FB3"),
    ("P6", "多平台・打磨", 12, "#4C7FB3"),
]
PX, L, TOP, H = 22, 40, 70, 90
total = sum(p[2] for p in phases)
W = L * 2 + total * PX
img = Image.new("RGB", (W, 230), "white")
d = ImageDraw.Draw(img)
f_title = ImageFont.truetype(BOLD, 20, index=TC)
f_lab = ImageFont.truetype(FONT, 15, index=TC)
f_small = ImageFont.truetype(FONT, 13, index=TC)

d.text((L, 18), "分階段路線圖（每週 8 小時，取估計上限共 76 週；P1 結束後依實際數據重新校準）", font=f_title, fill="#222")
x, week = L, 1
for code, name, w, color in phases:
    x2 = x + w * PX
    d.rectangle([x, TOP, x2 - 3, TOP + H], fill=color)
    fg = "white" if color != "#8FB3D9" else "#102030"
    d.text(((x + x2) / 2, TOP + 22), code, font=f_lab, fill=fg, anchor="mm")
    d.text(((x + x2) / 2, TOP + 46), name, font=f_small if w < 10 else f_lab, fill=fg, anchor="mm")
    d.text(((x + x2) / 2, TOP + 70), f"W{week}–W{week + w - 1}", font=f_small, fill=fg, anchor="mm")
    x, week = x2, week + w

# week axis
for wk in range(0, total + 1, 4):
    xx = L + wk * PX
    d.line([xx, TOP + H + 6, xx, TOP + H + 12], fill="#888")
    d.text((xx, TOP + H + 22), str(wk), font=f_small, fill="#666", anchor="mm")
d.text((W - L, TOP + H + 40), "（週）", font=f_small, fill="#666", anchor="rm")

# milestone after P1 (end of week 18)
mx = L + 18 * PX - 2
d.polygon([(mx, TOP + H + 32), (mx - 8, TOP + H + 46), (mx + 8, TOP + H + 46)], fill="#C0392B")
d.text((mx + 14, TOP + H + 40), "W18：MVP 驗收（連續 14 天使用）＋ 時程重新校準", font=f_lab, fill="#C0392B", anchor="lm")
img.save(sys.argv[1])
