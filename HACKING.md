Hacking Zrythm
==============

# Code Structure

    .
    ├── AUTHORS                           # Author information
    ├── CHANGELOG.md                      # Changelog
    ├── CONTRIBUTING.md                   # Contributing guidelines
    ├── CONTRIBUTOR_CERTIFICATE_OF_ORIGIN # Certificate for contributing
    ├── COPYING                           # Main license
    ├── COPYING.*                         # Other licenses
    ├── INSTALL.rst                       # Installation instructions
    ├── PACKAGING.md                      # Information for packagers
    ├── README.md                         # Main README file
    ├── THANKS                            # Thanks notice
    ├── TRADEMARKS.md                     # Trademark policy
    ├── TRANSLATORS                       # List of translators
    ├── VERSION                           # Version file
    ├── build                             # temporary build dir
    ├── data                              # Data files to be installed
    │   ├── css-themes                    # UI CSS themes
    │   ├── fonts                         # Fonts
    │   ├── gtksourceview-*.xml           # GtkSourceView themes
    │   ├── icon-themes                   # Icon themes
    │   ├── org.zrythm.Zrythm-mime.xml    # Custom MIME types
    │   ├── samples                       # Audio samples
    │   ├── scripts                       # Built-in Guile scripts
    │   ├── windows                       # Files used on windows builds
    │   ├── zrythm.desktop.in             # Desktop file
    │   ├── zrythm_launch.in              # Launcher script
    │   └── zrythm_*.in                   # Other scripts
    ├── doc                               # Documentation
    │   ├── dev                           # Developer docs
    │   ├── man                           # Manpage
    │   └── user                          # User manual
    ├── ext                               # External libs
    │   ├── midilib                       # MIDI file serialization
    │   ├── nanovg                        # OpenGL drawing
    │   ├── sh-manpage-completions        # Shell completion generator
    │   ├── weakjack                      # Weakly-linked JACK
    │   ├── whereami                      # Get executable path
    │   ├── zita-resampler                # Real-time resampler
    │   └── zix                           # Data struct utils
    ├── git-packaging-hooks               # Git hooks for packaging
    ├── inc                               # Include dir
    │   ├── actions                       # Actions (undo, quit, etc.)
    │   ├── audio                         # Audio-related headers
    │   ├── gui                           # Gui-related headers
    │   │   ├── backend                   # Backend for serialization
    │   │   └── widgets                   # Gui widgets
    │   ├── guile                         # Guile scripting interface
    │   ├── plugins                       # Plugin handling
    │   │   ├── carla                     # Carla plugin handling
    │   │   └── lv2                       # LV2 plugin handling
    │   ├── schemas                       # Struct schema history
    │   ├── settings                      # Settings
    │   ├── utils                         # Various utils
    │   └── zrythm.h                      # Main zrythm struct
    ├── meson.build                       # Meson conf
    ├── meson.options                     # Meson options
    ├── po                                # I18n
    ├── resources                         # Bundled resources
    │   ├── gl                            # OpenGL
    │   │   └── shaders                   # Shaders
    │   ├── gtk                           # Standard GTK resources
    │   ├── icons                         # Icons
    │   ├── theme                         # Parent GTK theme
    │   └── ui                            # GTK ui files for widgets
    ├── scripts                           # Various scripts
    ├── src                               # Source (.c) counterparts of inc
    ├── subprojects                       # Subprojects to auto-fetch if some dependencies are not found
    ├── tests                             # Unit & integration tests
    │   └── helpers                       # Test helpers
    └── tools                             # Various tools

# Debugging
After installing Zrythm once, for example in
`~/.local`, use

    G_MESSAGES_DEBUG=zrythm G_DEBUG=fatal_criticals GSETTINGS_SCHEMA_DIR=~/.local/share/glib-2.0/schemas GDK_SYNCHRONIZE=1 ZRYTHM_DEBUG=1 gdb -ex "handle SIG32 noprint nostop" -ex run --args build/src/zrythm

Command-line options can be passed at the end. For
example,

    ... -ex run --args build/src/zrythm --dummy

