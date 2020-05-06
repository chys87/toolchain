# -*- coding: utf-8 -*-

import re
import hashlib

from . import utils
from . import x86

__all__ = ['rewrite_generic']

_pattern_addressing_const = br'\d+|0x[\da-fA-F]+|[A-Za-z_\.][\w.@]*'
_pattern_addressings = br'[-+]?(?:'+_pattern_addressing_const+br')?(?:[-+](?:'+_pattern_addressing_const+br'))*\((?:,?%\w+)+(?:,\d)?\)'

_bwlq_bits = {
        b'b': 8,
        b'w': 16,
        b'l': 32,
        b'q': 64,
    }

def _sub_streamline_branches(match, x86_cc_opposite=x86.cc_opposite):
    cc, label = match.group(1, 3)
    cc = x86_cc_opposite[cc]
    return b'\tj' + cc + b'\t' + label

# We *MUST* reject digits only labels. They're "relative" labels.
_label_consec_label = re.compile (br'^([._A-Za-z][.\w]*):\n((?:(?:\t\.p2align [,\d]+|[._A-Za-z][.\w]*:)\n)+)', re.M | re.A)
_label_immediate_jump = re.compile (br'^([._A-Za-z][.\w]*):\n\tjmp\t([._A-Za-z][.\w]*)$', re.M | re.A)
# Cannot add 'uleb128' to _label_ref_multi. It's unsafe to modify the excpetion range in gcc_except_table
_label_ref_multi = re.compile(br'^(\t(?:jmp|jn?[espo]|j[abgl]e?|\.quad|\.long|mov[lq]?)\t\$?)((?!\d)[.\w]+)(?=(,[^;\n]*)?$)', re.M | re.A)
def propagate_jumps(contents
        ,dict_get=dict.get
        ,frozenset=frozenset
        ,match_group=utils.match_type.group
        ):
    # Propagate jumps.
    # Old                New
    #      jz .L2        jz .L3
    # .L2: jmp .L3    .L2: jmp .L3

    lst = {}
    # Collect consecutive labels
    for dst, srcs in _label_consec_label.findall(contents):
        for src in srcs.splitlines():
            if src[-1:] == b':':
                lst[src[:-1]] = dst
    # Add immediate jumps
    for src, dst in _label_immediate_jump.findall(contents):
        lst[src] = dict_get(lst, dst, dst)
    # Avoid circles: We require each label can only be in either lst.keys() or lst.values()
    for lbl in frozenset(lst.values()).intersection(lst.keys()):
        del lst[lbl]
    def sub(match):
        lbl = match_group(match, 2)
        return match_group(match, 1) + dict_get(lst, lbl, lbl)
    contents = _label_ref_multi.sub(sub, contents)
    return contents

def _sub_fix_add_mov(match
        ,int=int
        ,match_group=utils.match_type.group
        ,x86_reg_to_64=x86.reg_to_64):
    imm = int(match_group(match, 1), 0)
    r1 = match_group(match, 2)
    off = match_group(match, 3)
    if not off:
        off = 0
    else:
        off = int(off, 0)
    r2 = match_group(match, 4)
    s = '\tmov\t{}(%{}),%{}'.format(imm+off,r1.decode(),r2.decode()).encode()
    if x86_reg_to_64.get(r1, r1) != x86_reg_to_64.get(r2, r2):
        s += '\n\tadd\t${},%{}'.format(imm,r1.decode()).encode()
    return s

def _sub_fix_testl_to_testb(match):
    imm = int(match.group(1), 0)
    off = int(match.group(2), 0)
    if imm < 0x100:
        return match.group(0)
    elif (imm & 0xff00) == imm:
        imm >>= 8
        off += 1
    elif (imm & 0xff0000) == imm:
        imm >>= 16
        off += 2
    elif (imm & 0xff000000) == imm:
        imm >>= 24
        off += 3
    else:
        return match.group(0)
    return '\ttestb\t${},{}'.format(imm, off).encode()

def _sub_fix_consec_store(match):
    bwlq = match.group('W')
    addr = match.group('addr')
    if bwlq==b'l' and b'%r' not in addr: # 32-bit system has no movq
        return match.group(0)
    bwlq_bytes = _bwlq_bits[bwlq] // 8
    bwlq_range = 256 ** bwlq_bytes
    o1 = match.group('o1')
    o2 = match.group('o2')
    o1 = int(o1) if o1 else 0
    o2 = int(o2) if o2 else 0
    i1 = int(match.group('i1'))
    i2 = int(match.group('i2'))
    if o2 - o1 == bwlq_bytes:
        pass
    elif o1 - o2 == bwlq_bytes:
        t = o1; o1 = o2; o2 = t
        t = i1; i1 = i2; i2 = t
    else:
        return match.group(0)
    if i1 < 0:
        i1 += bwlq_range
    if i2 < 0:
        i2 += bwlq_range
    i = i2 * bwlq_range + i1
    if bwlq==b'b':
        W = 'w'
    elif bwlq==b'w':
        W = 'l'
    else:
        assert bwlq==b'l'
        if (i>>31<<31) not in (0,0xffffffff80000000):
            return match.group(0)
        W = 'q'
    return '\tmov{}\t${},{}{}'.format(W, i, o1,addr.decode()).encode()

