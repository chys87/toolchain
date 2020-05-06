# -*- coding: utf-8 -*-

import itertools
import re
import subprocess

from . import x86


__all__ = ['ZeroExtend']


def P(*args, **kwargs):
    return (''.join(tpl) for tpl in itertools.product(*args, **kwargs))

def _bwlq(*ins):
    return P(ins, ('', 'b', 'w', 'l', 'q'))

def _avx(*ins):
    return P(('', 'v'), ins)

def _yieldbytes(f):
    def foo(*args, **kwargs):
        return (s.encode('ascii') for s in f(*args, **kwargs))
    return foo


REG64 = tuple(regs[3] for regs in x86.regslist)
REG32 = tuple(regs[2] for regs in x86.regslist)
REG16 = tuple(regs[1] for regs in x86.regslist)
REG8  = tuple(regs[0] for regs in x86.regslist)
REGI = {r: i for (i, regs) in enumerate(x86.regslist) for r in regs}
NREG = 1 + max(REGI.values())
AX_I = REGI[b'ax']
BX_I = REGI[b'bx']
CX_I = REGI[b'cx']
DX_I = REGI[b'dx']
SP_I = REGI[b'sp']
DI_I = REGI[b'di']
SI_I = REGI[b'si']
R8_I = REGI[b'r8']
R9_I = REGI[b'r9']
R11_I = REGI[b'r11']
REGSLO_I = frozenset(REGI[x] for x in x86.regslo)

REG_I_BITS = {reg: (index, x86.reg_bits[reg]) for (reg, index) in REGI.items()}

@_yieldbytes
def _yield_functions_returning_pointers():
    lst = (
            'memcpy', 'memmove', 'mempcpy',
            'strchr', 'strrchr', 'memchr', 'memrchr', 'strdup', 'strpbrk', 'stpcpy',
            'strstr', 'memmem',
            'malloc', 'calloc', 'realloc',
            'memalign', 'aligned_alloc', 'mmap',
            'realpath', 'getenv',
            '__errno_location',
            '_Znwj', '_Znaj', # new, new[] (x32 version)
            '__cxa_allocate_exception',
            '__cxa_begin_catch',
            # gnumake
            'gmk_alloc',
    )
    return P(lst, ('', '@plt', '@PLT'))

AMD64_CALL_PRESERVED = frozenset(REGI[reg] for reg in (b'bx', b'bp', b'r12', b'r13', b'r14', b'r15'))
FUNCTIONS_RETURNING_POINTERS = frozenset(_yield_functions_returning_pointers())

@_yieldbytes
def _yield_noaffect_keys():
    # Doesn't affect registers (may modify flags)
    yield from ('.p2align', '.align', '.balign')
    yield from _bwlq('nop')
    yield from ('j' + cc for cc in x86.cc_set_str)
    yield from ('push', 'pushl', 'pushq', 'pushf')
    yield from _bwlq('cmp', 'test')

@_yieldbytes
def _yield_reset_keys():
    yield from P(('jmp', 'ret'), ('', 'l', 'q'))
    yield from ('cpuid', 'ud2', 'ud2a', 'hlt')

@_yieldbytes
def _yield_bits_results():
    yield from _bwlq('bsf', 'bsr', 'lzcnt', 'tzcnt', 'popcnt')

@_yieldbytes
def _yield_simple_ariths():
    '''Simple arithmetic operations.
    No side effect other than writing to target and modifying flags.
    '''
    yield from _bwlq('lea', 'add', 'sub', 'inc', 'dec', 'adc', 'sbb')
    yield from _bwlq('xor', 'and', 'or', 'andn')
    yield from _bwlq('rol', 'ror', 'shl', 'shr', 'sar', 'shrx', 'sarx', 'shlx')
    yield from _bwlq('not', 'neg')
    yield from _bwlq('crc32')
    yield from ('bswap', 'bswapl', 'bswapq')
    yield from _avx('movd', 'movq', 'pextrb', 'pextrw', 'pextrd', 'pextrq')  # XMM->REG
    yield from _avx(*P(('cvt', 'cvtt'), ('sd2si', 'ss2si')))
    yield from _avx('pmovmskb', 'movmskps')

