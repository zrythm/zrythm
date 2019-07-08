Contributing Guidelines
=======================

*This file is part of Zrythm and is licensed under the
GNU Affero General Public License version 3. You should have received
a copy of the GNU Affero General Public License
  along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.*

# CODE STRUCTURE

    .
    ├── AUTHORS                      # author information
    ├── autogen.sh                   # creates configure
    ├── config.guess                 # used by configure
    ├── config.sub                   # used by configure
    ├── configure.ac                 # options to create configure
    ├── CONTRIBUTING.md              # contributing guidelines
    ├── COPYING                      # license
    ├── data                         # data files to be installed
    │   ├── org.zrythm.gschema.xml   # GSettings schema
    │   └── zrythm.desktop           # desktop file
    ├── Doxyfile                     # doxygen configuration
    ├── ext                          # external libs
    │   └── audio_decoder            # audio decoder by rgareus
    ├── inc                          # include dir
    │   ├── actions                  # actions (undo, quit, etc.)
    │   ├── audio                    # audio-related headers
    │   ├── gui                      # gui-related headers
    │   │   ├── backend              # backend for serialization
    │   │   └── widgets              # gui widgets
    │   ├── plugins                  # headers forplugin hundling
    │   ├── settings                 # settings-related headers
    │   ├── utils                    # various utils
    ├── install-sh                   # needed when installing
    ├── Makefile.in                  # custom makefile
    ├── PKGBUILD.in                  # arch linux package config
    ├── README.md                    # main README file
    ├── resources                    # non-code resources
    │   ├── fonts                    # fonts
    │   ├── icons                    # icons
    │   ├── theme                    # GTK theme
    │   └── ui                       # GTK ui files for widgets
    ├── scripts                      # various scripts
    ├── src                          # source (.c) files
    │   ├── actions                  # actions (undo, quit, etc.)
    │   ├── audio                    # audio-related code
    │   ├── gui                      # gui-related code
    │   │   ├── backend              # backend for serialization
    │   │   └── widgets              # gui widgets
    │   ├── main.c                   # main
    │   ├── plugins                  # plugin handling code
    │   │   ├── lv2                  # lv2 plugin handling code
    │   │   │   └── zix              # lv2 utilities
    │   ├── settings                 # settings-related code
    │   ├── utils                    # various utils
    └── THANKS                       # thank you notice

# DEPENDENCIES
## Required
TODO make this a table (name|arch pkg name|license|upstream URL|use)
- GTK+3 (library GPLv2+): https://gitlab.gnome.org/GNOME/gtk
- jack (LGPLv2.1+): http://jackaudio.org/
- lv2 (ISC): http://lv2plug.in/
- lilv (ISC): https://drobilla.net/software/lilv
- libsndfile (LGPLv3): http://www.mega-nerd.com/libsndfile
- libyaml
- libsamplerate (2-clause BSD): http://www.mega-nerd.com/libsamplerate

## Optional
- portaudio (MIT): www.portaudio.com/
- ffmpeg (LGPL 2.1+, GPL 2+): https://ffmpeg.org/
- Qt5

Note: optional dependencies are turned on using
`--with-***`. See `./configure --help`

# BUILDING
The project uses meson, so the steps are

    meson _build
    ninja -C _build
    meson install -C _build # optional

Once the program is built, it will need to be installed the first time before it can run (to install the GSettings)

Alternatively if you don't want to install anything on your system you can run `glib-compile-schemas data/` and then run zrythm using `GSETTINGS_SCHEMA_DIR=data ./_build/src/zrythm`. The built program will be at `_build/src/zrythm` by default

## Non-standard locations

When installing in non-standard locations, glib
needs to find the gsettings schema. By default,
it looks in /usr and /usr/share.
It is possible to set
the `GSETTINGS_SCHEMA_DIR` environment variable to
`<your prefix>/share/glib-2.0/schemas` or prepend
`XDG_DATA_DIRS` with `<your prefix>/share` before
running `<your prefix>/bin/zrythm` to make glib
use the schema installed in the custom location.

There are also translations installed in the custom
location so XDG_DATA_DIRS might be a better idea.

Generally, we recommend installing under /usr or
/usr/local (default) to avoid these problems.

# DEBUGGING
  Use `gdb _build/src/zrythm`

  Can also do `G_DEBUG=fatal_warnings,signals,actions G_ENABLE_DIAGNOSTIC=1 gdb _build/src/zrythm`. G_DEBUG will trigger break points at GTK warning messages and  G_ENABLE_DIAGNOSTIC is used to break at deprecation  warnings for GTK+3 (must all be fixed before porting to GTK+4).

# TESTS / COVERAGE
  To run the test suite, use `meson _build -Denable_tests`
  followed by `meson test -C _build`.

  To get a coverage report use `meson _build -Denable_tests=true -Denable_coverage=true`, followed by
  the test command above, followed by
  `gcovr -r .` or `gcovr -r . --html -o coverage.html` for html output.

# PACKAGING
  See the [README](git-packaging-hooks/README.md) in git-packaging-hooks and the `packaging` branch

# COMMENTING
  - Document how to use the function in the header file
  - Document how the function works (if it's not obvious from the code) in the source file
  - Document each logical block within a function (what it does so one can understand what the function does at each step by only reading the comments)

# CODING STYLE
  GNU style is preferable

# INCLUDE ORDER
standard C library headers go first in alphabetic
order, then local headers in alphabetic order, then
GTK or glib headers, then all other headers in
alphabetic order

# LINE LENGTH
Please keep lines within 60 characters

# LICENSING
If you contributed significant (for copyright purposes)
amounts of code, you may add your copyright notice
(name, year and optionally email/website) at the top
of the file, otherwise you agree to assign all
copyrights to Alexandros Theodotou, the main author.
You agree that all your changes will be licensed under
the AGPLv3+ like the rest of the project.

# TROUBLESHOOTING
## Getting random GUI related errors with no trace in valgrind or GTK warnings
Calling GTK code or g_idle_add from non-GUI threads?
## GUI lagging
Using g_idle_add without returning false in the GSourceFunc? Not using g_idle_add?

# OTHER
  1. Backend files should have a _new() and a _init() function. The _new is used to create the object and the _init is used to initialize it. This is so that objects can be created somewhere and initialized somewhere else, for whatever reason.
  2. GUI widgets that exist no matter what (like the tracklist, mixer, browser, etc.) should be initialized by the UI files with no _new function. Widgets that are created dynamically like channels and tracks should have a _new function. In both cases they should have a _setup function to initialize them with data and a _refresh function to update the widget to reflect the backend data. _setup must only be called once and _refresh can be called any time, but excessive calling should be avoided on widgets with many children such as the mixer and tracklist to avoid performance issues.

  Anything else just ask in the chatrooms!
