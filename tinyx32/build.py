#!/usr/bin/env python3
# coding: utf-8


import argparse
import glob
import os
import sys


COMPILER_PATTERNS = ['{}-svn-opt', '{}-svn', 'x86_64-linux-gnux32-{}', '{}']
BUILD_FILE = 'TINYX32BUILD'


NINJA_HEADER = r'''
pool lto_link
    depth = 1

argv0 = {argv0}
'''


NINJA_TEMPLATE = r'''
{name}cc = {cc}
{name}cxx = {cxx}
{name}cppflags = -DTX32_PREFERRED_STACK_BOUNDARY={stack_alignment} -I{tinyx32_dir} -D _GNU_SOURCE -D NDEBUG -D __STDC_LIMIT_MACROS -D __STDC_CONSTANT_MACROS -D __STDC_FORMAT_MACROS -D __NO_MATH_INLINES -U _FORTIFY_SOURCE
{name}commonflags = -O2 -march=native -mx32 -mpreferred-stack-boundary={stack_alignment} -ffreestanding -fbuiltin -fno-PIE -fno-PIC -Wall -flto -fdata-sections -ffunction-sections -fmerge-all-constants -fdiagnostics-color=always -funsigned-char -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -mno-vzeroupper
{name}cflags = -std=gnu11
{name}cxxflags = -fno-exceptions -fno-rtti -std=gnu++17 -fconcepts
{name}ldflags = -nostdlib -static -fuse-linker-plugin -flto-partition=none -Wl,--gc-sections
{name}libs = -lgcc
{name}builddir = {builddir}
{name}instdir = {instdir}

rule {name}cc
  command = ${name}cc -c -MD -MF $out.d ${name}cppflags ${name}commonflags ${name}cflags -o $out $in
  depfile = $out.d

rule {name}cxx
  command = ${name}cxx -c -MD -MF $out.d ${name}cppflags ${name}commonflags ${name}cxxflags -o $out $in
  depfile = $out.d

rule {name}ld
  command = ! test -f $out || mv -f $out $out.bak; ${name}cxx ${name}commonflags ${name}cxxflags ${name}ldflags -o $out $in ${name}libs
  pool = lto_link

rule {name}strip
  command = strip --strip-unneeded -R .comment -R .GCC.command.line -o $out $in
rule {name}install
  command = install -m755 -C -T -v $in $out
rule {name}size
  command = {tinyx32_dir}/../scripts/detailed_size ${name}instdir/{name} ${name}builddir/{name}.out
rule {name}bincmp
  command = {tinyx32_dir}/../scripts/bincmp ${name}instdir/{name} ${name}builddir/{name}.out
  pool = console
rule {name}un
  command = {tinyx32_dir}/../scripts/unassembly ${name}builddir/{name}.n.out
  pool = console
rule {name}clean
  command = rm -rfv ${name}builddir/{name}.n.out ${name}builddir/{name}.out {objs}

build ${name}builddir/{name}.n.out: {name}ld {objs}
build ${name}builddir/{name}.out: {name}strip ${name}builddir/{name}.n.out
default ${name}builddir/{name}.out

build ${name}instdir/{name}: {name}install ${name}builddir/{name}.out
build install-{name}: phony ${name}instdir/{name}
build install: phony install-{name}

build size-{name}: {name}size ${name}builddir/{name}.out
build size: phony size-{name}

build bincmp-{name}: {name}bincmp ${name}builddir/{name}.out
build bincmp: phony bincmp-{name}

build un-{name}: {name}un ${name}builddir/{name}.n.out
build un: phony un-{name}

build clean-{name}: {name}clean
build clean: phony clean-{name}

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
    __slots__ = 'name', 'srcs', 'stack_alignment'

    def build_hash(self):
        return f'{self.stack_alignment}'


class BuildFile:
    __slots__ = 'binaries'

    def __init__(self, filename):
        self.binaries = []

        try:
            with open(filename) as f:
                build_content = f.read()
        except FileNotFoundError as e:
            sys.exit(str(e))

        try:
            exec(build_content, {
                'binary': self.__callback_binary,
                '__builtin__': {},
            })
        except Exception as e:
            sys.exit(f'Failed to parse {filename}: {e}')

        if not self.binaries:
            sys.exit(f'No target is specified in {filename}')

    def __callback_binary(self, *, name, srcs, stack_alignment=4):
        binary = Binary()
        binary.name = name
        binary.srcs = srcs
        binary.stack_alignment = stack_alignment
        self.binaries.append(binary)

    def gen_ninja(self):
        cc, cxx = find_compilers()
        tinyx32_dir = os.path.dirname(os.path.realpath(__file__))
        instdir = os.path.expanduser('~/bin')

        ninja_content = NINJA_HEADER.format(
            argv0=os.path.realpath(sys.argv[0])
        )

        for binary in self.binaries:
            name = binary.name
            builddir = os.path.realpath(os.path.join('build', name))
            safe_replace(os.path.join(builddir, '__hash'), binary.build_hash())

            # obj: src
            objs = {}

            for src in binary.srcs:
                objs[f'${name}builddir/{src}.o'] = os.path.join('..', src)

            for suffix in ('*.c', '*.S', '*.cpp'):
                for src in glob.glob(os.path.join(tinyx32_dir, 'lib', suffix)):
                    relsrc = os.path.relpath(src, tinyx32_dir)
                    objs[f'${name}builddir/__tinyx32/{relsrc}.o'] = src

            extra_deps = f'$argv0 ${name}builddir/__hash'
            rules = ''
            for obj, src in sorted(objs.items()):
                if src.endswith(('.c', '.S')):
                    rules += f'build {obj}: {name}cc {src} | {extra_deps}\n'
                else:
                    rules += f'build {obj}: {name}cxx {src} | {extra_deps}\n'

            ninja_content += NINJA_TEMPLATE.format(
                tinyx32_dir=tinyx32_dir, instdir=instdir,
                name=binary.name, cc=cc, cxx=cxx, builddir=builddir,
                stack_alignment=binary.stack_alignment,
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
    parser.add_argument('-v', '--verbose', action='store_true')
    parser.add_argument('ninja_arg', nargs='*')
    args = parser.parse_args()

    build = BuildFile(args.f)
    ninja_content = build.gen_ninja()

    if args.print_ninja:
        print(ninja_content)
        return

    os.chdir('build')

    ninja_filename = 'build.ninja'
    safe_replace(ninja_filename, ninja_content)

    ninja_args = ['ninja',
                  args.verbose and '-v',
                  '-f', ninja_filename
        ] + args.ninja_arg
    ninja_args = list(filter(None, ninja_args))
    os.execvp('ninja', ninja_args)


if __name__ == '__main__':
    main()