@_yieldbytes
def _yield_cmovcc():
    yield from ('cmov' + cc for cc in x86.cc_set_str)

@_yieldbytes
def _yield_ignore_simd_keys():
    # Put SIMD instructions which NEVER clobber GPR
    yield from _avx('movaps', 'movups', 'movdqa', 'movdqu', 'movapd', 'movupd')
    yield from _avx(*P(('and', 'or', 'xor', 'add', 'sub'), ('sd', 'ss', 'pd', 'ps')))
    yield from _avx(*P(('padd', 'psub', 'pmaxu', 'pmaxs'), 'bwdq'))

@_yieldbytes
def _yield_memory_dst_ok_keys():
    # OK means no GPR is clobbered
    yield from _bwlq('mov', 'add', 'sub', 'inc', 'dec')
    yield from _bwlq('cmp', 'test')

NOAFFECT_KEYS = frozenset(_yield_noaffect_keys())
RESET_KEYS = frozenset(_yield_reset_keys())
BITS_RESULTS_KEYS = frozenset(_yield_bits_results())
SIMPLE_ARITH_KEYS = frozenset(_yield_simple_ariths())
CMOVCC_KEYS = frozenset(_yield_cmovcc())
IGNORE_SIMD_KEYS = frozenset(_yield_ignore_simd_keys())
MEMORY_DST_OK_KEYS = frozenset(_yield_memory_dst_ok_keys())

SINGLE_REG_ADDRESSING = re.compile(br'\(%(\w+)\)', re.A)
IMMEDIATE_VALUE = re.compile(b'[+-]?(0x[\da-f]+|0+|[1-9]\d*)', re.A)  # Decimal/hex only


