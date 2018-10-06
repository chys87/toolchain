#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:ts=8 sts=4 sw=4 expandtab ft=python

#
# Copyright (c) 2013, chys <admin@CHYS.INFO>
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

__all__ = ['detailed_size']

import re
import sys
import subprocess
import os

_pattern = re.compile(r'\] *([^ ]*) +(\w+) +[0-9a-f]+ +[0-9a-f]+' +
                      r' +([0-9a-f]+) +[^ ]+ +([A-Z]*)')


_col_names = ['text', 'ehframe', 'data', 'rodata', 'bss',
              'others', 'total', 'file']

I_TEXT = 0
I_EHFRAME = 1
I_DATA = 2
I_RODATA = 3
I_BSS = 4
I_OTHERS = 5
I_TOTAL = 6

I_FILE = 7

I_COUNT = 8

assert(len(_col_names) == I_COUNT)


def detailed_size(file):
    data = [0] * I_COUNT
    readelf = subprocess.check_output(['readelf', '-S', file])
    if not isinstance(readelf, str):  # Convert to Unicode string (for Python3)
        readelf = readelf.decode('ascii')
    readelf = readelf.replace('\n', ' ')
    for line in _pattern.findall(readelf):
        Name, Type, Size, Flag = line
        Size = int(Size, 16)
        if Flag == 'AX':
            data[I_TEXT] += Size
        elif Flag == 'WA' and Type in ('PROGBITS', 'INIT_ARRAY',
                                       'FINI_ARRAY', 'PREINIT_ARRAY'):
            data[I_DATA] += Size
        elif Flag == 'WA' and Type == 'NOBITS':
            data[I_BSS] += Size
        elif (Name.startswith('.eh_frame') or
              Name.startswith('.gcc_except_table')):  # .eh_frame etc.
            data[I_EHFRAME] += Size
        elif Flag == 'A' and Type == 'PROGBITS':
            data[I_RODATA] += Size
        elif Name.startswith('.rodata'):  # .rodata.str1 etc.
            data[I_RODATA] += Size
        else:
            data[I_OTHERS] += Size
    data[I_TOTAL] = sum(data[:I_TOTAL])
    data[I_FILE] = os.stat(file).st_size
    return data


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(0)
    print(''.join('{:>10}'.format(col) for col in _col_names) +
          '    filename')
    error = 0
    for filename in sys.argv[1:]:
        try:
            data = detailed_size(filename)
            print(''.join('{:10}'.format(v) for v in data) +
                  '    ' + filename)
        except subprocess.CalledProcessError:
            # readelf already prints error info
            error = 1
    sys.exit(error)


if __name__ == '__main__':  # If run as an independent script
    main()
