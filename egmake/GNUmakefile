# egmake, Enhanced GNU make
# Copyright (C) 2014-2025, chys <admin@CHYS.INFO>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

.PHONY: all install clean test un size bincmp
.SUFFIXES:
.DELETE_ON_ERROR:

all: egmake.so

SRCS := $(wildcard *.c)
OBJS := $(SRCS:%.c=%.o)
DEPS := $(SRCS:%.c=%.P)

LDFLAGS := -shared -Wl,-O1,--gc-sections,--as-needed

ifneq "" "$(shell which clang 2>/dev/null)"
  CC := clang
endif

ifneq "" "$(findstring clang,$(CC))"
  STRIP := llvm-strip
  ifneq "" "$(shell which lld 2>/dev/null)"
    LDFLAGS += -fuse-ld=lld
  endif
else
  CC := gcc
  STRIP := strip
endif

ifneq "" "$(shell which ccache 2>/dev/null)"
  CC := ccache $(CC)
endif

CFLAGS := -O2 -march=native -flto -std=gnu2x -fPIC \
	-D_GNU_SOURCE -DNDEBUG \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -fno-exceptions \
	-fno-stack-protector -fno-stack-clash-protection \
	-fvisibility=hidden \
	-fmerge-all-constants -fno-common \
	-fno-ident \
	-Qn \
	-Wall -Wextra -Wno-shadow -Wwrite-strings \
	-Wno-unused-parameter -Wno-missing-field-initializers \
	-Wno-sign-compare -Wno-clobbered \
	-Wformat -Wformat-security -Wmissing-include-dirs -Wfloat-equal \
	-Wlogical-op \
	-Wunused-macros \
	-Wunused-local-typedefs \
	-Werror=address \
	-Werror=endif-labels \
	-Werror=comment \
	-Werror=multichar \
	-Werror=pointer-arith \
	-Werror=char-subscripts \
	-Werror=format \
	-Werror=format-extra-args \
	-Wno-format-zero-length \
	-Werror=return-type \
	-Werror=overflow \
	-Werror=parentheses \
	-Werror=trigraphs \
	-Werror=pragmas -Werror=unknown-pragmas \
	-Werror=parentheses \
	-Werror=strict-aliasing \
	-Werror=trampolines \
	-Werror=switch \
	-Wswitch-enum \
	-Werror=address \
	-Werror=enum-compare \
	-Wdisabled-optimization \
	-Wsuggest-attribute=noreturn \
	-Werror=implicit-int \
	-Werror=implicit-function-declaration \
	-Werror=old-style-declaration -Werror=old-style-definition \
	-Werror=missing-parameter-type \
	-Werror=strict-prototypes \
	-Werror=jump-misses-init

ifneq "" "$(findstring aarch64,$(shell $(CC) -dumpmachine))"
	CFLAGS += -mcmodel=tiny
endif

INSTNAME := ~/bin/egmake.so

-include Makefile-spec

egmake.so: egmake-unstripped.so
	$(STRIP) --strip-all --strip-unneeded -o $@ $<

egmake-unstripped.so: $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(DEPS): %.P: %.c
	$(CC) $(CFLAGS) -MM -MP -MF $@ -MT '$@ $(@:%.P=%.o)' $<

ifneq "$(MAKECMDGOALS)" "clean"
include $(DEPS)
endif

clean:
	rm -f *.so *.o

install: $(INSTNAME)
$(INSTNAME): egmake.so
	install -m755 -C -T -v $< $@

un: egmake-unstripped.so
	../scripts/unassembly $<

size: egmake.so
	../scripts/detailed_size $(wildcard $(INSTNAME)) egmake.so

bincmp:
	../scripts/bincmp $(INSTNAME) egmake.so

define define_test
.PHONY: test-$(1)
test: test-$(1)
test-$(1): egmake.so
	+./run_test.sh $(1)
endef
$(foreach src,$(SRCS),$(eval $(call define_test,$(src))))
