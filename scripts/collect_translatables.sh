#!/usr/bin/env python3

import os
import os.path
import sys

for path in ['inc', 'src', 'resources']:
  for dirpath, dirnames, filenames in os.walk(path):
    for filename in [f for f in filenames if f.endswith(".c") or f.endswith(".h") or f.endswith(".ui")]:
      print(os.path.join(dirpath, filename))
