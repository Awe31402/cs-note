#!/usr/bin/env bash
# Render every diagram source in src/ to a PNG next to this script.
# Needs: plantuml, graphviz (dot), python3 + Pillow, Noto Sans CJK fonts.
set -euo pipefail
cd "$(dirname "$0")"
plantuml -tpng -o .. src/*.puml
for f in src/*.dot; do dot -Tpng "$f" -o "$(basename "${f%.dot}").png"; done
for f in src/*.py; do python3 "$f" "$(basename "${f%.py}").png"; done
echo "rendered: $(ls *.png | wc -l) png"
