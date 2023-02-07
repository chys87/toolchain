#define def_fsys_base(funcname, sysname, rettype, clobbermem, argc, ...)    \
  fsys_inline rettype fsys_##funcname(FSYS_FUNC_ARGS_##argc(__VA_ARGS__)) { \
    FSYS_LOAD_REGS_##argc(__VA_ARGS__);                                     \
    register rettype r __asm__("x0");                                       \
    register long syscall_no __asm__("x8") = __NR_##sysname;                \
    __asm__ __volatile__("svc 0"                                            \
                         : "=r"(r)                                          \
                         : "r"(syscall_no)FSYS_ASM_REGS_##argc              \
                         : FSYS_MEMORY_##clobbermem "cc");                  \
    return r;                                                               \
  }
#define def_fsys(funcname,sysname,rettype,argc,...)                 \
  def_fsys_base(funcname,sysname,rettype,1,argc,##__VA_ARGS__)
#define def_fsys_nomem(funcname,sysname,rettype,argc,...)           \
  def_fsys_base(funcname,sysname,rettype,0,argc,##__VA_ARGS__)

// This is a macro, so we'd better separate the two stages (LOAD_ARGS/LOAD_REGS)
#define fsys_generic_base(sysno, rettype, clobbermem, argc, ...)    \
  __extension__({                                                   \
    FSYS_GENERIC_LOAD_ARGS_##argc(__VA_ARGS__);                     \
    FSYS_GENERIC_LOAD_REGS_##argc;                                  \
    register unsigned long sysno__ __asm__("x8") = (sysno);         \
    register rettype r__ __asm__("x0");                             \
    __asm__ __volatile__("svc 0"                                    \
                         : "=r"(r__)                                \
                         : "r"(sysno__)FSYS_GENERIC_ASM_REGS_##argc \
                         : FSYS_MEMORY_##clobbermem "cc");          \
    r__;                                                            \
  })
#define fsys_generic(sysno,rettype,argc,...) \
  fsys_generic_base(sysno,rettype,1,argc,##__VA_ARGS__)
#define fsys_generic_nomem(sysno,rettype,argc,...) \
  fsys_generic_base(sysno,rettype,0,argc,##__VA_ARGS__)

#define FSYS_LOAD_REGS_0()
#define FSYS_LOAD_REGS_1(ta) \
  register ta A __asm__("x0") = a
#define FSYS_LOAD_REGS_2(ta,tb) \
  FSYS_LOAD_REGS_1(ta); register tb B __asm__("x1") = b
#define FSYS_LOAD_REGS_3(ta,tb,tc) \
  FSYS_LOAD_REGS_2(ta,tb); register tc C __asm__("x2") = c
#define FSYS_LOAD_REGS_4(ta,tb,tc,td) \
  FSYS_LOAD_REGS_3(ta,tb,tc); register td D __asm__("x3") = d
#define FSYS_LOAD_REGS_5(ta,tb,tc,td,te) \
  FSYS_LOAD_REGS_4(ta,tb,tc,td); register te E __asm__("x4") = e
#define FSYS_LOAD_REGS_6(ta,tb,tc,td,te,tf) \
  FSYS_LOAD_REGS_5(ta,tb,tc,td,te); register tf F __asm__("x5") = f

#define FSYS_ASM_REGS_0
#define FSYS_ASM_REGS_1 FSYS_ASM_REGS_0,"r"(A)
#define FSYS_ASM_REGS_2 FSYS_ASM_REGS_1,"r"(B)
#define FSYS_ASM_REGS_3 FSYS_ASM_REGS_2,"r"(C)
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
#define FSYS_GENERIC_LOAD_ARGS_1(a) \
  __typeof__((a)) a__ = (a)
#define FSYS_GENERIC_LOAD_ARGS_2(a,b) \
  FSYS_GENERIC_LOAD_ARGS_1(a); __typeof__((b)) b__ = (b)
#define FSYS_GENERIC_LOAD_ARGS_3(a,b,c) \
  FSYS_GENERIC_LOAD_ARGS_2(a,b); __typeof__((c)) c__ = (c)
#define FSYS_GENERIC_LOAD_ARGS_4(a,b,c,d) \
  FSYS_GENERIC_LOAD_ARGS_3(a,b,c); __typeof__((d)) d__ = (d)
#define FSYS_GENERIC_LOAD_ARGS_5(a,b,c,d,e) \
  FSYS_GENERIC_LOAD_ARGS_4(a,b,c,d); __typeof__((e)) e__ = (e)
#define FSYS_GENERIC_LOAD_ARGS_6(a,b,c,d,e,f) \
  FSYS_GENERIC_LOAD_ARGS_5(a,b,c,d,e); __typeof__((f)) f__ = (f)

#define FSYS_GENERIC_LOAD_REGS_0
#define FSYS_GENERIC_LOAD_REGS_1 \
  register __typeof__((a__)) A__ __asm__("x0") = a__
#define FSYS_GENERIC_LOAD_REGS_2 \
  FSYS_GENERIC_LOAD_REGS_1; register __typeof__((b__)) B__ __asm__("x1") = b__
#define FSYS_GENERIC_LOAD_REGS_3 \
  FSYS_GENERIC_LOAD_REGS_2; register __typeof__((c__)) C__ __asm__("x2") = c__
#define FSYS_GENERIC_LOAD_REGS_4 \
  FSYS_GENERIC_LOAD_REGS_3; register __typeof__((d__)) D__ __asm__("x3") = d__
#define FSYS_GENERIC_LOAD_REGS_5 \
  FSYS_GENERIC_LOAD_REGS_4; register __typeof__((e__)) E__ __asm__("x4") = e__
#define FSYS_GENERIC_LOAD_REGS_6 \
  FSYS_GENERIC_LOAD_REGS_5; register __typeof__((f__)) F__ __asm__("x5") = f__

#define FSYS_GENERIC_ASM_REGS_0
#define FSYS_GENERIC_ASM_REGS_1 FSYS_GENERIC_ASM_REGS_0,"r"(A__)
#define FSYS_GENERIC_ASM_REGS_2 FSYS_GENERIC_ASM_REGS_1,"r"(B__)
#define FSYS_GENERIC_ASM_REGS_3 FSYS_GENERIC_ASM_REGS_2,"r"(C__)
#define FSYS_GENERIC_ASM_REGS_4 FSYS_GENERIC_ASM_REGS_3,"r"(D__)
#define FSYS_GENERIC_ASM_REGS_5 FSYS_GENERIC_ASM_REGS_4,"r"(E__)
#define FSYS_GENERIC_ASM_REGS_6 FSYS_GENERIC_ASM_REGS_5,"r"(F__)
