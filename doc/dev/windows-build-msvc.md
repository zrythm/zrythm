<!---
SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

# Building on Windows (MSVC)

> you install VisualStudio community edition, python, git (those 3 are just click click installers) then open "Developer powershell for VS2019" (you should have that in windows apps after installing vs), pip install meson, git clone `your_project`, meson build, ninja -C build
> what are you supposed to do about dependencies btw? what's the general practice?
> Port them all to meson and build them as subproject

> chocolatey also has packages for all those things: https://gitlab.gnome.org/creiter/gitlab-ci-win32-runner-v2/-/blob/master/ansible/setup.yml#L57
> nice! does meson in the vs command prompt see the chocolatey packages by default?
> If you have a pkgconfig from chocolatey, yes.

1. Install as admin https://chocolatey.org/install
2. choco install sed pkgconfiglite gnuwin32-coreutils.install grep
3. https://sourceforge.net/projects/ezwinports/files/guile-2.0.11-2-w32-bin.zip/download (doesn't work)

# Other programs
- https://sourceforge.net/projects/gnuwin32/files/
- gettext: https://sourceforge.net/projects/gnuwin32/files/gettext/0.14.4/gettext-0.14.4.exe/download?use_mirror=kumisystems&download=

# Running a command
` & 'C:\Program Files (x86)\GnuWin32\bin\cat.exe' .\README.md`

# TODO
See https://www.collabora.com/news-and-blog/blog/2021/03/18/build-and-run-gtk-4-applications-with-visual-studio/
for building gtk4 on Windows.

# Thanks

Thanks to Company, xclaesse, dcbaker and lazka for explanations.
