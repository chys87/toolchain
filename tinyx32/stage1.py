#!/usr/bin/env python3
# coding: utf-8


import glob
import os
import sys


CPPFLAGS = '-DTX32_PREFERRED_STACK_BOUNDARY=5 -I.'
C_CXX_FLAGS = ('-O2 -march=native -mno-vzeroupper -mx32 -ffreestanding -fbuiltin'
               ' -mpreferred-stack-boundary=5'
               ' -fno-PIE -fno-PIC -Wall -Werror -flto'
               ' -ffunction-sections -fdata-sections')
CFLAGS = C_CXX_FLAGS + ' -std=gnu11'
CXXFLAGS = C_CXX_FLAGS + ' -std=gnu++17'
LDFLAGS = '-nostdlib -static -fuse-linker-plugin -Wl,--gc-sections'
LIBS = ' -lgcc'
BUILDDIR = 'build/stage1'


NINJA_TEMPLATE = r'''
cc = {cc}
cxx = {cxx}
cppflags = {cppflags}
cflags = {cflags}
cxxflags = {cxxflags}
ldflags = {ldflags}
libs = {libs}
builddir = {builddir}
dst = $builddir/tinyx32

rule cc
  command = $cc -c -MD -MF $out.d $cppflags $cflags -o $out $in
  depfile = $out.d

rule cxx
  command = $cxx -c -MD -MF $out.d $cppflags $cxxflags -o $out $in
  depfile = $out.d

rule ld
  command = ! test -f $out || mv -f $out $out.bak; $cxx $cxxflags $ldflags -o $out $in $libs

build $dst: ld {objs}
default $dst

{rules}
'''


def find_executable(exe):
    for path in os.environ.get('PATH', '').split(os.pathsep):
        fullpath = os.path.join(path, exe)
        if os.access(fullpath, os.X_OK):
            return fullpath
    return None


def find_compilers():
    for exe_pattern in ['{}-svn', 'x86_64-linux-gnux32-{}', '{}']:
        gcc = find_executable(exe_pattern.format('gcc'))
        gxx = find_executable(exe_pattern.format('g++'))
        if gcc and gxx:
            return gcc, gxx
    raise sys.exit('Failed to find an appropriate C++ compiler')


def main():
    build_ninja = os.path.join(BUILDDIR, 'build.ninja')
    cc, cxx = find_compilers()

    lib_c_s = glob.glob('lib/*.c') + glob.glob('lib/*.S')
    lib_cc = glob.glob('lib/*.cpp')
    src_cc = glob.glob('src/*.cpp')

    objs = []
    rules = ''
    for fname in lib_c_s:
        obj = os.path.join('$builddir', fname + '.o')
        objs.append(obj)
        rules += 'build {}: cc {} | {}\n'.format(obj, fname, sys.argv[0])

    for fname in lib_cc + src_cc:
        obj = os.path.join('$builddir', fname + '.o')
        objs.append(obj)
        rules += 'build {}: cxx {} | {}\n'.format(obj, fname, sys.argv[0])

    ninja_content = NINJA_TEMPLATE.format(
        cc=cc, cxx=cxx, cppflags=CPPFLAGS, cflags=CFLAGS, cxxflags=CXXFLAGS,
        ldflags=LDFLAGS, libs=LIBS, builddir=BUILDDIR,
        objs=' '.join(objs), rules=rules)

    try:
        os.makedirs(BUILDDIR)
    except FileExistsError:
        pass
    with open(build_ninja, 'w') as f:
        f.write(ninja_content)

    os.environ['NINJA_STATUS'] = '[%u/%r/%f/%t] '
    os.execvp('ninja', ['ninja', '-v', '-f', build_ninja])


if __name__ == '__main__':
    main()
