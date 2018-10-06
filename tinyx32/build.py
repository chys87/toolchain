#!/usr/bin/env python3
# coding: utf-8


import argparse
import glob
import os
import sys


COMPILER_PATTERNS = ['{}-svn', 'x86_64-linux-gnux32-{}', '{}']
BUILD_FILE = 'TINYX32BUILD'


NINJA_HEADER = r'''
'''


NINJA_TEMPLATE = r'''
{name}cc = {cc}
{name}cxx = {cxx}
{name}cppflags = -DTX32_PREFERRED_STACK_BOUNDARY=5 -I{tinyx32_dir}
{name}commonflags = -O2 -march=native -mx32 -mpreferred-stack-boundary=5 -ffreestanding -fbuiltin -fno-PIE -fno-PIC -Wall -flto -fdata-sections -ffunction-sections -fmerge-all-constants
{name}cflags = -std=gnu11
{name}cxxflags = -fno-exceptions -fno-rtti -std=gnu++17
{name}ldflags = -nostdlib -static -fuse-linker-plugin -Wl,--gc-sections
{name}libs = -lgcc
{name}builddir = {builddir}
{name}dst = ${name}builddir/{name}

rule {name}cc
  command = ${name}cc -c -MD -MF $out.d ${name}cppflags ${name}commonflags ${name}cflags -o $out $in
  depfile = $out.d

rule {name}cxx
  command = ${name}cxx -c -MD -MF $out.d ${name}cppflags ${name}commonflags ${name}cxxflags -o $out $in
  depfile = $out.d

rule {name}ld
  command = ! test -f $out || mv -f $out $out.bak; ${name}cxx ${name}commonflags ${name}cxxflags ${name}ldflags -o $out $in ${name}libs

build ${name}dst: {name}ld {objs}
default ${name}dst

{rules}
'''


def find_executable(exe):
    for path in os.environ.get('PATH', '').split(os.pathsep):
        fullpath = os.path.join(path, exe)
        if os.access(fullpath, os.X_OK):
            return fullpath
    return None


def find_compilers():
    for exe_pattern in COMPILER_PATTERNS:
        gcc = find_executable(exe_pattern.format('gcc'))
        gxx = find_executable(exe_pattern.format('g++'))
        if gcc and gxx:
            return gcc, gxx
    raise sys.exit('Failed to find an appropriate C++ compiler')


class Binary:
    __slots__ = 'name', 'srcs'


class BuildFile:
    __slots__ = 'binaries'

    def __init__(self, filename):
        self.binaries = []

        with open(filename) as f:
            build_content = f.read()

        exec(build_content, {
            'binary': self.__callback_binary,
        })

    def __callback_binary(self, *, name, srcs):
        binary = Binary()
        binary.name = name
        binary.srcs = srcs
        self.binaries.append(binary)

    def gen_ninja(self):
        cc, cxx = find_compilers()
        tinyx32_dir = os.path.dirname(os.path.realpath(__file__))

        ninja_content = NINJA_HEADER

        for binary in self.binaries:
            name = binary.name
            builddir = os.path.realpath(os.path.join('build', name))

            # obj: src
            objs = {}

            for src in binary.srcs:
                objs[f'${name}builddir/{src}.o'] = os.path.join('..', src)

            for suffix in ('*.c', '*.S', '*.cpp'):
                for src in glob.glob(os.path.join(tinyx32_dir, 'lib', suffix)):
                    relsrc = os.path.relpath(src, tinyx32_dir)
                    objs[f'${name}builddir/__tinyx32/{relsrc}.o'] = src

            extra_deps = f'{os.path.realpath(sys.argv[0])}'
            rules = ''
            for obj, src in sorted(objs.items()):
                if src.endswith(('.c', '.S')):
                    rules += f'build {obj}: {name}cc {src} | {extra_deps}\n'
                else:
                    rules += f'build {obj}: {name}cxx {src} | {extra_deps}\n'

            ninja_content += NINJA_TEMPLATE.format(
                tinyx32_dir=tinyx32_dir,
                name=binary.name, cc=cc, cxx=cxx, builddir=builddir,
                objs=' '.join(sorted(objs)), rules=rules)

        return ninja_content


def safe_replace(filename, content):
    if isinstance(content, str):
        content = content.encode()
    try:
        with open(filename, 'rb') as f:
            old_content = f.read()
        if old_content == content:
            return
    except FileNotFoundError:
        pass

    dirname = os.path.dirname(filename)
    if dirname:
        try:
            os.makedirs(dirname)
        except FileExistsError:
            pass
    with open(filename, 'wb') as f:
        f.write(content)


def main():
    parser = argparse.ArgumentParser(description='Build against tinyx32')
    parser.add_argument('-f', default='TINYX32BUILD')
    parser.add_argument('--print-ninja', action='store_true', default=False,
                        help='Print generated ninja build file')
    args = parser.parse_args()

    build = BuildFile(args.f)
    ninja_content = build.gen_ninja()

    if args.print_ninja:
        print(ninja_content)
        return

    os.environ['NINJA_STATUS'] = '[%u/%r/%f/%t]'
    os.chdir('build')

    ninja_filename = 'build.ninja'
    safe_replace(ninja_filename, ninja_content)

    os.execvp('ninja', ['ninja', '-v', '-f', ninja_filename])


if __name__ == '__main__':
    main()
