#!/bin/sh
# D7: sample DRAM current shuffle (read-only) while idle, under memory load, and after.
cd /tmp
echo "== date"; date -u
echo "== idle 20 x 0.5 s"; python3 /tmp/dram_shu.py 20 0.5 | awk '{print $1,$2,$4,$NF}'
echo "== load: 4 x dd memcpy 8 GiB each (bs=64M from /dev/zero to /dev/null)"
for i in 1 2 3 4; do (dd if=/dev/zero of=/dev/null bs=64M count=128 2>/dev/null &) ; done
sleep 0.5
python3 /tmp/dram_shu.py 20 0.5 | awk '{print $1,$2,$4,$NF}'
wait
echo "== load: stream-ish copy with python (256 MiB buffer x 10)"
python3 -c "
import time
b=bytearray(256<<20); t=time.monotonic()
for i in range(10): c=bytes(b)
print('copied 2.5 GiB in %.2f s'%(time.monotonic()-t))" &
sleep 0.3
python3 /tmp/dram_shu.py 10 0.3 | awk '{print $1,$2,$4,$NF}'
wait
echo "== after load 10 x 0.5 s"; python3 /tmp/dram_shu.py 10 0.5 | awk '{print $1,$2,$4,$NF}'
echo "== full lines (1 sample)"; python3 /tmp/dram_shu.py 1
