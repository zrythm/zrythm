Information for Packagers
=========================

We are already providing homemade packages for various
distros on [OBS (Open Build System)](https://build.opensuse.org/package/show/home:alextee/zrythm#), so feel free to
use these as a base.

# Meson

Zrythm needs a fairly recent version of Meson to build.
We don't know the exact version number but something around 45
and above should be fine. The one in Debian Stretch
will not work, but the one in stretch-backports will
work.

# Manual

The documentation in this distribution is source code
documentation for developers only. The user manual is
maintained as a [separate project](https://git.zrythm.org/cgit/zrythm-docs/), so we would recommend having
a separate package for it and adding it as a
dependency. Example:

    zrythm
    └── zrythm-man

# Patches

Please send a patch if something does not build
in a particular scenario and you manged to fix it, or
if you want to add compilation flags.

----

Copyright (C) 2019 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