def AnalyzeCxxPrototype(contents):

    def _demangle_all(contents):
        symbols = re.findall(br'\b(_Z\w+)', contents, re.A)
        if not symbols:
            return {}
        with subprocess.Popen(['c++filt', '-sgnu-v3'], stdin=subprocess.PIPE, stdout=subprocess.PIPE) as proc:
            out = proc.communicate(b''.join(symbol + b'\n' for symbol in symbols))[0]
        # We don't like the parentheses
        out = out.replace(b'(anonymous namespace)', b'__ANONYMOUS_NAMESPACE__')
        demangled_symbols = out.splitlines()
        assert len(symbols) == len(demangled_symbols)
        return dict(zip(symbols, demangled_symbols))

    demangle_dict = _demangle_all(contents)
    if not demangle_dict:
        return None

    # Names that appear before __ANONYMOUS_NAMESPACE__ are known to be namespaces
    recognized_namespace_prefixes = set()
    for prefix in frozenset(re.findall(br'^(?:\w+::)*__ANONYMOUS_NAMESPACE__::', b'\n'.join(demangle_dict.values()), re.M | re.A)):
        pos = prefix.find(b'::')
        while pos >= 0:
            recognized_namespace_prefixes.add(prefix[:pos + 2])
            pos = prefix.find(b'::', pos + 2)

    # Sort from longest to shortest
    recognized_namespace_prefixes = sorted(recognized_namespace_prefixes, key=len, reverse=True)

    def strip_namespace(name):
        for prefix in recognized_namespace_prefixes:
            if name.startswith(prefix):
                name = name[len(prefix):]
                break
        return name


    CTOR_PATTERN = re.compile(br'^(?:\w+::)*(\w+)::\1\(\)$', re.A)
    DTOR_PATTERN = re.compile(br'^(?:\w+::)*(\w+)::~\1\(\)$', re.A)
    PARAMETER_PATTERN = re.compile(br'^(?:\w+::)*\w+\((.*)\)(?: const)?$', re.A)
    NON_THISCALL_PATTERN = re.compile(br'^\w+\(', re.A)

    PARAMETER_POINTER = 0
    PARAMETER_GPR = 1
    PARAMETER_XMM = 2
    PARAMETER_UNKNOWN = 3

    POINTER_TYPE_PATTERN = re.compile(br'^[\w: ]+(?:\*|&+)$', re.A)

    INTEGRAL_KEYWORDS = frozenset([
        b'unsigned', b'signed',
        b'char', b'short', b'int', b'long',
        b'bool', b'wchar_t', b'char16_t', b'char32_t',
    ])
    XMM_PARAMETERS = [b'float', b'double']
    X32_PARAMETER_REGISTERS = DI_I, SI_I, DX_I, CX_I, R8_I, R9_I

    def _get_parameter_storage_type(parameter):
        if POINTER_TYPE_PATTERN.match(parameter):
            return PARAMETER_POINTER
        if all(part in INTEGRAL_KEYWORDS for part in parameter.split()):
            return PARAMETER_GPR
        if parameter in XMM_PARAMETERS:
            return PARAMETER_XMM
        return PARAMETER_UNKNOWN

    # Given a demangled C++ function name, return a set of registers which we
    # are certain that have 32-to-64 extended values
    def _get_z32_registers(name):
        if not name.startswith(b'_Z'):
            if name == 'main':  # argv, environ
                return (SI_I, DX_I)
            return ()

        name = demangle_dict.get(name, None)
        if not name:
            return ()

        # Not supported. No sufficient info
        if b'.' in name:  # Most likely something like .constprop or .isra
            return ()

        if name.startswith(b'_Z'):  # Not properly demangled
            return ()

        # Strip leading parts which we are certain are namespaces. Not interested
        name = strip_namespace(name)

        if DTOR_PATTERN.match(name):
            # Destructor: EDI = object
            return (DI_I,)

        # parse argument list
        match = PARAMETER_PATTERN.match(name)
        if match:
            parameters = match.group(1).split(b',')
            if not parameters:  # No parameter
                return ()
            para_pointer = []
            for parameter in parameters:
                if parameter.count(b'(') != parameter.count(b')'):
                    # This means we failed to parse some complicated types correctly
                    break
                stype = _get_parameter_storage_type(parameter)
                if stype == PARAMETER_POINTER:
                    para_pointer.append(True)
                elif stype == PARAMETER_GPR:
                    para_pointer.append(False)
                elif stype == PARAMETER_XMM:
                    pass
                else:
                    break

            if name.endswith(b') const') or CTOR_PATTERN.match(name):
                # Non-static.
                para_pointer = [True] + para_pointer
            elif NON_THISCALL_PATTERN.match(name):
                # Static or global
                pass
            else:
                # Don't know whether it's static or not
                if not para_pointer:
                    return ()
                para_pointer = [a and b for (a, b) in zip([True] + para_pointer[:-1], para_pointer)]
            return itertools.compress(X32_PARAMETER_REGISTERS, para_pointer)

        return ()

    return _get_z32_registers


def _look_ahead_no_static_address(s):
    # Check the last parts of s
    k = s.rfind(b',')
    if k >= 0:
        s = s[k + 1:]
    s = s.strip()
    if not s:
        return True
    try:
        int(s, 0)
        return True
    except:
        pass
    return False


