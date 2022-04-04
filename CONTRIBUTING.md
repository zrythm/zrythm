<!---
SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP

Initially written for the GNU Guix project by
sirgazil who waives all copyright interest.
-->

Contributing to Zrythm
======================

Zrythm is a project developed mostly by volunteers
from all around the world. You are welcome to join
us on our mailing lists or in the #zrythm channel in
Libera.chat IRC. Tell us how would you like to
help, and we will do our best to guide you.

# Project Management
We use [Sourcehut](https://sr.ht/~alextee/zrythm/)
as the central point for development, maintenance and
issue tracking of Zrythm.

The source files for all the components of the
project, including software, web site, documentation,
and artwork, are available in our
[git repositories](https://git.zrythm.org/zrythm).

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
developers discover Zrythm.

# Programming
Source code is in the
[main Git repository](https://git.zrythm.org/zrythm/zrythm/).
We use C as the main programming language for the
various components of Zrythm, in addition to
Scheme for writing scripts and Python for the
website.

You will find it useful to read introductory
material about C. A C primer is available at
<https://www.enlightenment.org/docs/c/start>.
Also, make sure to read
[HACKING.md](HACKING.md) for more details on the
development setup, as well as the coding and
cooperation conventions used in the project.

## Submitting Patches
We prefer code contributions in the form of patches.
Use `git format-patch` to generate patch files and
send them to the
[development mailing list](https://lists.sr.ht/~alextee/zrythm-devel).
You can also use
[git-send-email](https://git-send-email.io/) for this.
SourceHut also has a web UI to make submitting
patches from your own repositories easier. See
[this video tutorial](https://spacepub.space/videos/watch/ad258d23-0ac6-488c-83fc-2bacf578de3a)
for reference.
If you are having difficulties creating patches
please contact us and we will guide you.

The Zrythm project uses a Contributor Ceritificate of
Origin, which is a mechanism similar to the
[Developer Certificate of Origin (DCO)](https://developercertificate.org/) to manage contributions.
The Contributor Certificate of Origin is an
affirmation that you are the creator of your
contribution, and that you wish to allow the Zrythm
project to use your work.

Acknowledgement of this permission is done using a
sign-off process in Git. The sign-off is a simple
line at the end of the explanation for the patch.
The Contributor Certificate of Origin can be found
in the
[CONTRIBUTOR_CERTIFICATE_OF_ORIGIN](CONTRIBUTOR_CERTIFICATE_OF_ORIGIN)
file at the root of this distribution.

If you are willing to agree to these terms, please
add a line to every git commit message:

    Signed-off-by: Joe Smith <joe.smith@email.com>

If you set your user.name and user.email as part of
your git configuration, you can sign your commit
automatically with `git commit -s`.

You must use your real name (i.e., pseudonyms or
anonymous contributions cannot be made).

# Test and Bug Reports
Install the software and send feedback to the
community about your experience. You can help the
project by reporting issues.

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
