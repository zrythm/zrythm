#!/usr/bin/env python3

import os
import os.path
import sys

for dirpath, dirnames, filenames in os.walk(sys.argv[1]):
  for filename in [f for f in filenames if f.endswith(".c") or f.endswith(".h") or f.endswith(".ui")]:
    print(os.path.join(dirpath, filename))