# Use a function to emulate a class for better performance
def ZeroExtend(opt, contents
        # Performance hacks
        ,REG8=REG8
        ,REG16=REG16
        ,REG32=REG32
        ,REG64=REG64
        ,REGI=REGI
        ,REGSLO_I=REGSLO_I
        ,REG_I_BITS=REG_I_BITS
        ,bytes_endswith=bytes.endswith
        ,bytes_startswith=bytes.startswith
        ,len=len, int=int, bytes=bytes, list=list, tuple=tuple, min=min, max=max
        ,AX_I=AX_I, CX_I=CX_I
        ,RESET_KEYS=RESET_KEYS
        ,IGNORE_SIMD_KEYS=IGNORE_SIMD_KEYS
        ,MEMORY_DST_OK_KEYS=MEMORY_DST_OK_KEYS
        ,NOAFFECT_KEYS=NOAFFECT_KEYS
        ,BITS_RESULTS_KEYS=BITS_RESULTS_KEYS
        ,SIMPLE_ARITH_KEYS=SIMPLE_ARITH_KEYS
        ,CMOVCC_KEYS=CMOVCC_KEYS
        ,x86_reg_bits=x86.reg_bits
        ,IMMEDIATE_VALUE=IMMEDIATE_VALUE
        ,AMD64_CALL_PRESERVED=AMD64_CALL_PRESERVED
        ):
    _x32 = (opt.abi == 'x32')
    _64 = (opt.abi == '64')
    if _x32:
        _get_z32_registers = AnalyzeCxxPrototype(contents)
    else:
        _get_z32_registers = None

    _default_r = [64] * NREG
    if _x32:
        _default_r[SP_I] = 32

    sr = _default_r.copy()

    def _imm_bits(v, bits):
        if v < 0:
            return bits
        return min(v.bit_length(), bits)

    def reset():
        sr[:] = _default_r

    def feed(line, flag_never_used):
        '''Return the new line or None.
        flag_never_used is a callback function.
        '''
        res = None

        if b';' in line:
            reset()
            return res

        if line[-1:] == b':':
            reset()
            if _x32 and _get_z32_registers:
                for reg in _get_z32_registers(line[:-1]):
                    sr[reg] = 32
            return res

        tokens = line.split(maxsplit=1)
        if not tokens:  # Empty line
            return res
        key = tokens[0]
        if len(tokens) == 1:
            operand = b''
        else:
            operand = tokens[1]

        if key == b'.loc' or key[:5] == b'.cfi_':
            return res

        # Try to fix single-register addressing for x32
        if _x32 and operand and b'(%' in operand:
            match = SINGLE_REG_ADDRESSING.search(operand)
            if match:
                reg = match.group(1)
                if reg in x86.regs32 and sr[REGI[reg]] <= 32 and \
                        (sr[REGI[reg]] <= 31 or _look_ahead_no_static_address(operand[:match.start()])):
                    # We can do this only if the addressing is a small integer + register.
                    # For example, we can't do this: array(%eax) => array(%rax), in case %eax==-1
                    operand = operand[:match.start()] + b'(%' + x86.reg_to_64[reg] + b')' + operand[match.end():]
                    line = res = b'\t' + key + b'\t' + operand

        if key in RESET_KEYS:
            reset()
            return res

        # Assume regular x86-64 calling convention
        if key in (b'call', b'calll', b'callq'):
            old = tuple(sr)
            reset()
            if _64 or _x32:
                for ri in AMD64_CALL_PRESERVED:
                    sr[ri] = old[ri]
                if _x32 and operand in FUNCTIONS_RETURNING_POINTERS:
                    sr[AX_I] = 32
            return res

        # Assume Linux
        if key == b'syscall' and (_64 or _x32):
            sr[AX_I] = 64
            sr[CX_I] = 64
            sr[R11_I] = 64
            return res

        # Handle some instructions without operands
        if key == b'cltq':
            if sr[AX_I] < 32:
                res = b''
            else:
                sr[AX_I] = 64
            return res

        # Ignore most SSE/AVX instructions since they don't clobber GPR
        if key in IGNORE_SIMD_KEYS:
            return res

        # Handle some instructions with memory destination
        if operand[-1:] == b')' and key in MEMORY_DST_OK_KEYS:
            return res

        # Try to extract dst register
        if len(operand) < 3:
            if key not in NOAFFECT_KEYS:
                reset()
            return res

        percent_pos = operand.rfind(b'%')
        if percent_pos >= 0 and (percent_pos == 0 or operand[percent_pos-1] in b' \t,') and operand[percent_pos + 1:] in REG_I_BITS:
            dst_s = operand[percent_pos + 1:]
            dst, dst_bits = REG_I_BITS[dst_s]
        else:
            if key not in NOAFFECT_KEYS:
                reset()
            return res

        # Try to extract source, if it's a register
        src_s = b''
        src = -1
        src_bits = 0
        if operand[:1] == b'%' and operand[1:] != dst_s:
            loperand = len(operand)
            for l in (2, 3, 4):
                if loperand > l and operand[1:l + 1] in REG_I_BITS and \
                        (loperand == l + 1 or operand[l + 1] in b', \t'):
                    src_s = operand[1:l + 1]
                    src, src_bits = REG_I_BITS[src_s]
                    break

        # Try to extract source, if it's an immediate
        src_imm = None
        if operand[:1] == b'$':
            match = IMMEDIATE_VALUE.match(operand[1:])
            if match:
                src_s = match.group(0)
                src_imm = int(src_s, 0)
                if src_imm < 0:
                    src_imm += 1 << dst_bits

        if key in (b'mov', b'movb', b'movw', b'movl', b'movq', b'movabs', b'movabsq'):
            if dst == src and dst_bits == src_bits == 32 and sr[dst] <= 32:  # Remove useless "mov %r32,%r32"
                res = b''
            elif dst_bits == 64 and src >= 0 and sr[src] <= 32 and src in REGSLO_I and dst in REGSLO_I:  # Don't need to move 64 bits
                sr[dst] = sr[src]
                res = b'\tmov\t%' + REG32[src] + b',%' + REG32[dst]
            elif src_imm == 0 and flag_never_used():
                # Replace "mov $0, %reg" with "xor %reg, %reg"
                # (Yes, GCC generates such code occasionally)
                if dst_bits >= 32:
                    reg = REG32[dst]
                    sr[dst] = 0
                else:
                    reg = dst_s
                    if sr[dst] <= dst_bits:
                        sr[dst] = 0
                res = b'\txor\t%' + reg + b',%' + reg
            elif dst_bits in (32, 64):
                if src >= 0:
                    sr[dst] = min(dst_bits, sr[src])
                elif src_imm is not None:
                    imm_bits = _imm_bits(src_imm, dst_bits)
                    sr[dst] = imm_bits
                    if dst_bits == 64 and imm_bits <= 32:
                        res = b'\tmov\t$' + src_s + b',%' + REG32[dst]
                else:
                    sr[dst] = dst_bits
            else:
                sr[dst] = max(sr[dst], dst_bits)

        elif key in (b'movzbl', b'movzwl'):
            if src >= 0 and sr[src] <= src_bits:  # Not necessary to use movzx
                if src == dst:
                    res = b''
                else:
                    res = b'\tmov\t%' + REG32[src] + b',%' + dst_s
                    sr[dst] = sr[src]
            else:
                # Can't use src_bits, which may be 0!! (when src is not a register)
                if key == b'movzbl':
                    sr[dst] = 8
                else:
                    sr[dst] = 16

        elif key == b'movslq':
            if src >= 0 and sr[src] < 32:  # Not necessary to use movslq
                if src == dst:
                    res = b''
                else:
                    res = b'\tmov\t%' + REG32[src] + b',%' + REG32[dst]
                    sr[dst] = sr[src]
            else:
                sr[dst] = 64

        elif key in (b'xor', b'xorl', b'xorq') and dst == src and dst_bits in (32, 64):
            if sr[dst] == 0 and flag_never_used():  # Already zero
                res = b''
            else:
                if dst_bits == 64 and dst in REGSLO_I:
                    reg32 = REG32[dst]
                    res = b'\txor\t%' + reg32 + b',%' + reg32
                sr[dst] = 0

        elif key in (b'xor', b'xorl', b'xorq') and src_imm == 65535 and dst_bits in (32, 64) and sr[dst] <= 32 and flag_never_used(): # "xor $65535,%r32" => "not %r16"
            res = b'\tnot\t%' + REG16[dst]
            sr[dst] = max(sr[dst], 16)

        elif key in (b'shr', b'shrq') and dst_bits == 64 and src < 0 and sr[dst] <= 32 and (src_imm is None or src_imm < 32):  # SRC is $1 or $imm
            # "shr %r64" ==> "shr %r32". Don't do this if %cl is used. That changes behavior when %cl >= number of bits!!!
            res = b'\tshr\t'
            if src_imm is not None:
                res += b'$' + src_s + b','
            res += b'%' + REG32[dst]
            sr[dst] = max(0, sr[dst] - (src_imm or 1))

        elif key in (b'shr', b'shrl') and dst_bits == 32 and src < 0 and (src_imm is None or src_imm < 32):
            # shr %r32.
            sr[dst] = max(0, min(32, sr[dst]) - (src_imm or 1))

        elif key in (b'and', b'andl', b'andq') and dst_bits in (32, 64) and src_imm in (255, 65535, 0xffffffff) and flag_never_used():  # Convert and to movzx/mov
            res = b''
            if src_imm == 255:
                if sr[dst] > 8:
                    res = b'\tmovzbl\t%' + REG8[dst] + b',%' + REG32[dst]
                    sr[dst] = 8
            elif src_imm == 65535:
                if sr[dst] > 16:
                    res = b'\tmovzwl\t%' + REG16[dst] + b',%' + REG32[dst]
                    sr[dst] = 16
            elif src_imm == 0xffffffff:
                if sr[dst] > 32:
                    res = b'\tmov\t%' + REG32[dst] + b',%' + REG32[dst]
                    sr[dst] = 32
            else:
                assert False

        elif key in (b'and', b'andl', b'andq') and src_imm is not None:  # and $imm, %reg
            imm_bits = _imm_bits(src_imm, dst_bits)
            sr[dst] = min(sr[dst], imm_bits)
            if imm_bits <= 32 and dst_bits == 64 and dst in REGSLO_I:  # and $imm, %r64 ==> and $imm, $r32
                # We will not affect SF. It's impossible to encode a 32-bit immediate in a 64-bit instruction with most signficant bit set
                res = b'\tand\t$' + src_s + b',%' + REG32[dst]

        elif key in (b'and', b'andl', b'andq') and src >= 0:  # and %r1, %r2
            sr[dst] = min(sr[dst], sr[src])

        elif key in (b'test', b'testl', b'testq') and dst_bits > 8 and src_imm is not None and 0 <= src_imm <= 255:
            # testq $imm, %reg ==> test $imm, %r8
            res = b'\ttest\t$' + src_s + b',%' + REG8[dst]

        elif key in (b'test', b'testq') and dst_bits == 64 and src_imm is not None and dst in REGSLO_I and 0 <= src_imm < (1 << 32):
            # testq $imm, %r64 ==> test $imm, %r32
            res = b'\ttest\t$' + src_s + b',%' + REG32[dst]

        elif key in (b'cmp', b'cmpb', b'cmpw', b'cmpl', b'cmpq') and src_imm == 0:
            # "cmp $0, %reg" is eqv. to "test %reg,%reg", producing the same flags
            res = b'\ttest\t%' + dst_s + b',%' + dst_s

        elif key in (b'shl', b'sal', b'shlq', b'salq') and dst_bits == 64 and src < 0:
            if src_imm is not None and (
                        src_imm + sr[dst] < 32 or
                        (src_imm + sr[dst] == 32 and flag_never_used()) # Check for flags because we will change SF
                    ):
                # shlq => shll
                sr[dst] += src_imm
                res = b'\tsall\t$' + src_s + b',%' + REG32[dst]

            elif src_imm is None and (
                        src_imm + sr[dst] < 31 or
                        (src_imm + sr[dst] == 31 and flag_never_used()) # Check for flags because we will change SF
                    ):
                # Similar, but dealing with shifts by 1
                sr[dst] += 1
                res = b'\tsall\t%' + REG32[dst]
            else:
                sr[dst] = 64

        elif key in BITS_RESULTS_KEYS:  # bsf/bsr/tzcnt/lzcnt/popcnt
            sr[dst] = 7

        elif key in SIMPLE_ARITH_KEYS:
            if dst_bits in (32, 64):
                sr[dst] = dst_bits
            else:
                sr[dst] = max(sr[dst], dst_bits)

        elif key in CMOVCC_KEYS:
            if src >= 0 and src_bits == dst_bits == 64 and sr[dst] <= 32 and sr[src] <= 32:
                # "cmov %r64, %r64", but both registers have 32-bit values
                res = b'\t' + key + b'\t%' + REG32[src] + b',%' + REG32[dst]
                sr[dst] = max(sr[dst], sr[src])
            else:
                # "cmov .., %r32" doesn't clear high bits if not actually moved
                sr[dst] = max(sr[dst], dst_bits)

        elif key in NOAFFECT_KEYS:
            pass

        else:  # If instruction is unrecognized, reset
            reset()

        return res

    return feed
