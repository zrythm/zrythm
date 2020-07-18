Information for Packagers
=========================

# Upstream URLs

You can use
https://www.zrythm.org/releases
to fetch tarballs. The project's home page is
https://www.zrythm.org. The git repositories are
on our [CGit instance](https://git.zrythm.org/cgit/).

# Included Programs

For various reasons, Zrythm ships with some libraries/resources
that could be packaged separately. If you wish to package
them separately and make Zrythm use them, you can pass the
flags found in `meson_options.txt`.

# Debug Symbols

Please do not strip symbols to assist with
meaningful stack traces which are sent with bug
reports.

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

See the [post-install script](scripts/meson-post-install.scm)
for more details.

# Trademarks
As mentioned in the
[Trademark Policy](TRADEMARKS.md),
if you wish to distribute modified versions of
Zrythm, you must either get permission or replace
the name and logo.

To replace the trademarked name and logo, you can use
the `program_name` and `custom_logo_and_splash` meson
options, after replacing all files inside
`data/icon-themes/zrythm-dark/scalable/apps`.

# Issues
Any issues should be reported to
[our issue tracker](https://redmine.zrythm.org/projects/zrythm).
If you have patches, please attach them to a new
issue.

----

Copyright (C) 2019-2020 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
