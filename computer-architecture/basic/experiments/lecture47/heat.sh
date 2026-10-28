#!/bin/bash
# heat.sh N : 讓 N 個 hardware thread 空轉 15 秒，記錄 CPU 封裝溫度和 P-core／E-core 的平均時脈
N=${1:-20}
Z=$(grep -l x86_pkg_temp /sys/class/thermal/thermal_zone*/type | head -1 | xargs dirname)
f() { cat /sys/devices/system/cpu/cpu$1/cpufreq/scaling_cur_freq; }
sample() {
	p=0; e=0
	for c in 0 2 4 6 8 10; do p=$((p + $(f $c))); done
	for c in 12 13 14 15 16 17 18 19; do e=$((e + $(f $c))); done
	echo "$1 溫度 $(( $(cat $Z/temp) / 1000 ))°C  cpu0 $(( $(f 0) / 1000 )) MHz  P-core 平均 $((p / 6 / 1000)) MHz  E-core 平均 $((e / 8 / 1000)) MHz"
}
echo "cpu0 最高 $(( $(cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq) / 1000 )) MHz，cpu12 最高 $(( $(cat /sys/devices/system/cpu/cpu12/cpufreq/cpuinfo_max_freq) / 1000 )) MHz"
sample "閒置      "
pids=""
for i in $(seq 0 $((N - 1))); do taskset -c $i bash -c 'while :; do :; done' & pids="$pids $!"; done
for t in 1 3 5 10 15; do sleep $((t - ${last:-0})); last=$t; sample "滿載 ${t}s "; done
kill $pids; wait 2>/dev/null
sleep 5; sample "停止後 5s"
