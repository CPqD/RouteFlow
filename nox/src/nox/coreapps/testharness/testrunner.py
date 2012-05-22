#!/usr/bin/env python
#
# Small script to ensure all test subprocesses have a common process
# group unique to that set of processes so we can kill anything that
# doesn't terminate properly.

import os
import sys


os.setpgid(0,0)
pid = os.fork()
if pid == 0:
    # child
    os.execl(sys.argv[1], sys.argv[1])
else:
    # parent
    pid, status = os.wait()
    if status & 0xff == 0:
        status = status >> 8
    else:
        # We were terminated a signal, must have been an error
        status = 1
    sys.exit(status)
