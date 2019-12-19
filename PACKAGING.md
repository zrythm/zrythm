Information for Packagers
=========================

# Upstream URLs

You can use
https://www.zrythm.org/releases
to fetch tarballs. The project's home page is
https://www.zrythm.org. The git repositories are
on our [CGit instance](https://git.zrythm.org/cgit/).

# Meson

Zrythm needs a fairly recent version of Meson to build 
(around 0.50 and above).

# Included Programs

For various reasons, Zrythm ships with some libraries/resources
that could be packaged separately. If you wish to package
them separately and make Zrythm use them, you can pass the
flags found in `meson_options.txt`.

# Docs

See the `manpage` and `user_manual` meson options.

# Post-Install Commands

Depending on the distro, some of the
the following commands will need to be run with
appropriate options. Some distros run these
automatically.

    glib-compile-schemas
    fc-cache
    gtk-update-icon-cache
    update-mime-database
    update-desktop-database
    update-gdk-pixbuf-loaders

See `scripts/meson_post_install.py` for more
details.

# Patches

Please send a patch at dev _at_ zrythm.org if something does not build
in a particular scenario and you manged to fix it, or
if you want to add compilation flags. Alternatively,
attach patches as new issues on our
[Redmine instance](https://redmine.zrythm.org/projects/zrythm/issues).

----

Copyright (C) 2019 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
