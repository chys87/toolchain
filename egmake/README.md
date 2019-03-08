egmake
======

Enhanced GNU make - My GNU make plugin (just a hobby)


This project is just a hobby.
It is a simple GNU make plugin to avoid always using `$(shell ...)`, which is significantly slower.
You need GNU make 4.0 to use it.

This project depends on [fsyscall](http://github.com/chys87/fsyscall), another insane hobby project of mine.
Symlink `fsyscal.h` properly before typing `make`.
