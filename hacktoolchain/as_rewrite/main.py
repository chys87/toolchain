#! /usr/bin/python3
# -*- coding: utf-8 -*-

from __future__ import print_function

import sys
import os
import subprocess
import time
import fcntl

def find_as():
    best_as = '/usr/local/binutils-svn/bin/as'
    if os.access(best_as, os.X_OK):
        return best_as
    else:
        return 'as'


class Options:
    pass


def init_as(argv):
    """
    This function returns fr,args,opt
    fr is the file descriptor to read in
    args is the command line to invoke
    If we fail, it returns None, None, None
    """
    args = [find_as()]
    infile = None
    outfile = None
    skip_next = True
    opt = Options()
    opt.abi = '64'
    for (i, arg) in enumerate(argv):
        if skip_next:
            skip_next = False
        elif arg.startswith('-m') or arg in ('--noexecstack', '-W'):
            args.append (arg)
        elif arg in ('--64', '--x32'):  # --32 is not supported
            opt.abi = arg[2:]
            args.append(arg)
        elif arg == '-' or arg.endswith(('.s','.S')):
            if infile:
                return None, None, None
            infile = arg
        elif arg == '-I' and i + 1 < len(argv):
            args.append(arg)
            args.append(argv[i + 1])
            skip_next = True
        elif arg == '-o' and i + 1 < len(argv):
            outfile = argv[i + 1]
            args.append(arg)
            args.append(argv[i + 1])
            skip_next = True
        else:  # Unknown options
            return None, None, None
    if not outfile:  # No outfile specified
        return None, None, None

    if infile is None or infile == '-':  # Read from stdin
        fr = sys.stdin.buffer  # Python3: Use ".buffer" to force binary mode
    else:
        try:
            fr = open(infile, 'rb')
        except IOError:
            return None, None, None
    return fr, args, opt


class Dumper:

    DIR = '/tmp/.hackas'
    CLEAN_THRESHOLD = 600

    def __init__(self, old):
        try:
            os.mkdir(self.DIR)
        except FileExistsError:
            pass
        self._dir_fd = fd = os.open(self.DIR, os.O_RDONLY | os.O_DIRECTORY)
        fcntl.flock(fd, fcntl.LOCK_EX)

        try:
            threshold = int(time.time()) - self.CLEAN_THRESHOLD
            index = 0
            for x in os.listdir(fd):
                if not x.endswith('.s'):
                    continue
                if os.stat(x, dir_fd=fd).st_mtime < threshold:
                    os.unlink(x, dir_fd=fd)
                else:
                    try:
                        index = max(index, int(x[:5]))
                    except ValueError:
                        pass
            index += 1
            self._index = index
            with open('{:05}.0.s'.format(index), 'wb', opener=self._opener) as f:
                f.write(old)

        finally:
            fcntl.flock(fd, fcntl.LOCK_UN)

    def dump_new(self, new):
        fname = '{:05}.1.s'.format(self._index)
        f = open(fname, 'wb+', opener=self._opener)
        f.write(new)
        f.seek(0, os.SEEK_SET)
        return f

    def _opener(self, filename, flags):
        return os.open(filename, flags, 0o644, dir_fd=self._dir_fd)

    def __del__(self):
        fd = self._dir_fd
        if fd >= 0:
            os.close(fd)


def _rewrite(fr, opt):
    # Read in everything
    content = fr.read()
    fr.close() # Might be stdin. OK.

    # Dump old file
    dumper = Dumper(content)

    if os.environ.get('HACKAS_PROFILE'):
        try:
            import cProfile as profile
        except ImportError:
            import profile
        import pstats
        pr = profile.Profile()
        pr.enable()
    else:
        pr = None

    from . import preprocess
    from . import generic

    content, preproc = preprocess.apply(content)
    content = generic.rewrite_generic(content, opt)
    content = preproc.restore(content)

    if pr:
        pr.disable()
        ps = pstats.Stats(pr, stream=sys.stderr)
        ps.sort_stats('cumulative', 'stdname')
        ps.print_stats(20)

        ps.sort_stats('tottime', 'stdname')
        ps.print_stats(20)

    return dumper.dump_new(content)


def main():
    fr, args, opt = init_as(sys.argv)
    if not fr or not args:
        # Don't use os.execve. That's too low-level for Python and does not deallocate things
        print('WARNING: Falling back to standard as. Command line not understood: ' + (' '.join(sys.argv)),file=sys.stderr)
        args = sys.argv.copy()
        args[0] = find_as()
        sys.exit(subprocess.call(args))

    fout = _rewrite(fr, opt)

    # Invoke as
    ret = subprocess.call(args, close_fds=True, stdin=fout)

    sys.exit(ret)
