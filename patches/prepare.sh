#!/bin/bash

set -e

version="$1"

if ! [[ -r "gcc-$version.patch" ]]; then
  echo "$0 VERSION" >&2
  exit 1
fi

tmp="/tmp/gcc-$version.patch"

filterdiff -p1 -i 'libstdc++-v3/*' "gcc-$version.patch" > "$tmp"
sed -i -e 's!libstdc++-v3/include/std/!!g' "$tmp"
sed -i -e 's!libstdc++-v3/libsupc++/!bits/!g' "$tmp"
sed -i -e 's!libstdc++-v3/include/!!g' "$tmp"

echo "Run as root:"

target_dir=

for try in \
  /usr/lib/gcc/*/$version/include/g++-v$version \
  /usr/include/c++/$version
do
  if [[ -d "$try" ]]; then
    echo "cd $try && patch -N -p1 < $tmp"
  fi
done