def _sub_fix_shr_shl(match):
    bits, reg = match.group(1, 2)
    mask = -1 << int(bits)
    return '\tand\t${},{}'.format(mask, reg.decode()).encode()


_simple_onetime_fixes = (
    # ".p2align 1" followed by ".p2align 4" makes no sense
    (br'^\t\.p2align 1\n(?=\t\.p2align 4$)', b''),

    # Replace "pslld $1, %xmm" with "paddd %xmm, %xmm"
    (br'^\t(?P<V>(?P<v>v)?)pslld\t\$1, ?(?P<x>%[xy]mm\d\d?)(?P<y>(?(v), ?%[xy]mm\d\d?))$', br'\t\g<V>paddd\t\g<x>,\g<x>\g<y>'),
    (br'^\t(?P<V>(?P<v>v)?)psllq\t\$1, ?(?P<x>%[xy]mm\d\d?)(?P<y>(?(v), ?%[xy]mm\d\d?))$', br'\t\g<V>paddq\t\g<x>,\g<x>\g<y>'),

    # Replace some operations of "pd" with "ps". Safe substitutions only
    # This is recommended by Intel Optimization Manual and used by ICC.
    # (However, we don't have to do this for AVX instructions. In AVX,
    #  PS and PD have the same length.)
    # (br'^\t(mov[alhu]|andn?|x?or)pd\t', br'\t\1ps\t'),

    # Remove some "insertps $15, %xmm, %xmm"
    #(br'^(\tv?movd\t%\w+, ?(%xmm\d\d?))\n\tv?insertps\t\$(?:15|0xe), ?\2, ?\2(, ?\2)?$', br'\1'),
    # Eliminate jump/branch to current position.
    # This may be generated as a result of __builtin_unreachable() or handwritten assembly from openssl
    (br'^\tj(n?[espo]|[abgl]e?|mp[lq]?)\t(\.L\w+)\n(?=(?:\t\.p2align [,\d]+\n)*(?:\.L\w+:\n)*\2:$)', br''),
    # Substitute "movq %r1,%r2; andl $imm,%r2d" with "movl %r1d, %r2d; andl $imm, %r2d"
    # Do this only if r1 and r2 are among the first 8 GP registers
    (br'movq?\t%r([a-z]+), ?%r([a-z]+)(?=\n\tandl?\t\$\d+, ?%e\2)$', br'movl\t%e\1,%e\2'),
    # Substitute "mov %r1,%r2; test %r2,%r2" with "mov %r1,%r2; test %r1,%r1"
    (br'^(\tmov[bwlq]?\t%(\w+), ?%(\w+)\n)\ttest([bwlq]?)\t%\3, ?%\3$', br'\1\ttest\4\t%\2,%\2'),
    # Substitute "add $imm,%r1; mov off(%r1),%r2" with "mov imm+off(%r1),%r2; add $imm,%r1" (r1 != r2)
    # We do not support "r2 = MMX register"
    (br'^\tadd[lq]?\t\$(-?\d+), ?%(\w+)\n\tmov[bwlq]?\t(-?\d*)\(%\2\), ?%((?!\2|mm\d)\w+)$', _sub_fix_add_mov),
    # Fix some testl, e.g. "testl $0x100,4(%rsp)" ==> "testb $0x1,5(%rsp)" (They produce exactly the same flags)
    (br'^\ttestl\t\$((?:0x)?[\da-fA-F]{3,}), ?(-?(?:0x)?[\dA-Fa-f]+)(?=\()', _sub_fix_testl_to_testb),

    # Disabled as GCC now includes this optimization
    # Fix consecutive small stores. Do it multiple times to allow b->w->l->q
    #(br'^\tmov(?P<W>b)\t\$(?P<i1>-?\d+), ?(?P<o1>-?\d*)(?P<addr>\([\w%,]+\))\n\tmovb\t\$(?P<i2>-?\d+), ?(?P<o2>-?\d*)(?P=addr)$',
    #    _sub_fix_consec_store),
    #(br'^\tmov(?P<W>w)\t\$(?P<i1>-?\d+), ?(?P<o1>-?\d*)(?P<addr>\([\w%,]+\))\n\tmovw\t\$(?P<i2>-?\d+), ?(?P<o2>-?\d*)(?P=addr)$',
    #    _sub_fix_consec_store),
    #(br'^\tmov(?P<W>l)\t\$(?P<i1>-?\d+), ?(?P<o1>-?\d*)(?P<addr>\([\w%,]+\))\n\tmovl\t\$(?P<i2>-?\d+), ?(?P<o2>-?\d*)(?P=addr)$',
    #    _sub_fix_consec_store),

    # shr $IMM,%reg  ==> and $IMM1, %reg
    # shl $IMM,%reg
    (br'^\tshr[bwlq]?\t\$([1-9]|[12][0-9]|3[01]), ?([^$;\n]+)\n\tshl[bwlq]?\t\$\1, ?\2$',
        _sub_fix_shr_shl),
    (br'^\tshr[bwlq]?\t([^$;\n]+)\n\tshl[bwlq]?\t\1$',
        br'\tand\t$-2,\1'),

    # xor $IMM,%reg  ==> not %reg
    # and $IMM,%reg      and $IMM,%reg
    # Where IMM typically is 1
    (br'^\txor([bwlq]?)\t\$(\d+), ?(%\w+)(?=\n\tand[bwlq]?\t\$\2, ?\3\n)', br'\tnot\1\t\3'),

    # Fix addressing "0(%rbp,%reg,1)" to shorter (%reg,%rbp,1)" (where reg is neither rbp nor r13)
    # Ditto for "r13"
    (br'^([\$\w \t,;]*[ \t,])0?\(%r(bp|13),%r((?!bp|13)\w+)(,1)?\)', br'\1(%r\3,%r\2)'),

    # Add "ud2" after indirect jumping to stop frontend encoding. Recommended by intel optimization manual.
    # Require enumerated follower to avoid misfiring va_start generated by GCC<4.6
    (br'^(\tjmp[lq]?[ \t]+\*[^;\n]*\n)(?=\t\.p2align|\t\.section|\t\.cfi_|[.$\w]+:\n)', br'\1\tud2\n'),

    # GCC occasionally inserts duplicate prefetch instructions
    (br'^(\tprefetch\w+\t[^;\n]*\n)(?:\1)+', br'\1'),

    # GCC occasionally generates such suboptimal instructions
    # vmovdqa (%rsi), %xmm0             => vpcmpeqb (%rsi), %xmm1, %xmm0
    # ....
    # vpcmpeqb %xmm1, %xmm0, %xmm0
    (br'^\tv(?:movdq[au]|mov[au]p[sd])\t(?P<src>[^;\n]*), ?(?P<dst>%[xy]mm\d+)\n'
     br'(?P<ins>(?:\tadd[bwlq]?\t\$\d+, ?%\w+\n){,2})'
     br'\t(?P<cmp>vpcmpeq[bwdq]\t|vcmpeqp[sd]\t|vcmpneqp[sd]\t|vcmpneq_oqp[sd]\t|vcmpp[sd]\t\$([0347]|12), ?)(?!(?P=dst))(?P<src2>%[xy]mm\d+), ?(?P=dst), ?(?P=dst)\n',
        br'\t\g<cmp>\g<src>,\g<src2>,\g<dst>\n\g<ins>'),

    # Store to stack and load
    # This usually happens when the return type is a structure with paddings
    (br'^\tmov([bwlq])\t(%\w+), ?([-\d]*\(%[er]sp\))\n'
     br'\tmov\1\t\3, ?(%\w+)\n',
        br'\tmov\1\t\2,\3\n\tmov\t\2,\4\n'),

    (br'^(\tvmov(?:[au]ps|dq[au])\t%[x-z]mm(\d+), ?([-\d]*\(%[er]sp\))\n)'
     br'\tmov[lq]?\t\3, ?%(r\d+d?|[er][a-d]x|[er]bp|[er][sd]i)\n',
        br'\1\tvmovd\t%xmm\2,%\4\n'),

    # Remove self-moving (may be a result of the last optimization)
    (br'^\tmov[bwq]?\t(%(' + b'|'.join(x86.regs8 | x86.regs16 | x86.regs64) + br')), ?\1\n', br''),

    # mov ...,%rsp
    # [LABEL:]
    # lea ....(%rbp),%rsp
    (br'^\tmovq?\t[^;\n]+, ?%rsp\n(?=([.\w]+:\n)*\tleaq?\t-?\d*\(%rbp\), ?%rsp\n)', br''),

    # vbroadcast+vinsert ==> vbroadcast to ymm; AVX==>AVX (src=mem); AVX2==>AVX2 (src=xmm)
    #(br'^\t(vp?broadcast([bwdq]|s[sd]))\t(?P<s>%xmm\d+|' + _pattern_addressings + br'), ?%xmm(?P<d>\d+)\n\tvinsert[if]128\t\$(0x)?1, ?%xmm(?P=d), ?%ymm(?P=d), %ymm(?P=d)$',
    #    br'\t\1\t\g<s>,%ymm\g<d>'),
    # vpunpcklqdq+vinserti128 ==> vpbroadcastq; AVX2 ==> AVX2 (vpbroadcast xmm,xmm is longer than vpunpcklqdq xmm,xmm,xmm; it's good to use broadcast for ymm)
    #(br'^\tvpunpcklqdq\t%xmm(\d+), ?%xmm\1, ?%xmm(\d+)\n\tvinserti128\t\$(0x)?1, ?%xmm\2, ?%ymm\2, ?%ymm\2$', br'\tvpbroadcastq\t%xmm\1,%ymm\2'),

    # Fix for C++: std::_Rb_tree_increment and std::_Rb_tree_decrement only neeeds to have one prototype
    #(br'^\t(call[lq]?|jmp[lq]?)\t_ZSt18_Rb_tree_(de|in)crementPKSt18_Rb_tree_node_base$',
    #        br'\t\1\t_ZSt18_Rb_tree_\2crementPSt18_Rb_tree_node_base'),

    # Fix for C++: std::exception::~exception is no-op.
    (br'^\tcall[lq]?\t_ZNSt9exceptionD2Ev(@PLT)?\n', br''),
    (br'^\tjmp[lq]?\t_ZNSt9exceptionD2Ev(@PLT)?$', br'\tret'),
)

