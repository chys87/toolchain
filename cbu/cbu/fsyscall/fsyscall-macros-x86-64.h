/*
 * cbu - chys's basic utilities
 * Copyright (c) 2013-2023, chys <admin@CHYS.INFO>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of chys <admin@CHYS.INFO> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY chys <admin@CHYS.INFO> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL chys <admin@CHYS.INFO> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if defined __cplusplus && !defined __clang__
#  undef __auto_type
#  define __auto_type auto
#endif

#define def_fsys_base(funcname, sysname, rettype, clobbermem, argc, ...)    \
  fsys_inline rettype fsys_##funcname(FSYS_FUNC_ARGS_##argc(__VA_ARGS__)) { \
    rettype r;                                                              \
    FSYS_LOAD_REGS_##argc(__VA_ARGS__);                                     \
    __asm__ __volatile__("syscall"                                          \
                         : "=a"(r)                                          \
                         : "a"(__NR_##sysname)FSYS_ASM_REGS_##argc          \
                         : FSYS_MEMORY_##clobbermem "cc", "r11", "cx");     \
    return r;                                                               \
  }
#define def_fsys(funcname,sysname,rettype,argc,...)                 \
  def_fsys_base(funcname,sysname,rettype,1,argc,##__VA_ARGS__)
#define def_fsys_nomem(funcname,sysname,rettype,argc,...)           \
  def_fsys_base(funcname,sysname,rettype,0,argc,##__VA_ARGS__)

// This is a macro, so we'd better separate the two stages (LOAD_ARGS/LOAD_REGS)
#define fsys_generic_base(sysno,rettype,clobbermem,argc,...)        \
  __extension__({                                                   \
      FSYS_GENERIC_LOAD_ARGS_##argc(__VA_ARGS__);                   \
      FSYS_GENERIC_LOAD_REGS_##argc;                                \
      unsigned long sysno__ = (sysno);                              \
      rettype r__;                                                  \
      __asm__ __volatile__ ("syscall" :                             \
          "=a"(r__) :                                               \
          "a"(sysno__) FSYS_GENERIC_ASM_REGS_##argc :               \
          FSYS_MEMORY_##clobbermem "cc", "r11", "cx");              \
      r__;                                                          \
  })
#define fsys_generic(sysno,rettype,argc,...) \
  fsys_generic_base(sysno,rettype,1,argc,##__VA_ARGS__)
#define fsys_generic_nomem(sysno,rettype,argc,...) \
  fsys_generic_base(sysno,rettype,0,argc,##__VA_ARGS__)

#define FSYS_LOAD_REGS_0()
#define FSYS_LOAD_REGS_1(ta) \
  ta A /*__asm__("rdi")*/ = a
#define FSYS_LOAD_REGS_2(ta,tb) \
  FSYS_LOAD_REGS_1(ta); tb B /*__asm__("rsi")*/ = b
#define FSYS_LOAD_REGS_3(ta,tb,tc) \
  FSYS_LOAD_REGS_2(ta,tb); tc C /*__asm__("rdx")*/ = c
#define FSYS_LOAD_REGS_4(ta,tb,tc,td) \
  FSYS_LOAD_REGS_3(ta,tb,tc); register td D __asm__("r10") = d
#define FSYS_LOAD_REGS_5(ta,tb,tc,td,te) \
  FSYS_LOAD_REGS_4(ta,tb,tc,td); register te E __asm__("r8") = e
#define FSYS_LOAD_REGS_6(ta,tb,tc,td,te,tf) \
  FSYS_LOAD_REGS_5(ta,tb,tc,td,te); register tf F __asm__("r9") = f

#define FSYS_ASM_REGS_0
#define FSYS_ASM_REGS_1 FSYS_ASM_REGS_0,"D"(A)
#define FSYS_ASM_REGS_2 FSYS_ASM_REGS_1,"S"(B)
#define FSYS_ASM_REGS_3 FSYS_ASM_REGS_2,"d"(C)
#define FSYS_ASM_REGS_4 FSYS_ASM_REGS_3,"r"(D)
#define FSYS_ASM_REGS_5 FSYS_ASM_REGS_4,"r"(E)
#define FSYS_ASM_REGS_6 FSYS_ASM_REGS_5,"r"(F)

