# -*- coding: utf-8 -*-

# This module tracks the use of flags

import itertools
import re
import string

from . import x86


def P(*args, **kwargs):
    return (''.join(tpl) for tpl in itertools.product(*args, **kwargs))

def _bwlq(*ins):
    return P(ins, ('', 'b', 'w', 'l', 'q'))

def _simd_float(*ins):
    return P(('', 'v'), ins, ('ps', 'pd', 'ss', 'sd'))

def _simd_floatp(*ins):
    return P(('', 'v'), ins, ('ps', 'pd'))

def _simd_int(*ins):
    return P(('p', 'vp'), ins, 'bwdq')

def _avx(*ins):
    return P(('', 'v'), ins)

def _yieldbytes(f):
    def foo(*args, **kwargs):
        return (s.encode('ascii') for s in f(*args, **kwargs))
    return foo

PB = _yieldbytes(P)

@_yieldbytes
def _yield_notuse_preserve_instructions():
    #br'', # Empty
    yield '.p2align' # Alignments (implicit NOPs)
    yield '.loc' # LOC
    yield from ('bswap', 'bswapl', 'bswapq')
    yield 'cpuid'
    yield from ('lea', 'leal', 'leaq')
    yield from ('leave', 'leavel', 'leaveq')
    yield from ('lfence', 'mfence', 'sfence')  # Fences

    # MOV
    yield from _bwlq('mov')
    yield from ('movabs', 'movabsq', 'movbe',)
    yield from ('movzx', 'movsx',)
    yield from ('movzbw', 'movzbl', 'movzwl',)
    yield from ('movsbw', 'movsbl', 'movsbq', 'movswl', 'movswq', 'movslq',)

    yield from _bwlq('nop')  # Explicit NOPs
    yield from P(('push', 'pop'), ('', 'l', 'q'))
    yield from _bwlq('xchg')
    yield 'pause'

    # String instructions
    yield from _bwlq('movs', 'stos')

    yield from _bwlq('crc32')
    yield from ('rdtsc', 'rdtscp')
    yield from ('mulx', 'salx', 'sarx', 'shlx', 'shrx', 'rorx') # BMI2: mulx/sarx/shlx/shrx/rorx
    yield from ('pdep', 'pext')  # BMI2: pdep/pext
    yield from _bwlq('not')
    yield from ('prefetch', 'prefetchnta', 'prefetcht0', 'prefetcht1', 'prefetcht2')
    yield 'xgetbv'

    # Most SIMD instructions
    yield from _simd_float('abs', 'add', 'sub', 'max', 'min', 'mul', 'div', 'rcp', 'sqrt', 'round')
    yield from _simd_floatp('and', 'andn', 'or', 'xor', 'hadd', 'hsub', 'dp')
    yield from _avx('movss', 'movsd', 'movaps', 'movapd', 'movups', 'movupd')
    yield from _avx('movdqa', 'movdqu', 'movd', 'movq')
    yield from _avx('movsldup', 'movshdup', 'movddup')
    yield from _avx('movlps', 'movlpd', 'movhps', 'movhpd')
    yield from _avx('movlhps', 'movhlps')
    yield from _simd_int('insr', 'extr')
    yield from _simd_int('abs', 'add', 'sub', 'maxs', 'mins', 'maxu', 'minu')
    yield from _simd_int('cmpgt', 'cmpeq', 'blend', 'blendv')
    yield from _avx('blendvps', 'blendvpd', 'blendps', 'blendpd')
    yield from P(('', 'v'), ('phadd', 'phsub', 'phsubs'), ('d', 'w'))
    yield from _simd_int('sll', 'srl', 'sra') # shift
    yield from _avx('pmaddwd', 'pmaddubsw')
    yield from _avx('pmulld', 'palignr')
    yield from P(('', 'v'), ('pmovsx', 'pmovzx'), 'bw|bd|bq|wd|wq|dq'.split('|'))
    yield from _simd_floatp('movmsk') # movmskps/movmskpd
    yield from _avx('pmovmskb')
    yield from _avx('pxor', 'por', 'pand', 'pandn')
    yield from P(('', 'v'), ('unpcklp', 'unpckhp'), 'sd')
    yield from P(('', 'v'), ('punpckl', 'punpckh'), 'bw|wd|dq|qdq'.split('|'))
    yield from P(('', 'v'), ('packus', 'packss'), ('wb', 'dw'))
    yield from _avx('lddqu')
    yield from P(('', 'v'), ('insertps', 'extractps'))
    yield from _simd_floatp('shuf') # shufps/shufpd
    yield from _avx('pshufb', 'pshufd')
    yield from P(('', 'v'), ('cvt', 'cvtt'), ('ps2dq', 'pd2dq')) # cvtps2dq/cvtpd2dq/cvttps2dq/cvttpd2dq
    yield from _avx('cvtdq2pd', 'cvtdq2ps')
    yield from P(('', 'v'), ('cvtsi2ss', 'cvtsi2sd'), ('', 'l', 'q'))
    yield from P(('', 'v'), ('cvt', 'cvtt'), ('sd2si', 'ss2si'), ('', 'l', 'q')) # cvttss2si/cvttsd2si/cvtss2si/cvtsd2si
    yield from P(('', 'v'), ('cvtpd2ps', 'cvtps2pd', 'cvtss2sd', 'cvtsd2ss'), ('', 'x', 'y', 'z')) # cvtpd2ps/cvtps2pd/cvtss2sd/cvtsd2ss (may have suffix x/y/z)
    yield from P(('vbroadcast',), ('i128', 'f128', 'ss', 'sd'))
    yield from P(('vpbroadcast',), 'bwdq')
    yield from ('vzeroupper', 'vzeroall')
    yield from P(('vextract', 'vinsert'), ('i128', 'f128')) # vinsertf128/vextractf128/vinserti128/vextracti128
    yield from P(('vperm',), ('2f128', 'ilps', 'ilpd')) # vperm2f128/vpermilps/vmpermilpd
    yield from P(('vfm', 'vfnm'), ('add', 'sub'), ('', '132', '213', '231'), 'ps', 'sd') # FMA4/FMA3 instructions


