# cbu-malloc

This is my own malloc implementation.
It is inspired by [tcmalloc](https://github.com/google/tcmalloc) and [jemalloc](https://github.com/jemalloc/jemalloc).

I have combined some designs from both tcmalloc and (a very early version of) jemalloc, and achieved comparable
performance with tcmalloc, and comparable low memory usage with jemalloc.

This implementation also aggressively attempts to make use of [anonymous huge page](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/performance_tuning_guide/s-memory-transhuge) (if enabled on your system).

Benchmark on my computer (running test case in [tests](tests)):

|   |cbu malloc|tcmalloc|jemalloc|ptmalloc|
|:-:|:-:|:-:|:-:|:-:|
|Time (user+sys)|<ins>5.10</ins>|**5.04**|5.58|5.53|
|CPU %|149|<ins>138</ins>|143|**133**|
|Max resident size (KiB)|<ins>875796</ins>|1643124|**779220**|1339036|
|VmRSS at exit (KiB)|<ins>263172</ins>|1320820|**101732**|1004396|

(Legend: **best**; <ins>2nd best</ins>)

## CAVEATS

My implementation doesn't (yet) register [atfork handlers](https://linux.die.net/man/3/pthread_atfork), so it's very unsafe to do fork in
a multi-threaded process (unless the child process immediately calls exec, and nothing between fork and exec requires malloc).
