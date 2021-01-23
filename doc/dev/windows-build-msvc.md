# Building on Windows (MSVC)

> you install VisualStudio community edition, python, git (those 3 are just click click installers) then open "Developer powershell for VS2019" (you should have that in windows apps after installing vs), pip install meson, git clone `your_project`, meson build, ninja -C build
> what are you supposed to do about dependencies btw? what's the general practice?
> Port them all to meson and build them as subproject

> chocolatey also has packages for all those things: https://gitlab.gnome.org/creiter/gitlab-ci-win32-runner-v2/-/blob/master/ansible/setup.yml#L57

# TODO

# Thanks

Thanks to xclaesse and lazka for explanations.

----

Copyright (C) 2021 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
