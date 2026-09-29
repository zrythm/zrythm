#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# Exports all vendored conan-center-index recipes from ext/conan-center-index
# into the local Conan cache. Run this after `conan config install conan` and
# before `conan install` or `conan lock create`.
#
# Usage:
#   python3 tools/export_conan_recipes.py

import subprocess
import sys

# (recipe path relative to repo root, version)
recipes = [
    ("ext/conan-center-index/recipes/qt/6.x.x/conanfile.py", "6.11.1"),
    ("ext/conan-center-index/recipes/mpg123/all/conanfile.py", "1.33.0"),
    ("ext/conan-center-index/recipes/libjpeg-turbo/all/conanfile.py", "3.1.4.1"),
    ("ext/conan-center-index/recipes/wayland/all/conanfile.py", "1.22.0"),
    ("ext/conan-center-index/recipes/xkbcommon/all/conanfile.py", "1.5.0"),
    ("ext/conan-center-index/recipes/zix/all/conanfile.py", "0.8.2"),
    ("ext/conan-center-index/recipes/lv2/all/conanfile.py", "1.18.10"),
    ("ext/conan-center-index/recipes/serd/all/conanfile.py", "0.32.10"),
    ("ext/conan-center-index/recipes/sord/all/conanfile.py", "0.16.22"),
    ("ext/conan-center-index/recipes/sratom/all/conanfile.py", "0.6.22"),
    ("ext/conan-center-index/recipes/lilv/all/conanfile.py", "0.28.0"),
]


def main():
    for path, version in recipes:
        subprocess.run(
            ["conan", "export", path, f"--version={version}"],
            check=True,
        )


if __name__ == "__main__":
    sys.exit(main())
