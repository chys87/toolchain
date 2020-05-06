#! /usr/bin/python3
# -*- coding: utf-8 -*-

import re
from . import utils
from . import x86

"""
This module does several things:
* Replaces contiguous data with a single placeholder to reduce workload of code optimizers
* "Canonicalize" directives/instructions to reduce workload of optimizers
"""

__all__ = ['apply']

# "common" implies a label and cannot be included
_data_directives = b'byte|value|long|quad|zero|string|ascii'

# Constant data/string only. No labels.
_pattern_blocks_pattern = br'^(\s*\.(' + _data_directives + br')\s+(-?\d+|-?0x[\da-fA-F]+|"[^\n]*")\n)+'
def _sub_canonicalize_align(match):
    n = int(match.group(1))
    if n == 16:
        k = 4
    elif n == 8:
        k = 3
    else:
        assert (n != 0)
        assert (n & (n - 1)) == 0
        k = 0
        n -= 1
        while n != 0:
            n >>= 1
            k += 1
    return '\t.p2align {}'.format(k).encode()

def _sub_canonicalize_cc(match):
    j, cc = match.group(1, 2)
    return b'\t' + j + x86.cc_canonicalize[cc]

_canonicalizations = (
        # Remove trailing spaces
        (br'[ \t]+$', b''),
        # If any space exist at the beginning of a line, replace it with a single tab.
        # (Hereinafter, we will assume any instruction is preceded by a single tab, in order to accelerate matching.)
        (br'^[ \t]+', b'\t'),
        # Remove comments and empty lines
        (br'^((\t?#.*)?\n)+', b''),

        # Keep one single tab after all instructions
        (br'^(\t\w+)[ \t]+', br'\1\t'),

        # Canonicalize all ".align"/".balign" to ".p2align"
        # But don't do this if we appear to be in a data section.
        (br'^\t\.b?align[ \t]+(\d+)$(?!\n(?:\t\.(?:size|type|b?align|p2align)[ \t].*\n|[.\w]+:\n)*\t\.(?:COMPRESSED|byte|string|ascii|value|long|quad|zero))',
            _sub_canonicalize_align),
        # Keep one single space after ".p2align" (to align with GCC)
        (br'^\t\.p2align[ \t]+', b'\t.p2align '),
        # Canonicalize ".p2align 4,,15" to ".p2align 4"
        (br'^\t\.p2align 4,,15$', b'\t.p2align 4'),

        # Canonicalize repe/repne to repz/repnz
        (br'^\t(repn?)e[;\s]', br'\t\1z\t'),

        # Remove ';' after rep*/lock
        (br'^\t(rep(n?z)?|lock)[;\n\t ]+', br'\t\1\t'),

        # "Canonicalizations" of instruction aliases
        # "Canonicalize" branches (jc/jnae => jb; etc.)
        (br'^\t(j|cmov|set)(' + b'|'.join(x86.cc_canonicalize) + br')(?=\t)', _sub_canonicalize_cc),
        # Replace "sal" with "shl"
        (br'^\tsal(?=[bwlq]?\t)', br'\tshl'),
    )

class Preprocessor:
    def __init__(self, data):
        self._data = data

    _back_canonicalizations = (  # Revert some instructions to GCC's preferred form to reduce diff size
        (br'^\t(rep|lock)\t', br'\t\1 '),
        (br'^\tshl([bwlq]?)\t', br'\tsal\1\t'),
    )

    def restore(self, contents):
        contents = utils.apply_re(contents, self._back_canonicalizations)
        parts = contents.split(b'\t.COMPRESSED\n')
        assert len(parts) == len(self._data) + 1
        for (i, data) in enumerate(self._data):
            parts[i] += data
        contents = b''.join(parts)
        return contents


def apply(contents):
    data = []
    def sub(match):
        data.append(match.group(0))
        return b'\t.COMPRESSED\n'
    contents = re.sub(_pattern_blocks_pattern, sub, contents, flags=re.M)
    # Canonicalizations
    contents = utils.apply_re(contents, _canonicalizations)
    return contents, Preprocessor(data)