#define FSYS_FUNC_ARGS_0() void
#define FSYS_FUNC_ARGS_1(ta) ta a
#define FSYS_FUNC_ARGS_2(ta,tb) ta a, tb b
#define FSYS_FUNC_ARGS_3(ta,tb,tc) ta a, tb b, tc c
#define FSYS_FUNC_ARGS_4(ta,tb,tc,td) ta a, tb b, tc c, td d
#define FSYS_FUNC_ARGS_5(ta,tb,tc,td,te) ta a, tb b, tc c, td d, te e
#define FSYS_FUNC_ARGS_6(ta,tb,tc,td,te,tf) ta a, tb b, tc c, td d, te e, tf f

#define FSYS_GENERIC_LOAD_ARGS_0()
#define FSYS_GENERIC_LOAD_ARGS_1(a) __auto_type a__ = (a)
#define FSYS_GENERIC_LOAD_ARGS_2(a, b) \
  FSYS_GENERIC_LOAD_ARGS_1(a);         \
  __auto_type b__ = (b)
#define FSYS_GENERIC_LOAD_ARGS_3(a, b, c) \
  FSYS_GENERIC_LOAD_ARGS_2(a, b);         \
  __auto_type c__ = (c)
#define FSYS_GENERIC_LOAD_ARGS_4(a, b, c, d) \
  FSYS_GENERIC_LOAD_ARGS_3(a, b, c);         \
  __auto_type d__ = (d)
#define FSYS_GENERIC_LOAD_ARGS_5(a, b, c, d, e) \
  FSYS_GENERIC_LOAD_ARGS_4(a, b, c, d);         \
  __auto_type e__ = (e)
#define FSYS_GENERIC_LOAD_ARGS_6(a, b, c, d, e, f) \
  FSYS_GENERIC_LOAD_ARGS_5(a, b, c, d, e);         \
  __auto_type f__ = (f)

#define FSYS_GENERIC_LOAD_REGS_0
#define FSYS_GENERIC_LOAD_REGS_1 __auto_type A__ /*__asm__("rdi")*/ = a__
#define FSYS_GENERIC_LOAD_REGS_2 \
  FSYS_GENERIC_LOAD_REGS_1;      \
  __auto_type B__ /*__asm__("rsi")*/ = b__
#define FSYS_GENERIC_LOAD_REGS_3 \
  FSYS_GENERIC_LOAD_REGS_2;      \
  __auto_type C__ /*__asm__("rdx")*/ = c__
#define FSYS_GENERIC_LOAD_REGS_4 \
  FSYS_GENERIC_LOAD_REGS_3;      \
  register __auto_type D__ __asm__("r10") = d__
#define FSYS_GENERIC_LOAD_REGS_5 \
  FSYS_GENERIC_LOAD_REGS_4;      \
  register __auto_type E__ __asm__("r8") = e__
#define FSYS_GENERIC_LOAD_REGS_6 \
  FSYS_GENERIC_LOAD_REGS_5;      \
  register __auto_type F__ __asm__("r9") = f__

#define FSYS_GENERIC_ASM_REGS_0
#define FSYS_GENERIC_ASM_REGS_1 FSYS_GENERIC_ASM_REGS_0,"D"(A__)
#define FSYS_GENERIC_ASM_REGS_2 FSYS_GENERIC_ASM_REGS_1,"S"(B__)
#define FSYS_GENERIC_ASM_REGS_3 FSYS_GENERIC_ASM_REGS_2,"d"(C__)
#define FSYS_GENERIC_ASM_REGS_4 FSYS_GENERIC_ASM_REGS_3,"r"(D__)
#define FSYS_GENERIC_ASM_REGS_5 FSYS_GENERIC_ASM_REGS_4,"r"(E__)
#define FSYS_GENERIC_ASM_REGS_6 FSYS_GENERIC_ASM_REGS_5,"r"(F__)