- `G_MESSAGES_DEBUG=zrythm` will enable debug
messages.
- `G_DEBUG=fatal_criticals` will trigger
breakpoints at critical messages. `fatal_warnings`
is similar and also includes warnings.
- `GSETTINGS_SCHEMA_DIR` specifies the path to look
for GSettings schemas. Zrythm will install its
schema under `share/glib-2.0/schemas` in the install
prefix.
- `GDK_SYNCHRONIZE=1` will make all X requests
synchronously, which can be useful for UI debugging,
but it will slow down performance.
- `ZRYTHM_DEBUG=1` behaves like a "developer mode" -
it shows more debug info in various elements in
the UI.

An easy way to test something is to add a
`G_BREAKPOINT ();`.

Below is a GDB cheatsheet. For more information
about using GDB, see `man gdb`.
- `r`: run the program
- `c`: continue execution
- `q`: quit
- `bt`: print backtrace
- `up`: go up in the call stack
- `down`: go down in the call stack
- `p myvar`: print variable `myvar`
- `call (void) myfunc (arg1, arg2)`: call function
`myfunc`, which takes in 2 arguments and returns
void

For more information on warnings and assertions see
<https://developer.gnome.org/glib/stable/glib-Warnings-and-Assertions.html>.

Also see [Running GLib Applications](https://developer.gnome.org/glib/stable/glib-running.html).

We suggest building with `-Dextra_debug_info` which
essentially adds `-g3` to the CFLAGS. This allows
use of C macros in gdb so you can do `p TRACKLIST`
instead of `p zrythm->project->tracklist`. This may
slow down gdb on some systems.

Use `MALLOC_CHECK_=3` to enforce additional checks
when allocating memory. This is used during tests.
See `man malloc` for details.

## Verifying Compiler Flags
Zrythm is built with `-frecord-gcc-switches` by
default, which stores the gcc flags used in each
compiled object. To view these flags, use

    readelf -p .GCC.command.line build/src/zrythm

*TODO add --gcc-switches option and use libelf to
print this info*

## Environment Variables
In addition to GTK/GLib variables, Zrythm
understands the following environment variables.
- `ZRYTHM_DSP_THREADS` - number of threads
  to use for DSP, including the main one
- `ZRYTHM_SKIP_PLUGIN_SCAN` - disable plugin scanning
- `ZRYTHM_DEBUG` - shows additional debug info about
  objects

**FIXME** these are not up to date. link to user
manual or point to the manpage

## UI Debugging
[GTK inspector](https://wiki.gnome.org/action/show/Projects/GTK/Inspector)
can be used to inspect UI widgets and change their properties
live (while the application is running).

This is useful when looking to try out different
widget properties to see what works before
implementing them. Adding custom CSS is also possible.
For example, the following CSS can be entered live
to color all the track backgrounds red.

    track {
      background-color: red;
    }

# Tests and Coverage
To run the test suite, use

    meson build -Dtests=true

followed by

    meson test -C build

To run a specific test with gdb use

    meson test -C build --gdb actions_arranger_selections

To get a coverage report see
<https://mesonbuild.com/howtox.html#producing-a-coverage-report>.

# Profiling
## gprof
To profile with gprof,
use the `profiling` option when running meson,
then build and run the program normally. The program
must end gracefully (ie, not Ctrl-C). When the
program ends, run

    gprof build/src/zrythm > results

and check the results file for the profiling results.

*If you are running Zrythm using `zrythm_launch`,
use the path of the `zrythm` binary in the same
directory.*

## Callgrind
Alternatively, you can use callgrind with kcachegrind.
Build Zrythm normally and then run it through
callgrind as follows:

    tools/run_callgrind.sh build/src/zrythm

When you are finished, close Zrythm and run
`kcachegrind` in the same directory to display the
profiling info in the kcachegrind GUI.
For more information, see
<https://docs.kde.org/stable5/en/kdesdk/kcachegrind/using-kcachegrind.html>.

# Memory usage
To debug memory usage, the
[massif](https://valgrind.org/docs/manual/ms-manual.html)
tool from valgrind can be used. To use on the
`actions/mixer selections action` test for example,
use

    meson test --timeout-multiplier=0 -C build --wrap="valgrind --tool=massif --num-callers=160 --suppressions=$(pwd)/tools/vg.sup" actions_mixer_selections_action

This will create a file called `massif.out.<pid>`
that contains memory snapshots in the `build`
directory. This file can be opened with
[Massif-Visualizer](https://apps.kde.org/en-gb/massif-visualizer/)
for inspection.

If the test takes too long, it can be stopped with
`SIGTERM` and results will be collected until
termination.

*Note: massif runs the program 20 times slower*

Alternatively, valgrind's leak check can be used

    meson test --timeout-multiplier=0 -C build --wrap="valgrind --num-callers=160 --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --suppressions=$(pwd)/tools/vg.sup" actions_mixer_selections_action

# Real-time safety
Use the following to get a stoat report.

    CC=stoat-compile CXX=stoat-compile++ meson build
    ninja -C build run_stoat

Realtime functions can be annotated as REALTIME or
specified in
[tools/stoat_whitelist.txt](tools/stoat_whitelist.txt).
Non-realtime functions or functions that should not
be run in realtime threads can be specified in
[tools/stoat_blacklist.txt](tools/stoat_blacklist.txt).
Suppressions (functions to ignore) are found in
[tools/stoat_suppressions.txt](tools/stoat_suppressions.txt).

For more info see
<https://github.com/fundamental/stoat>.

# Cppcheck

    ninja -C build cppcheck

# Collecting Translations
To collect all translatable filenames into
`po/POTFILES`, generate the POT file and update
the PO files, use

    ninja -C build collect-translatables zrythm-pot zrythm-update-po

# Creating Releases
Releases are created using git hooks.
See the [README](git-packaging-hooks/README.md) in
git-packaging-hooks for more information.

# Merging Pull Requests

    git remote add <name> <fork url>
    git fetch <name>
    git merge <name>/<branch-name>

# Coding Guidelines
## Commenting
Please document everything specified in header files
using Doxygen tags. At a bare minimum, every function
declaration should have a brief description.

It is extremely helpful if you document each logical
block within a function using plain comments, in a
way that one can understand what is happening in
each step by only reading the comments.

## Coding Style
Generally, try to follow the
[GNU coding standards](https://www.gnu.org/prep/standards/html_node/Writing-C.html).

TODO: prepare a clang-format config or similar.

## Include Order
In alphabetic order:
1. Config headers
2. Standard C library headers
3. Local headers
4. GTK and GLib headers
5. Any other headers

## Line Length
We keep lines within 60 characters. This works
nicely with 4 columns of files open simultaneously
in a tiled editor, but if you find this
difficult to work with < 80 characters is acceptable
too.

## Error Handling
To report invalid program states (such as programming
errors), use the `g_return_*`/`z_return_*` functions or
`g_critical`. This will show a bug report popup
if hit.

To report non-programming errors, such as failure
to open a file, use the
[GError](https://www.freedesktop.org/software/gstreamer-sdk/data/docs/latest/glib/glib-Error-Reporting.html) mechanism.

## Function attributes
Tips
* `inline` small functions that are called very
often
* `CONST`/`PURE` can be combined with inline
* `HOT` should not be combined with inline
* only use `CONST`/`PURE` on functions that do not log (or use `g_warn_*()`/`g_return_*()`)

## Licensing
If you contributed significant (for copyright
purposes) amounts of code in a file, you should
append your copyright notice (name, year and
optionally email/website) at the top.

See also [CONTRIBUTING.md](CONTRIBUTING.md).

## Commit Messages
Please follow the 50/72 rule:
* commit subject within 50 characters
* commit body (optional) wrapped at 72 characters

See `git log` for examples.

We are considering switching to a format that
resembles the
[GNU ChangeLog style](https://www.gnu.org/prep/standards/html_node/Style-of-Change-Logs.html#Style-of-Change-Logs).

# Troubleshooting

*TODO: move this section to the dev docs.*

## Getting random GUI related errors with no trace in valgrind or GTK warnings
This might happen when calling GTK code or
`g_idle_add()` from non-GUI threads. GTK runs in a
single GUI thread. Any GTK-related code must be run
from the GUI thread only.

## Values not being read properly at specific parts of the code (e.g. getting a zero value when we know the variable is non-zero)
Delete the build dir and reconfigure. This is likely
an optimization problem with meson/ninja that
appears rarely.

## Widget not receiving keyboard input/focus
For a widget to receive keyboard input/focus, every
single parent in the hierarchy must have
can-focus=true (default).

----

Copyright (C) 2018-2022 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
