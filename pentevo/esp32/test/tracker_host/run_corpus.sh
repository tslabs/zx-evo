#!/bin/bash
# run_corpus.sh <bin> <seconds> <dir>: every module of <dir> through build/<bin>, the XM ones
# also through build/<bin>_s (chunks of 1..${CHUNK:-3000} bytes); one line per module and path:
# hash of the sound or rc / findings
H=$(cd "$(dirname "$0")" && pwd)
pat='rc=-?[0-9]+( hash=[0-9a-f]+)?|(lib|stream)\.cpp:[0-9]+:[0-9]+: runtime error: [a-z ]+|(Address|Leak)Sanitizer: [A-Za-z -]+'
for f in "$3"/*; do
  r=$(timeout 20 "$H/build/$1" "$f" $2 2>&1); t=$?
  echo "$(basename "$f") obj: $(echo "$r" | grep -oE "$pat" | sed -E 's/rc=[0-9]+ hash/hash/' | sort -u | tr '\n' ' ')$([ $t -eq 124 ] && echo TIMEOUT)"
  case "$f" in *.xm|*.XM)
    r=$(timeout 20 "$H/build/${1}_s" "$f" $2 7 ${CHUNK:-3000} 2>&1); t=$?
    echo "$(basename "$f") str: $(echo "$r" | grep -oE "$pat" | sed -E 's/rc=0 hash/hash/' | sort -u | tr '\n' ' ')$([ $t -eq 124 ] && echo TIMEOUT)";;
  esac
done
