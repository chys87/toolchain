#include "tinyx32.h"


void abort() {
	fsys_kill(0, SIGABRT);
	__builtin_trap();
}
