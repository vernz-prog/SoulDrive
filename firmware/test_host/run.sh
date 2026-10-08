#!/bin/sh
set -e
cd "$(dirname "$0")"
c++ -std=c++17 -O2 -Wall -Wno-unused-function -Wno-for-loop-analysis -Imock -I../src sim.cpp -o sim
rm -f prefs.txt
SIM_PREFS=prefs.txt ./sim all | tee sim.log
ODO=$(grep '^odo ' sim.log | cut -d' ' -f2)
echo
echo "14. Выключение и повторное включение питания"
SIM_PREFS=prefs.txt ./sim resume > resume.txt
python3 - "$ODO" <<'PY'
import sys
odo_before = float(sys.argv[1])
odo_after, trip_after = map(float, open('resume.txt').read().split())
lost = odo_before - odo_after
ok = 0 <= lost <= 0.1
print(f"  {'OK  ' if ok else 'FAIL'}  пробег сохранился (потеря не больше 100 м){'':17} до {odo_before:.3f} км, после {odo_after:.3f} км")
sys.exit(0 if ok else 1)
PY
