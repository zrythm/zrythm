# Building on Windows

Install MSYS2 : https://www.msys2.org

Once installed hit start and look for and run `MSYS2 MinGW 64-bit`. This will open a command shell.
For visual studio code terminal integration see [VSCode terminal integration](#vsint)

In msys install all dependencies using the following command:

```
pacman --sync --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-gtk3 mingw-w64-x86_64-meson mingw-w64-x86_64-libsamplerate mingw-w64-x86_64-fftw mingw-w64-x86_64-ffmpeg mingw-w64-x86_64-libyaml mingw-w64-x86_64-libsndfile mingw-w64-x86_64-rubberband mingw-w64-x86_64-dlfcn mingw-w64-x86_64-SDL2 mingw-w64-x86_64-chromaprint guile libguile-devel mingw-w64-x86_64-gtksourceview3 mingw-w64-x86_64-graphviz
```

Let it run until all packages are done installing.
The above will install git as well which is needed for the next step.

### Clone Zrythm

```
cd && mkdir git && cd git
git clone https://git.zrythm.org/git/zrythm && cd zrythm
```

### Clone dependencies
Some dependencies are not yet pulled by meson.

#### Carla

Carla (source code at: https://github.com/falkTX/Carla ) needs two zip:
* https://www.zrythm.org/downloads/carla/carla-64-b082b8b.zip that needs to be extracted in the mysys dir (`start /mingw64/lib` to locate the lib file for example)
* https://www.zrythm.org/downloads/carla/carla-2.1-woe32.zip that needs to be extracted into `/mingw64/lib/carla/`

#### Breeze-icons

* download zip from https://github.com/KDE/breeze-icons/tree/master/icons-dark
* Extract sub folder `icons-dark` in `/mingw64/share/icons`
* `mv /mingw64/share/icons/icons-dark /mingw64/share/icons/breeze-dark`


### Build
Setup project files with meson
```
meson build -Dsdl=enabled -Drtaudio=auto -Drtmidi=auto -Dcarla=auto
```

Compile and finalize
```
ninja -C build install
```

### Run
The working directory must be the install bin, in the msys root directory:
```
/mingw64/bin/zrythm.exe
```
You can locate this exe by executing:
```
start /mingw64/bin
```

Copy whichever dll are missing next to the executable.
CarlaNativePlugin.dll can be found in the official ZRythm Trial version bin folder.

### Debugging ( VSCode )
visual studio code:
In VSCode open the root folder of zrythm. Hit F5 and you will be prompted with a selection of environment options.
Select `C++(GDB/LLDB)`, this will create a new launch.json file with a configuration for gdb.

Edit the following options:

`"program":"${workspaceFolder}/build/src/zrythm.exe"`

`"externalConsole":true`

`"miDebuggerPath":"C:/msys64/mingw64/bin/gdb.exe"`

hit `F5` and the debugger should run.

### VSCode terminal integration
VSCode allows you to change the integrated terminal. With this we can make calls to msys directly from VSCode.

Changing terminal from powershell to bash:

Open settings using `File > Preferences > Settings` or `ctrl+,`
Select `Workspace`, this will make it so that these changes only apply to the current workspace.
Click the "Open settings" button top right (file with arrow icon). This will open the settings as an editable json file.

Add the following lines

```
 "terminal.integrated.shell.windows": "C:\\msys64\\usr\\bin\\bash.exe",
 "terminal.integrated.shellArgs.windows": ["--login", "-i"],
 "terminal.integrated.env.windows": {
    "MSYSTEM": "MINGW64",
     "CHERE_INVOKING":"1"
  }
```

The above snippet assumes that msys64 was installed at the default location `C:/msys64`.
Make sure it matches with your installation path.

To open the terminal hit `ctrl`+`` ` ``

### ASIO

Zrythm uses the RtAudio library on Windows, which
can be built with ASIO support, however,
ASIO is proprietary and Zrythm uses code under the
GPL so distribution of Zrythm with ASIO support is
illegal. For more information on why this is not
allowed by the GPL, consider:

<https://gitlab.com/gnu-clisp/clisp/blob/dd313099db351c90431c1c90332192edce2bb5c9/doc/Why-CLISP-is-under-GPL>

<!---
SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
SPDX-FileCopyrightText: © 2020 Sidar Talei, Matthieu Talbot
SPDX-License-Identifier: FSFAP
-->