_simple_onetime_fixes_x86_64 = (
)

_simple_onetime_fixes_lp64 = (
    # On x86-64, fopen64 and fopen alias each other.
    (br'^(\t(?:jmp|call)q?\tfopen)64((?:@PLT|@plt)?)$', br'\1\2'),
    # Remove unnecessary checks before free/delete
    (br'^\ttestq?\t%rdi, ?%rdi\n\tje\t([\.\w]+)\n(?=\tcallq?\t(c?free|_Zd[al]Pv)(@PLT)?\n\1:\n)', br''),
    (br'^\ttestq?\t%rdi, ?%rdi\n\tje\t([\.\w]+)\n(?=\tjmpq?\t(c?free|_Zd[al]Pv)(@PLT)?\n(\t\.p2align [ ,\d]+\n)*\1:\n\t(rep\t)?retq?\n)', br''),
)

_simple_onetime_fixes_x32 = (
    # Remove unnecessary checks before free/delete
    (br'^\ttestl?\t%edi, ?%edi\n\tje\t([\.\w]+)\n(?=\tcalll?\t(c?free|_Zd[al]Pv)(@PLT)?\n\1:\n)', br''),
    (br'^\ttestl?\t(%\w+), ?\1\n\tje\t([\.\w]+)\n(?=\tmovl?\t\1, ?%edi\n\tcalll?\t(c?free|_Zd[al]Pv)(@PLT)?\n\2:\n)', br''),
)

