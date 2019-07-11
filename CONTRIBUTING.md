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
