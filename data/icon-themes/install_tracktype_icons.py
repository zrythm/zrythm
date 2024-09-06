#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

import os
import shutil
import sys

# These variables would be replaced by the build system
MESON_INSTALL_DESTDIR_PREFIX = os.environ.get('MESON_INSTALL_DESTDIR_PREFIX', '')
TRACK_TYPES_DIR = sys.argv[1]
CMD1 = sys.argv[2]  # 'cp' or 'ln'

tracktypes_dir = os.path.join(MESON_INSTALL_DESTDIR_PREFIX, TRACK_TYPES_DIR)

icons = [
    "actions/bus",
    "status/bars",
    "status/signal-audio",
    "status/signal-midi"
]

os.makedirs(tracktypes_dir, exist_ok=True)
os.chdir(tracktypes_dir)

for icon in icons:
    source = f"../{icon}.svg"
    destination = "./"
    
    if CMD1 == 'cp':
        shutil.copy2(source, destination)
    elif CMD1 == 'ln':
        os.symlink(source, os.path.join(destination, os.path.basename(source)))
    else:
        print(f"Unknown command: {CMD1}", file=sys.stderr)
        sys.exit(1)

print("Icon installation completed successfully.")