_simple_fixes_repeat = (
    # Exchange label with following p2align
    (re.compile (br'^(\.L\w+:\n)((?:\t\.p2align [,\d]+\n)+)', re.M | re.A), br'\2\1'),
    # Exchange align directives
    (re.compile (br'^(\t\.p2align 3\n)(\t\.p2align 4(?:,[,\d]+)?\n)', re.M | re.A), br'\2\1'),
    # Remove some unecessary align directives
    (re.compile (br'^(\t\.p2align 4,,[1-9]\d)\n\t\.p2align 4,,\d$', re.M | re.A), br'\1'),
    (re.compile (br'^(\t\.p2align 3)\n\t\.p2align 2$', re.M | re.A), br'\1'),

    # Collapse consecutive "rep\tret"
    (re.compile(br'^\t(rep\t)?ret\n(?=(\t\.p2align [,\d]+\n)*\.L\w+:\n\t(rep\t)?ret\n)', re.M | re.A), b''),
    # Collapse consecutive "vzeroupper; rep\tret"
    (re.compile(br'^\tvzeroupper\n\t(rep\t)?ret\n(?=(\t\.p2align [,\d]+\n)*\.L\w+:\n\tvzeroupper\n\t(rep\t)?ret\n)', re.M | re.A), b''),
    # Streamline branches.
    #         jnz .L2          jz .L3
    #         jmp .L3   ===>
    #    [.p2align...]
    #  .L2:
    (re.compile (br'^\tj([agbl]e?|n?[espo])\t(\.L\w+)\n\tjmp\t([.\w]+)(?=\n(?:\t\.p2align [,\d]+\n)*\2:$)', re.M | re.A),
        _sub_streamline_branches),
    # Removing a never used BB (most likely result of propagate_jumps or eliminating null-check before free/delete)
    (re.compile (br'^\t((rep\t)?ret|jmp\t[^\n]*)\n((\t\.p2align [,\d]+\n)*(\tjmp\t[^;\n]+\n|\t(rep\t)?ret\n))+', re.M | re.A), br'\t\1\n'),
)