@_yieldbytes
def _yield_use_preserve_instructions():
    yield 'pushf'
    yield from P(('cmov', 'set'), x86.cc_set_str) # cmov/setcc
    # jcc cannot be put here. It can cause uncertain results

@_yieldbytes
def _yield_notuse_set_instructions():
    # Fundamental arithmetics
    yield from _bwlq('add', 'sub', 'mul', 'imul', 'div', 'idiv', 'cmp', 'test', 'and', 'andn', 'neg', 'or', 'xor', 'bsf',
        'bsr', 'bextr', 'tzcnt', 'lzcnt', 'blsr', 'blsi', 'blsmsk', 'bzhi', 'inc', 'dec', 'shl', 'sal', 'shr', 'sar', 'rol', 'ror')
    yield 'popf'
    yield from _avx('ptest')
    yield from ('vtestps', 'vtestpd')
    yield from P(('', 'v'), ('pcmpistr', 'pcmpestr'), 'im') # pcmp[ie]str[im]
    yield from P(('', 'v'), ('', 'u'), ('comiss', 'comisd')) # comiss, comisd, ucomiss, ucomisd
    yield from _bwlq('bt', 'btc', 'btr', 'bts') # bt, btc, btr, bts
    yield from P(('popcnt',), ('', 'w', 'l', 'q')) # popcnt affects flags
    yield 'syscall' # Theoretically it could use flags, but Linux does not do so.
    yield from _bwlq('cmpxchg')
    yield from ('cmpxchg8b', 'cmpxchg16b')

@_yieldbytes
def _yield_use_set_instructions():
    yield from _bwlq('adc', 'sbb')

_pattern_alias_multi = re.compile (br'^\t\.set\t([.\w]*),([.\w]*)$', re.M | re.A)

LABEL_CHARS = frozenset(ord(x) for x in string.ascii_letters + string.digits + '_.')
def _is_label_line(line, LABEL_CHARS_SUPERSET=LABEL_CHARS.issuperset):
    return line[-1:] == b':' and LABEL_CHARS_SUPERSET(line[:-1])

TYPE_LABEL           = 1
TYPE_RET             = 2
TYPE_CALL            = 3
TYPE_JMP             = 4
TYPE_JCC             = 5
TYPE_USE_PRESERVE    = 6
TYPE_NOTUSE_PRESERVE = 7
TYPE_USE_SET         = 8
TYPE_NOTUSE_SET      = 9
TYPE_HALT            = 10
TYPE_UNKNOWN         = 99

