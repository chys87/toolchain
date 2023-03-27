#!/bin/bash

set -e

t1=${1:-cbu}
t2=${2:-tc}

bazel build :${t1}malloc_test :${t2}malloc_test

exedir=$PWD/../../../bazel-bin/cbu/malloc/test
PATH="$exedir:$PATH"

test -x $exedir/${t1}malloc_test
test -x $exedir/${t2}malloc_test

/usr/bin/time -v ${t1}malloc_test 2>&1 | tee /tmp/cbu.malloc.$$.1.txt
/usr/bin/time -v ${t2}malloc_test 2>&1 | tee /tmp/cbu.malloc.$$.2.txt

colordiff -y /tmp/cbu.malloc.$$.1.txt /tmp/cbu.malloc.$$.2.txt #| less -R
exec rm -f /tmp/cbu.malloc.$$.*.txt