_jmp_pair = {
# IMPORTANT: We cannot depend on their arithmetic meaning, but the actual
# flags they check!!!
# Example: We cannot add ('a','ae'):'e'. Because 'jae' tests C, while 'je' tests Z
# Otherwise we will cause problems with SSE4.2
        b'e\0be' : b'b',
        b'b\0be' : b'e',
        b'e\0a'  : b'ae',
        b'e\0le' : b'l',
        b'l\0le' : b'e',
        b'e\0g'  : b'ge',
    }


_pattern_andq_orq_mem = re.compile (br'\t(?P<ao>and|or)q\t\$(?P<imm>-?\d+), ?-?\d*\([^;]+$', re.A)

# Mandate the use of b/w/l/q suffix for cmp_imm (always used by GCC)
_pattern_cmp_imm = re.compile (br'\tcmp(?P<bwlq>[bwlq])\t\$(?P<imm>-?[1-9][0-9]*) ?(?P<o>,[^;]+)$', re.A)

# Cannot combine these two cases, because "cmp reg,mem" and "cmp mem,reg" are both supported, but
# there is no "comis[sd] reg,mem"(AT&T-style)
_pattern_cmp_reg_reg = re.compile (br'\t(?P<cmp>cmp[bwlq]?|v?comis[sd])\t(?P<r1>%\w+), ?(?P<r2>%\w+)$', re.A)
_pattern_cmp_reg_mem = re.compile (br'\t(?P<cmp>cmp[bwlq]?)\t(?P<r1>%\w+|'+_pattern_addressings+br'), ?(?P<r2>%\w+|'+
        _pattern_addressings+br')$', re.A)

_bwlq_range = { a:1<<b for a,b in _bwlq_bits.items() }

_skip_a_be_conv = {
        b'b': (2**8-1,),
        b'w': (0x7f, 2**16-1), # 0x7f is included because 0x7f can be encoded with one byte, while 0x80 cannot.
        b'l': (0x7f, 2**32-1),
        b'q': (0x7f, 2**31-1, 2**32-1, 2**64-1), # (2**31-1) is problematic because of the sign extension
    }



_pattern_a_be = re.compile (br'\t(j|cmov|set)(a|be)(\t[^;]+)$', re.A)
_conv_a_be = { b'a':b'ae', b'be':b'b' }
def _sub_a_be(match): # Used when converting "cmp $IMM,..." to "cmp $IMM+1,..."
    return b'\t' + match.group(1) + _conv_a_be[match.group(2)] + match.group(3)
_conv_a_be_neg = { b'a':b'b', b'be':b'ae' }
def _sub_a_be_neg(match): # Used when converting "cmp A,B" to "cmp B,A"
    return b'\t' + match.group(1) + _conv_a_be_neg[match.group(2)] + match.group(3)

_pattern_btl = re.compile(br'\tbt([bwlq]?)\t\$(\d+), ?([^;]+)', re.A)
_pattern_cmp_0 = re.compile (br'\t(test[bwlq]?\t%(\w+), ?%\2|cmp[bwlq]?\t\$(0x)?0,[^;]*)$', re.A)
_pattern_jl_jge = re.compile (br'\t(cmov|j)(l|ge)\t', re.A)
_conv_jl_jge = { b'l':b's', b'ge':b'ns' }
def _sub_jl_jge(match):
    return b'\t' + match.group(1) + _conv_jl_jge[match.group(2)] + b' '

_pattern_b_ae = re.compile(br'\t(j|cmov|set)(b|ae)(\t[^;]+)$', re.A)
_conv_b_ae_for_bt = { b'b':b'ne', b'ae':b'e' }
def _sub_b_ae_for_bt(match):
    return b'\t' + match.group(1) + _conv_b_ae_for_bt[match.group(2)] + match.group(3)

