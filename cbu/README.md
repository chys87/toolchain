# cbu - chys's basic utilities

This is my C++ utilities.

## Pre-requisites:

- [g++](https://gcc.gnu.org/) 9.0 or later
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

If you use bazel, Google Test is automatically downloaded from Github and built.

If you use anything else, you have to take care of it yourself.
