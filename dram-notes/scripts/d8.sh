#!/bin/sh
# D8: DRAM shuffle before/after one system suspend (read-only register reads).
O=/tmp/d8.out
{
echo "== $(date -u) before"; python3 /tmp/dram_shu.py 1
cat /sys/power/suspend_stats/success 2>/dev/null | sed 's/^/suspend success before: /'
rtcwake -m mem -s 10
echo "== $(date -u) after resume"; python3 /tmp/dram_shu.py 3 0.5
cat /sys/power/suspend_stats/success 2>/dev/null | sed 's/^/suspend success after: /'
dmesg | grep -i -E "PM: suspend (entry|exit)|dpm|dramc|dvfsrc" | tail -6
} > $O 2>&1
