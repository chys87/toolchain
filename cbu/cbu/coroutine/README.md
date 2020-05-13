# coroutine

This is my C++ coroutine implementation. (Just a small hobby - not tested on real-world production.)

I came up with the idea of implementing one when I learned of Tencent [libco](https://github.com/Tencent/libco), but
my implementation is very different from libco.

## Usage

```
CoContainer container;
container.Register([&]{
  // Coroutine A - do some network staff here.
});
container.Register([&]{
  // Coroutine B - do some network staff here.
});
container.Run();
```

It's possible to create new coroutines within another one:

```
container.Register([&]{
    auto coroutine_a = container.Register([&]{
        // Another coroutine - do some fancy network staff here.
    });
    auto coroutine_b = container.Register([&]{
        // Yet another coroutine - do some fancy network staff here.
    });

    // Wait for the two coroutines
    // This is optional - you don't have to wait for another coroutine.
    WaitFor(coroutine_a);
    WaitFor(coroutine_b);
});
```

## FIFO scheduler

My design seems to be more simliar with [libgo](https://github.com/yyzybb537/libgo).
Coroutines are put in a FIFO queue.
User-created coroutines cannot switch to one another directly.  A user-created coroutine always switches back to the shceduler, which then pops the next
coroutine from the FIFO and switch to it (or wait for IO if no coroutine is ready).

On the other hand, libco is very different.
It uses a stack design - newly created coroutines run immediately, and older coroutines are scheduled only if newly ones are waiting for IO.

## TODO

My syscall hooks are *very* incomplete right now.  Only `epoll_wait`, `poll`, `read`, `write`, `send`, `sendto`, `recv`, `recvfrom`, `usleep` and `sleep` are hooked.
There is lots of work to do for more syscalls to be hooked.

Also, my implementation hasn't hooked all syscalls that create FDs yet, so I have to call `fcntl` very often to determine whether a fd is non-blocking.
