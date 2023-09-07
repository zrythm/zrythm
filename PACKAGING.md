<!---
SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

Information for Packagers
=========================

# Upstream URLs

You can use <https://www.zrythm.org/releases> to fetch
tarballs. The project's home page is <https://www.zrythm.org>.
The main git repositories for packagers and the general public
are on [GitHub](https://github.com/zrythm/zrythm).

# Versioning

Zrythm follows the following
[Semantic Versioning (SemVer) 2.0.0](https://semver.org/)
scheme:

```yaml
---
API changes: project file/directory, user directory and settings changes
MAJOR: Manually incremented for breaking API changes
MINOR: Manually incremented for non-breaking API changes
PATCH: Automatically incremented based on number of commits since ${MAJOR}.${MINOR}.0

release tags:
  pre-alpha:
    description: Starts when work on a new major version starts
    identifier: ${unreleased major ver}.0.0-DEV.${MAJOR}.${MINOR}.{PATCH}
    examples: 2.0.0-DEV.0.1.0, 2.0.0-DEV.0.32.1
  alpha:
    description: Starts when a major version under development is somewhat usable
    identifier: ${unreleased major ver}.0.0-alpha.${MAJOR}.${MINOR}.{PATCH}
    examples: 2.0.0-alpha.0.1.0, 2.0.0-alpha.0.32.1
  beta:
    description: Starts when all features expected to break API are implemented
    identifier: ${unreleased major ver}.0.0-beta.${MAJOR}.${MINOR}.{PATCH}
    examples: 2.0.0-beta.0.1.0, 2.0.0-beta.0.32.1
  rc:
    description: Starts when all features are implemented and all known bugs are fixed
    identifier: ${unreleased major ver}.0.0-rc.${manually incremented for each release}
    examples: 2.0.0-rc.0, 2.0.0-rc.32
  v1, 2, 3, etc.:
    description: Starts when all features for the major ver are stable
    identifier: ${MAJOR}.${MINOR}.${PATCH}
    examples: 2.0.0, 2.32.1

nightlies:
  - ${release tag}+r${number of commits since last tag}.g${commit hash}
```

# Included Programs

For various reasons, Zrythm ships with some libraries/resources
that could be packaged separately. If you wish to package them
separately and make Zrythm use them, you can pass the flags
found in `meson_options.txt`.

# Debug Symbols

Please do not strip symbols to assist with meaningful stack
traces which are sent with bug reports.

# Docs

See the `manpage` and `user_manual` meson options.

# Post-Install Commands

Depending on the distro, some of the following commands will
need to be run with appropriate options. Some distros run these
automatically.

    glib-compile-schemas
    fc-cache
    gtk-update-icon-cache
    update-mime-database
    update-desktop-database
    update-gdk-pixbuf-loaders

See the
[post-install script](scripts/meson-post-install.sh)
for more details.

# Trademarks
As mentioned in the
[Trademark Policy](TRADEMARKS.md),
if you wish to distribute modified versions of Zrythm, you
must either get permission or replace the name and logo.

To replace the trademarked name and logo, you can use the
`program_name` and `custom_logo_and_splash` meson options,
after replacing all files inside
`data/icon-themes/zrythm-dark/scalable/apps`. There may be more
things that need to be changed that we missed. If you find any,
please let us know.
