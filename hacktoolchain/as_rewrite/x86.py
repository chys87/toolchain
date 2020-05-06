# -*- coding: utf-8 -*-

# Some tuples, if considering semantics, should be replaced by lists, but in this utility,
# we care about speed more, and therefore tuples are always used instead of lists whenever
# possible.

regslolist = (
    (b'al',  b'ax',  b'eax', b'rax'),
    (b'dl',  b'dx',  b'edx', b'rdx'),
    (b'cl',  b'cx',  b'ecx', b'rcx'),
    (b'bl',  b'bx',  b'ebx', b'rbx'),
    (b'sil', b'si',  b'esi', b'rsi'),
    (b'dil', b'di',  b'edi', b'rdi'),
    (b'bpl', b'bp',  b'ebp', b'rbp'),
    (b'spl', b'sp',  b'esp', b'rsp'),
)

regshilist = (
    (b'r8b', b'r8w', b'r8d', b'r8'),
    (b'r9b', b'r9w', b'r9d', b'r9'),
    (b'r10b',b'r10w',b'r10d',b'r10'),
    (b'r11b',b'r11w',b'r11d',b'r11'),
    (b'r12b',b'r12w',b'r12d',b'r12'),
    (b'r13b',b'r13w',b'r13d',b'r13'),
    (b'r14b',b'r14w',b'r14d',b'r14'),
    (b'r15b',b'r15w',b'r15d',b'r15'),
)

regslist = regslolist + regshilist

regs64 = frozenset(q for (b,w,l,q) in regslist)
regs32 = frozenset(l for (b,w,l,q) in regslist)
regs16 = frozenset(w for (b,w,l,q) in regslist)
regs8  = frozenset(b for (b,w,l,q) in regslist)

regslo = frozenset(reg for regs in regslolist for reg in regs)

reg_bits = {reg: bits for (b, w, l, q) in regslist for (reg, bits) in ((b, 8), (w, 16), (l, 32), (q, 64))}

bwlq = {8: b'b', 16: b'w', 32: b'l', 64: b'q'}

reg_to_64 = { x:q for (b,w,l,q) in regslist for x in (b,w,l,q) }
reg_to_32 = { x:l for (b,w,l,q) in regslist for x in (b,w,l,q) }
reg_to_16 = { x:w for (b,w,l,q) in regslist for x in (b,w,l,q) }
reg_to_8  = { x:b for (b,w,l,q) in regslist for x in (b,w,l,q) }

cc_canonicalize = {
        b'c'  : b'b',
        b'na' : b'be',
        b'nae': b'b',
        b'nb' : b'ae',
        b'nbe': b'a',
        b'nc' : b'ae',
        b'ng' : b'le',
        b'nge': b'l',
        b'nl' : b'ge',
        b'nle': b'g',
        b'nz' : b'ne',
        b'pe' : b'p',
        b'po' : b'np',
        b'z'  : b'e',
    }

cc_opposite = {
        b'a'  : b'be',
        b'ae' : b'b',
        b'b'  : b'ae',
        b'be' : b'a',
        b'e'  : b'ne',
        b'g'  : b'le',
        b'ge' : b'l',
        b'l'  : b'ge',
        b'le' : b'g',
        b'ne' : b'e',
        b'no' : b'o',
        b'np' : b'p',
        b'ns' : b's',
        b'o'  : b'no',
        b'p'  : b'np',
        b's'  : b'ns',
    }

cc_set = frozenset(cc_canonicalize) | frozenset(cc_opposite)
cc_set_str = frozenset(cc.decode('ascii') for cc in cc_set)
