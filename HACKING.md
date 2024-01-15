<!---
SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

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
    │   ├── dsp                           # DSP-related headers
    │   ├── gui                           # GUI-related headers
    │   │   ├── backend                   # Backend for serialization
    │   │   └── widgets                   # GUI widgets
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
For debugging or development, we recommend building
with the following options:

    -Db_lto=false -Dstrict_flags=true -Ddebug=true -Doptimization=0 -Dtests=true -Db_sanitize=address

After installing Zrythm once, for example in `~/.local`, use

    G_MESSAGES_DEBUG=zrythm G_DEBUG=fatal_criticals GSETTINGS_SCHEMA_DIR=~/.local/share/glib-2.0/schemas GDK_SYNCHRONIZE=1 ZRYTHM_DEBUG=1 gdb -ex "handle SIG32 noprint nostop" -ex run --args build/src/zrythm

Command-line options can be passed at the end. For example,

    ... -ex run --args build/src/zrythm --dummy

- `G_MESSAGES_DEBUG=zrythm` will enable debug messages.
- `G_DEBUG=fatal_criticals` will trigger breakpoints at
critical messages. `fatal_warnings` is similar and also
includes warnings.
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

### Valgrind

To debug memory errors (invalid read/writes), use Valgrind
as follows:

    valgrind --num-callers=30 --log-file=valog --suppressions=tools/vg.sup --vgdb-error=1 build/src/zrythm

The same environment variables mentioned above should be passed
as appropriate before the command.

Explanation for options:
- `--num-callers=30` makes valgrind show enough functions in the call stack
- `--log-file=valog` makes valgrind log into a file instead of in the console (optional)
- `--suppressions=tools/vg.sup` tells valgrind to ignore errors that are outside Zrythm's control and false positives
- `--vgdb-error=1` will make valgrind pause execution on the first error and allow gdb to be attached to it (valgrind will print instructions)

## Verifying Compiler Flags
Zrythm is built with `-frecord-gcc-switches` by
default, which stores the gcc flags used in each
compiled object. To view these flags, use

    readelf -p .GCC.command.line build/src/zrythm

*TODO add --gcc-switches option and use libelf to
print this info*

## Environment Variables
In addition to GTK/GLib variables, Zrythm
understands the environment variables mentioned
in the
[Environment Variables section in the user manual](https://manual.zrythm.org/en/appendix/environment.html).

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

## perf
To profile with gprof (recommended), set optimization to 2 and debug to true
and run:

    perf record --event=cache-references,cache-misses,cycles,instructions,branches,faults,migrations,cpu-clock --stat build/src/zrythm --dummy

Then examine the report with:

    perf report --input=perf.data --hide-unresolved

See also:
* <http://sandsoftwaresound.net/perf/perf-tutorial-hot-spots/>

## gprof
To profile with gprof,
use the `profiling` option when running meson,
then build and run the program normally. The program
must end gracefully (ie, not Ctrl-C). When the
program ends, run

    gprof --flat-profile --annotated-source -B --exec-counts --directory-path="$pwd)" --print-path --graph --table-length=16 --function-ordering --min-count=100 build/src/zrythm > results

and check the results file for the profiling results.

[gprof2dot](https://github.com/jrfonseca/gprof2dot)
along with
[xdot.py](https://github.com/jrfonseca/xdot.py) can
be used for a graphical representation of the
results.

gprof has the advantage of being fast (the program runs at normal speed).

*If you are running Zrythm using `zrythm_launch`, use the path of the `zrythm`
binary in the same directory.*

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

*Note: callgrind can slow down the program
considerably*

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

# Adding New Languages

    msginit -i ./po/zrythm.pot -l 'ca_ES.UTF-8' -o ./po/ca.po
    msginit -i ./build/doc/user/_build/gettext/zrythm-manual.pot -l 'ca_ES.UTF-8' -o ./doc/user/locale/ca/LC_MESSAGES/zrythm-manual.po
    msginit -i ./build/doc/user/_build/gettext/sphinx.pot -l 'ca_ES.UTF-8' -o ./doc/user/locale/ca/LC_MESSAGES/sphinx.po

# Creating Releases
Releases are created using git hooks.
See the [README](git-packaging-hooks/README.md) in
git-packaging-hooks for more information.

# Merging Pull Requests

    git remote add <name> <fork url>
    git fetch <name>
    git merge <name>/<branch-name>

# Building User and Dev Docs

    # Create a virtual environment (if not already created)
    # This will create a new directory called venv
    # Skip this step if an environment is already there
    python -m venv venv

    # Enable the virtual environment
    . ./venv/bin/activate

    # Install required python libraries in the environment
    install -r requirements.txt

    # Build the user docs (bundled manual)
    ninja -C build manual_bundle

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
We use a custom coding style based on the
[GNU coding standards](https://www.gnu.org/prep/standards/html_node/Writing-C.html).

There is a
[clang-format target](https://mesonbuild.com/Code-formatting.html#clangformat)
provided. Please run `ninja -C build clang-format`
before submitting patches and it will auto-format
the code. `ninja -C build clang-format-check`
should pass for a patch to be accepted.

## Error Handling
To report invalid program states (such as programming
errors), use the `g_return_*`/`z_return_*` functions or
`g_critical`. This will show a bug report popup
if hit.

To report non-programming errors, such as failure
to open a file, use the
[GError](https://www.freedesktop.org/software/gstreamer-sdk/data/docs/latest/glib/glib-Error-Reporting.html) mechanism.

## Function attributes
There are macros available for function
attributes that help the compiler optimize better
or provide better feedback. See the headers for
examples.

Tips
* `inline` small functions that are called very
often
* `CONST`/`PURE` can be combined with inline
* `HOT` should not be combined with inline
* only use `CONST`/`PURE` on functions that do not log (or use `g_warn_*()`/`g_return_*()`)

## Licensing
For license headers, we use SPDX license
identifiers. The comment style depends on the file
type:

    C source: // SPDX-License-Identifier: <SPDX License Expression>
    C header: /* SPDX-License-Identifier: <SPDX License Expression> */
    scripts:  # SPDX-License-Identifier: <SPDX License Expression>
    .rst:     .. SPDX-License-Identifier: <SPDX License Expression>

If you contributed significant (for copyright
purposes) amounts of code in a file, you should
append your copyright notice (name, year and
optionally email/website) at the top.

If a file incorporates work under a different
license, it should be explicitly mentioned below
the main SPDX tags and each copyright and permission
notice must be separated by `---`. See
[src/audio/graph_thread.c](src/audio/graph_thread.c)
for an example.

For the copyright years, Zrythm uses a range
(“2008-2010”) instead of listing individual years
(“2008, 2009, 2010”) if and only if every year
in the range, inclusive, is a “copyrightable” year
that would be listed individually.

See also [CONTRIBUTING.md](CONTRIBUTING.md) and
[REUSE FAQ](https://reuse.software/faq/).

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

## Error building template class 'TimelineToolbarWidget'...:0:0 `<property>` is not a valid tag here
The error is correct but line numbers are not
available because the resource files are compressed.
