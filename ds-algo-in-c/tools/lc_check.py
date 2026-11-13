#!/usr/bin/env python3
"""核對 LeetCode 題號：印出題名、難度、是否付費、連結。

用法：python3 tools/lc_check.py 215 703 347
題目清單第一次會下載到 build/lc_all.json，之後直接讀快取。
"""
import json
import os
import sys
import urllib.request

URL = "https://leetcode.com/api/problems/all/"
CACHE = os.path.join(os.path.dirname(__file__), "..", "build", "lc_all.json")
LEVEL = {1: "Easy", 2: "Medium", 3: "Hard"}


def load():
    if not os.path.exists(CACHE):
        os.makedirs(os.path.dirname(CACHE), exist_ok=True)
        req = urllib.request.Request(URL, headers={"User-Agent": "Mozilla/5.0"})
        with urllib.request.urlopen(req) as r, open(CACHE, "wb") as f:
            f.write(r.read())
    with open(CACHE) as f:
        return json.load(f)


def main():
    by_id = {}
    for p in load()["stat_status_pairs"]:
        s = p["stat"]
        by_id[str(s["frontend_question_id"])] = (
            s["question__title"],
            s["question__title_slug"],
            LEVEL[p["difficulty"]["level"]],
            p["paid_only"],
        )
    for q in sys.argv[1:]:
        r = by_id.get(q)
        if r is None:
            print(f"{q} NOT FOUND")
            continue
        title, slug, level, paid = r
        tag = "PAID" if paid else "free"
        print(f"| {q} | [{title}](https://leetcode.com/problems/{slug}/) | {level} | {tag}")


if __name__ == "__main__":
    main()