INSTRUCTION_TYPE_MAP = {
        ins: type
        for (gen, type) in
        (
            (_yield_use_preserve_instructions(), TYPE_USE_PRESERVE),
            (_yield_notuse_preserve_instructions(), TYPE_NOTUSE_PRESERVE),
            (_yield_use_set_instructions(), TYPE_USE_SET),
            (_yield_notuse_set_instructions(), TYPE_NOTUSE_SET),
            ((b'call', b'calll', b'callq'), TYPE_CALL),
            ((b'jmp', b'jmpl', b'jmpq'), TYPE_JMP),
            (PB('j', x86.cc_set_str), TYPE_JCC),
            ((b'hlt', b'ud2', b'ud2a'), TYPE_HALT),
            ((b'ret', b'retl', b'retq'), TYPE_RET),
        )
        for ins in gen
    }


class LineByLine:
    def __init__(self, contents):
        self._lines = contents.splitlines()
        # Construct a dict from each label to its corresponding line number
        is_label_line = _is_label_line
        self._labels = labels = { line[:-1]:i for (i, line) in enumerate(self._lines) if is_label_line(line) }
        # Add aliases
        for match in _pattern_alias_multi.finditer(contents):
            newname, oldname = match.groups()
            if oldname in labels and newname not in labels:
                labels[newname] = labels[oldname]

        self._cache = [0] * len(self._lines)

    def __iter__(self):
        return iter(self._lines)

    def __getitem__(self, i):
        return self._lines[i]

    def __setitem__(self, i, newline):
        self._lines[i] = newline
        self._cache[i] = 0

    def join(self):
        return b''.join(l + b'\n' for l in self._lines if l)

    def _line_type_nocache(self, line
            # Performance hack
            ,TYPE_UNKNOWN=TYPE_UNKNOWN
            ,TYPE_LABEL=TYPE_LABEL
            ,TYPE_NOTUSE_PRESERVE=TYPE_NOTUSE_PRESERVE
            ,TYPE_USE_PRESERVE=TYPE_USE_PRESERVE
            ,TYPE_NOTUSE_SET=TYPE_NOTUSE_SET
            ,TYPE_USE_SET=TYPE_USE_SET
            ,INSTRUCTION_TYPE_MAP=INSTRUCTION_TYPE_MAP
            ,len=len
            ,_is_label_line=_is_label_line
            ):
        if b';' in line:  # If we have ';', decline to proceed
            return TYPE_UNKNOWN

        if _is_label_line(line):
            return TYPE_LABEL

        # Get the main instruction
        tokens = line.split(maxsplit=1)
        if not tokens:  # Empty line
            return TYPE_NOTUSE_PRESERVE
        key = tokens[0]
        if key in (b'lock', b'rep', b'repz', b'repnz'):
            if len(tokens) < 2:
                if key in (b'lock', b'rep'):
                    return TYPE_NOTUSE_PRESERVE
                else:
                    return TYPE_UNKNOWN
            else:
                ret = self._line_type_nocache(tokens[1])
                if key in (b'repz', b'repnz'):
                    if ret == TYPE_NOTUSE_PRESERVE:
                        ret = TYPE_USE_PRESERVE
                    elif ret == TYPE_NOTUSE_SET:
                        ret = TYPE_USE_SET
                return ret

        if key[:5] == b'.cfi_':
            return TYPE_NOTUSE_PRESERVE

        return INSTRUCTION_TYPE_MAP.get(key, TYPE_UNKNOWN)

    def line_type(self, i):
        ret = self._cache[i]
        if ret == 0:
            ret = self._line_type_nocache(self._lines[i])
            self._cache[i] = ret
        return ret

    def get_flag_users(self, i):
        """
        Returns a list of *possible* users of flags generated at line i.
        (Line i must *not* be a branch or jump.)
        None means we are not sure (must skip the fix in the caller)
        """
        s = []
        def callback(j, s_append=s.append):
            s_append(j)
            return True
        if not self.get_flag_users_cb(i, callback):
            s = None
        return s

    def get_flag_users_cb(self, i, callback
            ,TYPE_RET=TYPE_RET
            ,TYPE_CALL=TYPE_CALL
            ,TYPE_HALT=TYPE_HALT
            ,TYPE_JCC=TYPE_JCC
            ,TYPE_JMP=TYPE_JMP
            ,TYPE_LABEL=TYPE_LABEL
            ,TYPE_USE_PRESERVE=TYPE_USE_PRESERVE
            ,TYPE_USE_SET=TYPE_USE_SET
            ,TYPE_NOTUSE_PRESERVE=TYPE_NOTUSE_PRESERVE
            ,TYPE_NOTUSE_SET=TYPE_NOTUSE_SET
            ):
        """
        Similar, but instead of returning the list, calls a callback function.

        If the callback returns False, we abort immediately.

        If we return False, the result (including previous calls to the callback)
        is invalid and any side effect must be rolled back.
        """
        L = len(self._lines)
        while i + 1 < L:
            i += 1
            type = self.line_type(i)
            if type == TYPE_RET: # We assert no return value is carried in flags
                break
            elif type == TYPE_CALL: # Got call. We assert no arguments are carried in flags
                break
            elif type == TYPE_HALT: # Got halt/ud2. Not used.
                break
            elif type == TYPE_JCC: # Branches
                if not callback(i):
                    return False
                dest = self._lines[i].split()[1]
                desti = self._labels.get(dest)
                if desti is None:  # Unknown label???
                    return False
                if not self.flag_never_used(desti):
                    return False
            elif type == TYPE_JMP: # Jumps
                dest = self._lines[i].split()[1]
                desti = self._labels.get(dest)
                if desti is None:  # Unknown label???
                    return False
                if self.flag_never_used(desti):
                    break
                else:
                    return False
            elif type == TYPE_LABEL: # Labels
                # Since the control flow may jump in from elsewhere. We must make sure no flag is used.
                if self.flag_never_used(i):
                    break
                else:
                    return False
            elif type == TYPE_USE_PRESERVE:
                if not callback(i):
                    return False
            elif type == TYPE_USE_SET:
                if not callback(i):
                    return False
                break
            elif type == TYPE_NOTUSE_PRESERVE:
                pass
            elif type == TYPE_NOTUSE_SET:
                break
            else:  # What the hell?
                return False
        return True

    def flag_never_used(self, i
            ,TYPE_JMP=TYPE_JMP
            ,TYPE_LABEL=TYPE_LABEL
            ,TYPE_RET=TYPE_RET
            ,TYPE_CALL=TYPE_CALL
            ,TYPE_HALT=TYPE_HALT
            ,TYPE_NOTUSE_SET=TYPE_NOTUSE_SET
            ,TYPE_NOTUSE_PRESERVE=TYPE_NOTUSE_PRESERVE
            ):
        """
        Returns whether the flag starting at line i+1 is never used.
        If we cannot tell, we assume it's used and returns false
        """
        visited_labels = set() # Avoid dead loops
        lines = self._lines
        labels = self._labels
        self_line_type = self.line_type
        L = len(lines)
        while i + 1 < L:
            i += 1
            type = self_line_type(i)
            if type == TYPE_JMP:
                dest = lines[i].split()[1]
                if dest in visited_labels: # Dead loop. Not used
                    return True
                desti = labels.get(dest)
                if desti is None: # Unknown label
                    return False
                visited_labels.add(dest)
                i = desti
            elif type == TYPE_LABEL: # Got a label? No problem
                visited_labels.add(self._lines[i][:-1])
            elif type == TYPE_RET: # Got ret. Assert no return value is carried in flags
                return True
            elif type == TYPE_CALL: # Got call. Assert arguments are not carried in flags
                return True
            elif type == TYPE_HALT: # hlt/ud2. Not used
                return True
            elif type == TYPE_NOTUSE_SET: # Not used. Set. Great
                return True
            elif type == TYPE_NOTUSE_PRESERVE: # Not used. Not changed.
                pass
            else: # All other situations. Assume used.
                return False
        # Reaches EOF.  Assume there's some magic.
        return False

    def preserve_flags(self, i):
        return self.line_type(i) in (TYPE_USE_PRESERVE, TYPE_NOTUSE_PRESERVE)
