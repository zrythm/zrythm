Contributing to Zrythm
======================

Zrythm is a project developed mostly by volunteers
from all around the world. You are welcome to join
us on our mailing lists or in the #zrythm channel in
IRC Freenode. Tell us how would you like to help,
and we will do our best to guide you.

We want to provide a friendly and
harassment-free environment so that anyone can
contribute to the best of their abilities. To this
end our project uses a “Contributor Covenant”, which
was adapted from https://contributor-covenant.org/.
You can find the full pledge in the
[CODE OF CONDUCT](CODE_OF_CONDUCT.md) file.

# Project Management
We use [Sourcehut](https://sr.ht/~alextee/zrythm/)
as the central point for development, maintenance and
issue tracking of Zrythm.

The source files for all the components of the
project, including software, web site, documentation,
and artwork, are available in Git repositories at
[CGit](https://git.zrythm.org/cgit/).

# Art
We are always looking for artists to help us design
and improve user interfaces, and create multimedia
material for documentation, presentations and
promotional items.

# Documentation
You can read the project documentation already
available in the manual and help us identify any
errors or omissions. Creating new manuals,
tutorials, and blog entries will also help users and
developers discover what we do.

# Programming
Source code is in the
[main Git repository](https://git.zrythm.org/cgit/zrythm/).
We use C as the main programming language for the
various components of Zrythm, in addition to
Scheme for writing scripts and Python for the
website.

You will find it useful to read introductory
material about C. Also, make sure to read
[HACKING.md](HACKING.md) for more details on the
development setup, as well as the coding and
cooperation conventions used in the project.

# Test and Bug Reports
Install the software and send feedback to the
community about your experience. You can help the
project by reporting issues.

## Generating Crash Reports
To get more information about a crash that will help
us find the issue and fix it, run Zrythm as follows

    zrythm_launch --gdb

When the program exits, it will create a crash report
inside the `gdb` directory under your Zrythm user
path. Please send this file to us.

# Translation
You can help translate the software, the website
and the manual into your language. Visit
[Weblate](https://hosted.weblate.org/engage/zrythm)
to get started.

Alternatively, you can edit the PO files directly
and submit a patch.

# Other resources for contributors
Documents, supporting material and auxiliary
information useful to hackers and maintainers is
available at <https://docs.zrythm.org>.

----

Copyright (C) 2020 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.

Initially written for the GNU Guix project by
sirgazil who waives all copyright interest.
