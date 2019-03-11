#! /usr/bin/env python
# -*- coding: utf-8 -*-

#
# Copyright (c) 2014, 2015, chys <admin@CHYS.INFO>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
#   Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
#   Neither the name of chys <admin@CHYS.INFO> nor the names of other
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

from __future__ import print_function


import argparse
import os
import sys


def _check_python_version(args):
    with open(args.filename) as f:
        line = f.readline()

    # No she-bang.  Consider version OK.
    if not line.startswith('#!'):
        return True, None

    shebang = line[2:].split()

    if not shebang or not shebang[0].startswith('/'):
        return True, None

    interpreter = shebang[0]
    if interpreter == '/usr/bin/env' and len(shebang) > 1:
        interpreter = shebang[1]

    if not os.path.isabs(interpreter):
        from distutils.spawn import find_executable
        full_interpreter = find_executable(interpreter) or interpreter
    else:
        full_interpreter = interpreter

    if full_interpreter == sys.executable or interpreter == sys.argv[0] \
            or full_interpreter == sys.argv[0]:
        return True, None

    return False, shebang + [__file__] + sys.argv[1:]


def _print_dependency(args):
    import modulefinder

    path = sys.path[:]
    path[0] = os.path.dirname(args.filename)

    finder = modulefinder.ModuleFinder(path=path)
    finder.load_file(args.filename)

    files = []
    for mod in finder.modules.values():
        if mod.__file__:
            files.append(mod.__file__)

    files = sorted(set(files))
    print(args.target, ': \\')

    for path in files:
        # Kill "setuptools/script template.py"
        if ' ' not in path:
            print('\t', path, ' \\')
    print()

    if args.dummy:
        for path in files:
            if ' ' not in path:
                print(path, ':')


def main():
    parser = argparse.ArgumentParser(
        description="Build Makefile-style dependency for Python")
    parser.add_argument('-t', '--target', help='Target name', required=True)
    parser.add_argument('--no-dummy', help='Don\'t add dummy rules',
                        dest='dummy', default=True, action='store_false')
    parser.add_argument('--no-shebang',
                        help='Don\'t examine the she-bang line. '
                             'Instead, assume we\'re running the right '
                             'interpreter.',
                        action='store_true', default=False)
    parser.add_argument('filename', help='Python filename')
    args = parser.parse_args()

    if not args.no_shebang:
        version_ok, cmdline = _check_python_version(args)
        if not version_ok:
            cmdline.append('--no-shebang')
            import subprocess
            sys.exit(subprocess.call(cmdline))

    _print_dependency(args)


if __name__ == '__main__':
    main()
