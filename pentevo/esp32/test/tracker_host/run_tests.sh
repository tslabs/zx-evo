#!/bin/bash
# run_tests.sh [src dir] [module dir]: the tracker loaders with ASan + UBSan (seconds)
#  - the generated modules of gen_bad.py on both load paths (the stream path also in
#    chunks of 1, 2 and 3 bytes): no finding, no hang, each damaged one plays, the
#    ones which only bound a value sound like base.xm
#  - the modules of [module dir] (XM, MOD, S3M), if given: no finding, no hang, and
#    the XM ones sound the same on both load paths
H=$(cd "$(dirname "$0")" && pwd); cd "$H"
SRC=${1:-$H/../../src/main}
./build.sh "$SRC" san -fsanitize=address,undefined -fno-omit-frame-pointer || exit 1
mkdir -p build/bad && python3 gen_bad.py build/bad > /dev/null || exit 1
err=0
./run_corpus.sh san 3 build/bad > build/bad.txt
for c in 1 2 3; do CHUNK=$c ./run_corpus.sh san 3 build/bad | grep ' str: ' > build/bad_c$c.txt
  diff <(grep ' str: ' build/bad.txt) build/bad_c$c.txt > /dev/null || { echo "FAIL stream in chunks of 1..$c:"; diff <(grep ' str: ' build/bad.txt) build/bad_c$c.txt; err=1; }
done
if grep -E 'runtime error|Sanitizer|TIMEOUT' build/bad.txt; then err=1; fi
h() { grep "^$1.xm $2: " build/bad.txt | grep -oE 'hash=[0-9a-f]+|rc=-?[0-9]+'; }
base=$(h base obj)
for c in odd16 len256 len300 loopbig; do
  for p in obj str; do [ "$(h $c $p)" = "$base" ] || { echo "FAIL $c $p: $(h $c $p), base $base"; err=1; }; done
done
for c in align env20 envloop sustain loop0 loop0p rows300; do
  [ "$(h $c obj)" = "$(h $c str)" ] && [ -n "$(h $c obj | grep hash)" ] || { echo "FAIL $c: $(h $c obj) / $(h $c str)"; err=1; }
done
if [ -n "$2" ]; then
  ./run_corpus.sh san 5 "$2" > build/corpus.txt
  if grep -E 'runtime error|Sanitizer|TIMEOUT' build/corpus.txt; then err=1; fi
  python3 - build/corpus.txt <<'PY' || err=1
import sys
d = {}
for l in open(sys.argv[1]):
    n, k, *v = l.split(); d.setdefault(n, {})[k] = ' '.join(v)
bad = [n for n, x in d.items() if 'str:' in x and x['obj:'] != x['str:']]
print(len(d), 'modules;', 'XM sound differs between the load paths:', bad or 'none')
sys.exit(1 if bad else 0)
PY
fi
[ $err = 0 ] && echo "tracker_host: 0 errors" || echo "tracker_host: ERRORS"
exit $err
