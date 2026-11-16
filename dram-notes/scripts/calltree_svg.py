#!/usr/bin/env python3
"""Draw diagrams/init-03-call-tree.svg: the mt_mem_init() call tree in dram-init.md §3.

Run from dram-notes/:  python3 scripts/calltree_svg.py
Then render the PNG:    inkscape diagrams/init-03-call-tree.svg --export-type=png \
                          --export-filename=diagrams/init-03-call-tree.png --export-dpi=144
"""
from html import escape

# (depth, function lines, where, note lines, kind)
# kind: "both" = LK and BL2, "bl2" = only BL2, "closed" = no source, "fail" = error path
TREE = [
    (0, ["mt_mem_init()"], "memory.c:790（BL2）／:854（LK）", [], "both"),
    (1, ["exception_detection_start()"], ":796", [], "bl2"),
    (1, ['print "libdram version"'], "", [], "both"),
    (1, ["mt_base_probe()、mt_base_init()"], "libbase.a", ["沒有原始碼，印 \"base init: MT8391 platform\""], "closed"),
    (1, ["normal_mem_init()"], ":167", [], "both"),
    (2, ["check_pl_build_time_updated()"], ":177", ["比對 dramk header 的 build time"], "bl2"),
    (2, ["mt_set_emi()"], "dramc_top.c:2236", [], "both"),
    (3, ["setup_dramc_voltage_by_pmic()"], ":2273", ["設 Vcore、VMDDR、VDDQ、VDD2H、VIO18"], "both"),
    (3, ["dram_auto_detection()"], ":2117", [], "both"),
    (4, ["Init_DRAM(..., GET_MDL_USED)"], "", ["讀 MR5～MR8：型號、rank、密度",
                                               "→ update_dram_setting／decode_emi_info／update_emi_setting"], "both"),
    (3, ["Init_DRAM(..., NORMAL_USED)"], ":2312", ["真正的初始化＋校正"], "both"),
    (4, ["Init_DramcCalibrationAllFreq()"], "dramc_pi_main.c:2358", [], "both"),
    (3, ["校正失敗 → clean_dram_calibration_data()", "→ reset"], ":2314-2325", [], "fail"),
    (3, ["switch_dramc_voltage_to_auto_mode()", "restore_vcore_vddq_setting()", "check_dramc_voltage_after_k()"],
     "", [], "both"),
    (1, ["mem_test()"], ":839", ["complex_mem_test 0x2000 bytes"], "bl2"),
    (1, ["exception_detection_end()"], "", [], "bl2"),
    (1, ["emi_init3()"], ":844", [], "both"),
    (1, ['print "DDR frequency"、"Memory size"'], "", [], "both"),
    (1, ["aoc_init()"], ":850", [], "bl2"),
    (1, ["reserve_dramc_dummy_read()"], ":851", ["mblock 保留 dramc-rk0／rk1"], "bl2"),
]

STYLE = {  # fill, border
    "both": ("#E8F0FE", "#4A6FA5"),
    "bl2": ("#FEF7E0", "#B08900"),
    "closed": ("#EEEEEE", "#888888"),
    "fail": ("#FCE8E6", "#D93025"),
}
FONT = 'font-family="Noto Sans CJK TC, sans-serif"'
MONO = 'font-family="DejaVu Sans Mono, Noto Sans Mono CJK TC, monospace"'
X0, INDENT, NOTE_X, LINE, PAD, GAP = 24, 34, 560, 17, 8, 8
W = 1180


def text_width(s, size=12.5):
    return sum(size if ord(c) > 0x2e7f else size * 0.61 for c in s)


def main():
    rows, y = [], 60
    for depth, funcs, where, notes, kind in TREE:
        n = max(len(funcs), len(notes) + (1 if where else 0), 1)
        h = n * LINE + 2 * PAD
        rows.append((depth, funcs, where, notes, kind, y, h))
        y += h + GAP
    H = y + 70

    out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" {FONT} font-size="12.5">',
           f'<rect width="{W}" height="{H}" fill="white"/>',
           f'<text x="{X0}" y="30" font-size="16" font-weight="bold">libdram 的 mt_mem_init() 一路往下（BL2 版 memory.c:790-852，LK 版 :854-889）</text>']

    # Elbow connectors: each node hangs off the nearest shallower row above it.
    for i, (depth, *_rest) in enumerate(rows):
        if depth == 0:
            continue
        y_mid = rows[i][5] + rows[i][6] / 2
        p = next(j for j in range(i - 1, -1, -1) if rows[j][0] == depth - 1)
        px = X0 + (depth - 1) * INDENT + 14
        py = rows[p][5] + rows[p][6]
        cx = X0 + depth * INDENT
        out.append(f'<polyline points="{px},{py} {px},{y_mid} {cx},{y_mid}" fill="none" stroke="#999" stroke-width="1.3"/>')

    for depth, funcs, where, notes, kind, y, h in rows:
        fill, border = STYLE[kind]
        x = X0 + depth * INDENT
        w = max(text_width(f) for f in funcs) + 2 * PAD + 4
        out.append(f'<rect x="{x}" y="{y}" width="{w:.0f}" height="{h}" rx="5" fill="{fill}" stroke="{border}" stroke-width="1.4"/>')
        fy = y + PAD + LINE * 0.75 + (h - 2 * PAD - len(funcs) * LINE) / 2
        for k, f in enumerate(funcs):
            out.append(f'<text x="{x + PAD}" y="{fy + k * LINE:.1f}" {MONO} font-size="12.5">{escape(f)}</text>')
        right = ([("where", where)] if where else []) + [("note", t) for t in notes]
        ny = y + PAD + LINE * 0.75 + (h - 2 * PAD - len(right) * LINE) / 2
        if right:
            out.append(f'<line x1="{x + w + 6:.0f}" y1="{y + h / 2}" x2="{NOTE_X - 8}" y2="{y + h / 2}" stroke="#ddd" stroke-dasharray="3 3"/>')
        for k, (what, t) in enumerate(right):
            style = f'{MONO} fill="#4A6FA5"' if what == "where" else 'fill="#333"'
            out.append(f'<text x="{NOTE_X}" y="{ny + k * LINE:.1f}" {style}>{escape(t)}</text>')

    lx, ly = X0, H - 34
    for kind, label in (("both", "LK 和 BL2 都跑"), ("bl2", "只有 BL2"), ("closed", "沒有原始碼（libbase.a）"), ("fail", "失敗路徑")):
        fill, border = STYLE[kind]
        out.append(f'<rect x="{lx}" y="{ly}" width="22" height="16" rx="3" fill="{fill}" stroke="{border}" stroke-width="1.4"/>')
        out.append(f'<text x="{lx + 30}" y="{ly + 12.5}">{label}</text>')
        lx += 60 + text_width(label)
    out.append(f'<text x="{W - 24}" y="{ly + 12.5}" text-anchor="end" fill="#777">右欄：行號（沒寫檔名的跟上一層同一個檔案）＋說明</text>')
    out.append('</svg>')
    with open("diagrams/init-03-call-tree.svg", "w", encoding="utf-8") as fh:
        fh.write("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
