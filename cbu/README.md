# cbu - chys's basic utilities

This is my C++ utilities.

## Pre-requisites:

- [g++](https://gcc.gnu.org/) 10 or later
- [Bazel](https://bazel.build/).  Other make tools may also be used if you prefer; you can also just link or copy the files

## Bazel notes

If you need to specify your favorite compilers and/or library paths, create a file `user.bazelrc` and say:

```
build --action_env=CC=gcc-svn
build --action_env=CXX=g++-svn
build --action_env=BAZEL_LINKLIBS=-Wl,-rpath,/usr/local/LIB
```

## Unit test notes

Unit testing in this project requires [Google Test](https://github.com/google/googletest).

If you use bazel, Google Test can be automatically downloaded from Github and built.  Just type:

```
bazel test ...
```

If you use anything else, you have to take care of it yourself.

## List

### cbu/common

[`cbu/common`](cbu/common): Common utilities

* `align.h`: Alignment and power-of-two operations
* `bit_cast.h`: An (incomplete) implementation of C++20 [`std::bit_cast`](https://en.cppreference.com/w/cpp/numeric/bit_cast)
* `bit.h`: Bit manipulation code
  - `ctz`, `clz`, `bsr`, `popcnt`: type-generic bit manipulation functions
  - `set_bits`: Iterate over bits of an integer, e.g. `set_bits(0b00110011)` yields `0`, `1`, `4`, `5`
* `byteorder.h`: Conversion between little-endian and big-endian data
* `byte_size.h`: Provides `ByteSize`, that represents the size of an array, has semantics of an integer, but stores data in bytes
* `bytes_view.h`: Provides `BytesView`, similar with [`std::string_view`](https://en.cppreference.com/w/cpp/string/basic_string_view),
   with additional interfaces for convenient cast to and from pointers of other character types (`signed char`, `unsigned char`, `char8_t`, `std::bytes`)
   and `const void*` (explicit).
* `concepts.h`: Somewhat like C++20's [`<concepts>`](https://en.cppreference.com/w/cpp/header/concepts)
* `defer.h`: Provides macro `CBU_DEFER` for easier [RAII](https://en.cppreference.com/w/cpp/language/raii)
* `destruct.h`: Provides several destruction functions
* `encoding.h`: UTF-8 functions
* `fastarith.h`: Various C++ arithmetic functions
* `fastdiv.h`: Provides `fastdiv` and `fastmod`, to provide for faster division operations if the dividend is known to be small
* `faststr.h`: Provides `mempick`, `memdrop`, `append`, `concat` and similar functions for easy string operations
* `heapq.h`: Heap operations
* `immutable_string.h`: An "immutable" string class that holds either a `string_view` or a real `string` object.
* `memory.h`: Provides some C++20 memory functions that are missing from GCC 9
* `mp.h`: Multi-precicision integer operations: full support for addition, subtraction, multiplication and limited support for division
* `once.h`: Provides macro `CBU_ONCE`, somewhat like but easier to use than [`pthread_once`](https://linux.die.net/man/3/pthread_once)
* `range.h`: Wraps two iterators to an object for [range-based for](https://en.cppreference.com/w/cpp/language/range-for)
* `scoped_fd.h`: Wraps a file descriptor in a class
* `shared_instance.h`: An enhanced singleton implementation
* `stdhack.h`: Hacks standard strings and containers, providing resizing without initialization
* `strpack.h`: Converts a string literal to a class with variadic templates
* `strutil.h`: Various string utilities, higher level than `faststr.h`
* `swap.h`: Like [boost::swap](https://www.boost.org/doc/libs/1_64_0/libs/core/doc/html/core/swap.html), but also suports swapping by calling a member function named `swap`
* `type_traits.h`: Adds some type traits missing from [`<type_traits>`](https://en.cppreference.com/w/cpp/header/type_traits)
* `utility.h`: Adds some generic utilities missing from [`<utility>`](https://en.cppreference.com/w/cpp/header/utility),
   e.g. POD pair type, `reversed` (like Python's counterpart), `enumerate` (like Python's counterpart)

### cbu/fsyscall

"Inline system call wrapper for the crazy people", see [fsyscall/README.md](cbu/fsyscall/README.md).

This one is just a hobby - a crazy one.

### cbu/io

[`cbu/io`](cbu/io): IO utilities

### cbu/malloc

[`cbu/malloc`](cbu/malloc): My malloc implementation.  See [benchmark data](cbu/malloc).

### cbu/sys

[`cbu/sys`](cbu/sys): System utilities

* `low_level_mutex.h`: Implement a very simple and low-level mutex (for use in low-level code such as malloc)
