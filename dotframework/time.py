#!/usr/bin/env python3
import time
import os
import sys

time1 = time.time()
os.system(' '.join(sys.argv[1:]))
time2 = time.time()
print("Run took", time2-time1, "seconds")