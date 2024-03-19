#!/usr/bin/env python3

import fcntl
import os
import re
import subprocess
import sys
import time


def get_make_pid():
    pid = os.getppid()
    while True:
        try:
            exe = os.readlink('/proc/{}/exe'.format(pid))
        except OSError:
            return None
        if os.path.basename(exe) in ('make', 'gmake'):
            return pid
        try:
            with open('/proc/{}/status'.format(pid), 'rb') as f:
                status = f.read()
        except OSError:
            return None
        m = re.search(br'^ppid:\s*(\d+)', status, re.M | re.I)
        if not m:
            return None
        pid = int(m.group(1))


def clear_old_files(dirname):
    threshold = time.time() - 3600
    with os.scandir(dirname) as it:
        for entry in it:
            try:
                if entry.is_file(follow_symlinks=False) and \
                        entry.stat(follow_symlinks=False).st_mtime < threshold:
                    os.unlink(entry.path)
            except OSError:
                pass


def main():
    if len(sys.argv) < 3:
        print('Too few arguments', file=sys.stderr)
        sys.exit(127)

    make_pid = get_make_pid()
    if make_pid is None:
        print('Unable to find a running make session', file=sys.stderr)
        sys.exit(127)

    track_dir = '/tmp/makefile-multiple-output-helper-{}'.format(os.getuid())
    try:
        os.mkdir(track_dir, 0o755)
    except FileExistsError:
        pass

    clear_old_files(track_dir)

    track_file = os.path.join(
        track_dir,
        '{}-{}.tag'.format(make_pid, sys.argv[1].replace('/', '--')))
    try:
        fd = os.open(track_file, os.O_RDWR | os.O_CREAT, 0o600)
    except OSError:
        print('Unable to open file', track_file, file=sys.stderr)
        sys.exit(127)

    fcntl.flock(fd, fcntl.LOCK_EX)
    content = os.read(fd, 128)
    if content:
        sys.exit(int(content))

    ret = subprocess.call(sys.argv[2:])
    os.lseek(fd, 0, os.SEEK_SET)
    os.pwrite(fd, str(ret).encode(), 0)
    sys.exit(ret)


if __name__ == '__main__':
    main()