class GenericRewriter:
    _label_use = re.compile(br'(?<![\w\n])\.L\w+', re.M | re.A)
    _label_definition = re.compile(br'^(\.L\w+):\n', re.M | re.A)

    def __init__(self, opt):
        self._opt = opt

    def remove_unused_labels(self, contents
            ,frozenset=frozenset
            ,match_group=utils.match_type.group):
        # Remove unused local labels.
        # This will help future analyses
        used_labels = frozenset(self._label_use.findall(contents))
        def sub(match):
            if match_group(match, 1) in used_labels:
                return match_group(match, 0)
            return b''
        contents = self._label_definition.sub(sub, contents)
        return contents

    def convert_jmp_ret(self, contents):
        # OLD            NEW
        # jmp .Lx        ret
        # .Lx: ret       .Lx: ret
        ret_label_pattern = re.compile(br'^(\.\w+):\n\t(?:rep\t)?ret[lq]?$', re.M | re.A)
        ret_labels = ret_label_pattern.findall(contents)
        if not ret_labels:
            return contents
        labels_re = br'|'.join(label.replace(b'.', br'\.') for label in ret_labels)
        convert_pattern = re.compile(br'^\tjmp[lq]?\t(' + labels_re + br')$', re.M | re.A)

        return convert_pattern.sub(br'\tret', contents)

    def optimize_for_unreachable(self, contents):
        # Labels immediately followed by ".cfi_endproc" are probably generated by __builtin_unreachable()
        unreachable_pattern = re.compile(br'^(\.L\w+):\n\t\.cfi_endproc\n', re.M | re.A)
        unreachable_labels = unreachable_pattern.findall(contents)
        if not unreachable_labels:
            return contents
        eliminate_pattern = re.compile(br'^\tj(mp[lq]?|' + br'|'.join(x86.cc_opposite) + br')\t(' + br'|'.join(unreachable_labels) + br')\n', re.M | re.A)
        return eliminate_pattern.sub(br'', contents)

    def remove_empty_sections(self, contents):
        # For contiguous section specfiers, keep only the last one
        # We can't remove a line if it contains something like "ax",@progbits
        pattern = re.compile(br'^((\t\.section\t[\w\.]+\n|\t\.data\n|\t\.text\n){2,})', re.M | re.A)
        def sub(match):
            return match.group(1).splitlines(True)[-1]
        contents = pattern.sub(sub, contents)

        # For text sections, we can be more aggressive
        pattern = re.compile(br'^((\t\.section\t\.text(\.[\w\.]+)?\n|\t\.text|\.(b|p2)?align [\d,]+\n)+)(?=\t\.text|\t\.section)', re.M | re.A)
        contents = pattern.sub(b'', contents)
        return contents

    def icf(self, contents):
        # Identical code fold
        # Let's assume that we never compare the addresses of two functions
        # Currently this is only useful in LTO, since we don't recognize ".globl" yet

        function_pattern = re.compile(br'^\t(?P<section>\.section\t\.text\.[\.\w]+|\.text)\n' +
                br'\t(?P<align>\.(p2|b)?align [\d,]+)\n' +
                br'\t\.type[ \t](?P<f>[\w\.]+),\s*@function\n' +
                br'(?P=f):\n' +
                br'(?P<code>(\n|\t\.cfi[\w,-\. \t]+\n|[\w\.]+:\n|\t[^\.\n][^\n]*\n){1,20})' +
                br'\t\.size[ \t](?P=f), \.-(?P=f)\n', re.M | re.A)

        functions = {}

        for match in function_pattern.finditer(contents):
            section = match.group('section')
            align = match.group('align')
            code = match.group('code')
            name = match.group('f')
            key = (section, align, code)
            value = (name, match.span())
            functions.setdefault(key, []).append(value)

        replace_list = []
        for (section, align, code), func_list in functions.items():
            if len(func_list) < 2:
                continue
            identifier = b'.L_hackasICF_' + hashlib.md5(section + align + code).hexdigest().encode()
            replacement = b'\t' + section + b'\n\t' + align + b'\n'
            replacement += b'\t.type\t' + identifier + b', @function\n'
            replacement += identifier + b':\n'
            replacement += code
            replacement += b'\t.size\t' + identifier + b', .-' + identifier + b'\n'
            for func, _ in func_list:
                replacement += b'\t.set\t' + func + b',' + identifier + b'\n'
            for _, span in func_list:
                replace_list.append((span, replacement))
                replacement = b''

        if replace_list:
            replace_list.sort()
            res = []
            copied = 0
            for (lo, hi), replacement in replace_list:
                res.append(contents[copied:lo])
                res.append(replacement)
                copied = hi
            res.append(contents[copied:])
            contents = b''.join(res)

        return contents

    def do_onetime_fix(self, contents
            ,len=len
            ,bytes_startswith=bytes.startswith
            ,x86_cc_set=x86.cc_set):

        contents = utils.apply_re(contents, _simple_onetime_fixes)
        contents = utils.apply_re(contents, _simple_onetime_fixes_x86_64)
        if self._opt.abi == '64':
            contents = utils.apply_re(contents, _simple_onetime_fixes_lp64)
        if self._opt.abi == 'x32':
            contents = utils.apply_re(contents, _simple_onetime_fixes_x32)

        contents = self.convert_jmp_ret(contents)
        contents = self.optimize_for_unreachable(contents)
        contents = self.remove_empty_sections(contents)

        # ICF proves pretty useless, and it has caused strange bugs
        # contents = self.icf(contents)

        from . import flags
        lines = flags.LineByLine(contents)

        from .zeroextend import ZeroExtend
        ze = ZeroExtend(self._opt, contents)

        last_jmp = b''
        last_instruction = b'' # Used by ret fixer only

        def reassign(newline):
            nonlocal lines, line, i, tokens, key, operand
            lines[i] = line = newline
            tokens = line.split(maxsplit=1)
            if not tokens:
                key = b''
                operand = b''
            else:
                key = tokens[0]
                operand = b''
                if len(tokens) > 1:
                    operand = tokens[1]

        def flag_never_used_callback():
            return lines.flag_never_used(i)

        for i, line in enumerate(lines):
            # ZeroExtend
            zeres = ze(line, flag_never_used=flag_never_used_callback)
            if zeres is not None:
                lines[i] = line = zeres

            # Do not support ';' in line
            if b';' in line:
                last_jmp = b''
                last_instruction = b''
                continue

            # Extract this instruction name
            tokens = line.split(maxsplit=1)
            if not tokens:  # Empty line
                continue
            key = tokens[0]

            if key == b'lock' and len(tokens) > 1:
                op_tokens = tokens[1].split(maxsplit=1)
                if op_tokens:
                    tokens = op_tokens
                    key = b'lock\t' + tokens[0]

            operand = b''
            if len(tokens) > 1:
                operand = tokens[1]

            # Remove ".file" line
            if key == b'.file':
                if operand[:1] == b'"':
                    # Don't remove if operand starts with a number. The number is referred to in ".loc"
                    reassign(b'')
                continue

            # Fix "ret" and "rep ret". Common overuse of rep prefix in openssl
            if key == b'ret':
                if last_instruction in (b'call', b'syscall') or \
                        (last_instruction[:1] == b'j' and last_instruction[1:] in x86_cc_set):
                        # Don't add rep merely because there is a label or alignment
                        # (may be used by EH data only, or may be aligned for other reasons such as
                        #  preventing from returning from the last byte of a 16-byte chunk)
                    # It appears GCC thinks it's no longer necessary to use "rep ret" on my CPU.
                    # Keep the code in case one day I need to enable it again.
                    pass
                    #reassign(b'\trep\tret')
            elif key == b'rep' and operand == b'ret':
                if last_instruction.startswith((b'add', b'cltq', b'cmov', b'lea', b'mov', b'or', b'pop', b'pxor', b'xor', b'sub')):
                    reassign(b'\tret')
            if not bytes_startswith(key, (b'.cfi_', b'.p2align')):
                last_instruction = key

            # Replace certain conditional codes
            # Example:
            # Old        New
            # je L1        je L1
            # jbe L2    jb L2
            if key[:1] == b'j' and key[1:] in x86_cc_set:
                cur_jmp = key[1:]
                pair = last_jmp + b'\0' + cur_jmp
                new_jmp = _jmp_pair.get(pair)
                if new_jmp:
                    reassign(b'\tj' + new_jmp + b'\t' + operand)
                last_jmp = cur_jmp
            else:
                if last_jmp and not lines.preserve_flags(i):
                    last_jmp = b''

            # Replace andq/orq with andl/orl if equivalent
            if bytes_startswith(key, (b'and', b'or')):
                match = _pattern_andq_orq_mem.match(line)
                if match and lines.flag_never_used(i): # This change may affect flags
                    ao = match.group('ao') # Either "and" or "or"
                    imm = int(match.group('imm'))
                    if imm < 0:
                        imm += 2**64
                    if (ao==b'and' and (imm>>32)==0xffffffff):
                        reassign(line.replace(b'andq', b'andl'))
                    elif (ao==b'or' and (imm>>32)==0):
                        reassign(line.replace(b'orq', b'orl'))

            # Selectively replace "add/sub $1," with "inc/dec".
            # We do this only if this instruction is followed by an instruction which
            # (1) does not use flags;
            # (2) sets flags.
            if key.startswith((b'add', b'sub', b'lock\tadd', b'lock\tsub')) and operand[:3] == b'$1,':
                next_type = lines.line_type(i + 1)
                if next_type == flags.TYPE_NOTUSE_SET or \
                        (next_type == flags.TYPE_LABEL and lines.line_type(i + 2) == flags.TYPE_NOTUSE_SET):
                    newkey = key.replace(b'add', b'inc').replace(b'sub', b'dec')
                    reassign(b'\t' + newkey + b'\t' + operand[3:].strip())

            # Convert comparisons:
            # OLD            NEW
            # cmp $2, %al    cmp $3, %al
            # jbe .L1        jb .L1
            if key[:3] == b'cmp' and operand[:1] == b'$':
                match = _pattern_cmp_imm.match(line)
                if match:
                    imm = int(match.group('imm'))
                    bwlq = match.group('bwlq')
                    if imm < 0: # Convert negative number to positive representation
                        imm += _bwlq_range[bwlq]
                    if imm in _skip_a_be_conv[bwlq]: # Can't do it.
                        continue

                    flag_users = []
                    def callback(j):
                        match = _pattern_a_be.match(lines[j])
                        if match:
                            flag_users.append(j)
                        return match

                    if not lines.get_flag_users_cb(i, callback):
                        continue
                    if not flag_users:
                        # Result never used. This line can be removed.
                        # This can be the result of __builtin_unreachable
                        reassign(b'')
                        continue
                    # We can start the fix process.
                    newline = '\tcmp{}\t${}{}'.format(bwlq.decode(),imm+1,match.group('o').decode()).encode()
                    reassign(newline)
                    for j in flag_users:
                        lines[j] = _pattern_a_be.sub(_sub_a_be, lines[j])
                    continue

            # Fix cmp/comiss/comisd %r1, %r2; ja/jbe
            # To  cmp/comiss/comisd %r2, %r1; jb/jae
            if key.startswith((b'cmp', b'comis', b'vcomis')) and operand[:1] == b'%':
                match = _pattern_cmp_reg_reg.match(line)
                if match:
                    flag_users = []
                    def callback(j):
                        match = _pattern_a_be.match(lines[j])
                        if match:
                            flag_users.append(j)
                        return match

                    if not lines.get_flag_users_cb(i, callback) or not flag_users:
                        continue
                    # We can start the fix process.
                    line = '\t{}\t{},{}'.format(
                            match.group('cmp').decode(),
                            match.group('r2').decode(),
                            match.group('r1').decode(),
                            ).encode()
                    reassign(line)
                    for j in flag_users:
                        lines[j] = _pattern_a_be.sub(_sub_a_be_neg, lines[j])
                    continue

            # The same, but applies to "cmp reg,mem" and "cmp mem,reg"
            if key[:3] == b'cmp':
                match = _pattern_cmp_reg_mem.match(line)
                if match:
                    flag_users = []
                    def callback(j):
                        match = _pattern_a_be.match(lines[j])
                        if match:
                            flag_users.append(j)
                            return True
                        else:
                            return False
                    if not lines.get_flag_users_cb(i, callback) or not flag_users:
                        continue
                    # We can start the fix process.
                    line = '\t{}\t{},{}'.format(
                            match.group('cmp').decode(),
                            match.group('r2').decode(),
                            match.group('r1').decode(),
                            ).encode()
                    reassign(line)
                    for j in flag_users:
                        lines[j] = _pattern_a_be.sub(_sub_a_be_neg, lines[j])
                    continue

            # Fix "bt $const,...; jb/nb ...", common in handwritten assembly in openssl
            if key[:2] == b'bt':
                match = _pattern_btl.match(line)
                if match:
                    bwlq = match.group(1)
                    bit = int(match.group(2))
                    dst = match.group(3)

                    if not bwlq:
                        if dst.startswith(b'%'):
                            dst_bits = x86.reg_bits.get(dst[1:])
                            if dst_bits:
                                bwlq = x86.bwlq[dst_bits]
                    if not bwlq:
                        continue

                    if not 0 <= bit < min(32, _bwlq_bits[bwlq]):
                        continue

                    flag_users = []
                    def callback(j):
                        match = _pattern_b_ae.match(lines[j])
                        if match:
                            flag_users.append(j)
                            return True
                        else:
                            return False

                    if not lines.get_flag_users_cb(i, callback) or not flag_users:
                        continue
                    # We can start the fix process
                    if bit < 8 and (bwlq != b'b') and \
                            dst.startswith(b'%') and \
                            dst[1:] in x86.reg_to_8:
                        bwlq = b'b'
                        dst = b'%' + x86.reg_to_8[dst[1:]]
                    newline = '\ttest{}\t${},{}'.format(
                            bwlq.decode(),
                            1 << bit,
                            dst.decode()
                            ).encode()
                    reassign(newline)
                    for j in flag_users:
                        lines[j] = _pattern_b_ae.sub(_sub_b_ae_for_bt, lines[j])
                    continue

            # Convert some jl/jge
            # OLD: test %eax, %eax; jl/jge
            # NEW: test %eax, %eax; js/jns
            if key.startswith((b'cmp', b'test')) and operand and operand[0] in b'$%':
                match = _pattern_cmp_0.match(line)
                if match:
                    flag_users = lines.get_flag_users(i)
                    if not flag_users:
                        continue
                    for j in flag_users:
                        lines[j] = _pattern_jl_jge.sub(_sub_jl_jge, lines[j])
                    continue

        return lines.join()

    def do_fix_round(self,contents):
        contents = utils.apply_re(contents, _simple_fixes_repeat)
        contents = propagate_jumps(contents)
        contents = self.remove_unused_labels(contents)
        return contents

    def apply(self, contents):
        contents = self.remove_unused_labels(contents)
        contents = self.do_onetime_fix(contents)
        old_contents = None
        while contents != old_contents:
            old_contents = contents
            contents = self.do_fix_round(contents)
        return contents


def rewrite_generic(contents, opt):
    return GenericRewriter(opt).apply(contents)
