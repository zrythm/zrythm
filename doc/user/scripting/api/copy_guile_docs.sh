#!@BASH@
# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# this just copies the rst files (created by meson
# in the main build dir because you can only have
# outputs in the dir corresponding to the source dir)
# to where they need to be (scripting/api/*.rst)
echo "copying from @GUILE_DOCS_SRCDIR@ to @GUILE_DOCS_DESTDIR@"
cp -f @GUILE_DOCS_SRCDIR@/*.rst @GUILE_DOCS_DESTDIR@/
