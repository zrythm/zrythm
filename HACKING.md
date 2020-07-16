Hacking Zrythm
==============

# Code Structure

    .
    ├── AUTHORS                      # Author information
    ├── build                        # temporary build dir
    ├── CHANGELOG.md                 # Changelog
    ├── CODE_OF_CONDUCT.md           # Code of conduct
    ├── CONTRIBUTING.md              # Contributing guidelines
    ├── COPYING                      # Main license
    ├── COPYING.*                    # Other licenses
    ├── data                         # Data files to be installed
    │   ├── fonts                    # Fonts
    │   ├── gtksourceview-monokai-extended.xml # GtkSourceView theme
    │   ├── org.zrythm.Zrythm-mime.xml # MIME type specification
    │   ├── samples                  # Audio samples
    │   ├── theme.css                # UI theme
    │   ├── zrythm-completion.bash   # Bash completion
    │   ├── zrythm.desktop.in        # Desktop file
    │   └── zrythm_launch.in         # Launcher script
    ├── doc                          # Documentation
    │   ├── dev                      # Developer docs
    │   ├── man                      # Manpage
    │   └── user                     # User manual
    ├── ext                          # External libs
    │   ├── midilib                  # MIDI file serialization
    │   └── zix                      # Data struct utils
    ├── git-packaging-hooks          # Git hooks for packaging
    ├── inc                          # Include dir
    │   ├── actions                  # Actions (undo, quit, etc.)
    │   ├── audio                    # Audio-related headers
    │   ├── gui                      # Gui-related headers
    │   │   ├── backend              # Backend for serialization
    │   │   └── widgets              # Gui widgets
    │   ├── guile                    # Guile scripting interface
    │   ├── plugins                  # Plugin handling
    │   │   ├── lv2                  # LV2 plugin handling
    │   │   └── vst                  # VST plugin handling
    │   ├── settings                 # Settings
    │   ├── utils                    # Various utils
    │   └── zrythm.h                 # Main zrythm struct
    ├── INSTALL.md                   # Installation instructions
    ├── meson.build                  # Meson conf
    ├── meson.options                # Meson options
    ├── PACKAGING.md                 # Information for packagers
    ├── po                           # I18n
    │   ├── *.po                     # Translations
    │   ├── LINGUAS                  # Languages
    │   ├── POTFILES                 # List of translatable files
    │   └── zrythm.pot               # Source strings
    ├── README.md                    # Main README file
    ├── resources                    # Bundled resources
    │   ├── gen-gtk-resources-xml.scm # Gtk resources generator
    │   ├── icons                    # Icons
    │   ├── theme                    # Parent GTK theme
    │   └── ui                       # GTK ui files for widgets
    ├── scripts                      # various scripts
    │   ├── collect_translatables.sh # Prints the translatable files
    │   ├── generic_guile_wrap.sh    # Wrapper for running guile scripts
    │   ├── gschema-gen.scm          # GSettings schema generator
    │   ├── guile-gen-docs.scm       # Guile API doc generator
    │   ├── guile_gen_texi_docs.sh   # Creates texi docs for the Guile API
    │   ├── guile-snarf-wrap.scm     # Snarfing for Guile API
    │   ├── guile-utils.scm          # Utilities used in other scripts
    │   ├── meson_dist.sh            # Command to run after meson dist
    │   ├── meson-post-install.scm   # Commands to run after install
    │   └── run_stoat.sh             # Runs stoat
    ├── src                          # Source (.c) counterparts of inc
    ├── subprojects                  # Subprojects to auto-fetch if some dependencies are not found
    ├── tests                        # Unit tests
    │   └── helpers                  # Test helpers
    ├── THANKS                       # Thanks notice
    ├── tools                        # Various tools
    └── TRANSLATORS                  # List of translators

# Debugging
Use `G_DEBUG=fatal_warnings gdb build/src/zrythm`.
`G_DEBUG=fatal_warnings` will trigger break points at
warning messages, and is very useful when debugging.

`GDK_SYNCHRONIZE=1` will make all X requests
synchronously, which can be useful for UI debugging,
but it will slow down performance.

An easy way to test something is to add a
`g_warning ("hello");` or
`g_warn_if_reached ();`
and it will automatically break there for you.

For more information on warnings and assertions see
<https://developer.gnome.org/glib/stable/glib-Warnings-and-Assertions.html>.

We suggest building with `-Dextra_debug_info` which
essentially adds `-g3` to the CFLAGS. This allows
use of C macros in gdb so you can do `p TRACKLIST`
instead of `p zrythm->project->tracklist`.

## Environment Variables
In addition to GTK/GLib variables, Zrythm
understands the following environment variables.
- `ZRYTHM_DSP_THREADS` - number of threads
  to use for DSP, including the main one
- `NO_SCAN_PLUGINS` - disable plugin scanning
- `ZRYTHM_DEBUG` - shows additional debug info about
  objects

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

    ninja -C build test

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

## Callgrind
Alternatively, you can use callgrind with kcachegrind.
Build Zrythm normally and then run it through
callgrind as follows:

    valgrind --tool=callgrind build/src/zrythm

When you are finished, close Zrythm and run
`kcachegrind` in the same directory to display the
profiling info in the kcachegrind GUI.
For more information, see
<https://docs.kde.org/stable5/en/kdesdk/kcachegrind/using-kcachegrind.html>.

# Real-time safety
Use the following to get a stoat report.

    CC=stoat-compile CXX=stoat-compile++ meson build
    ninja -C build run_stoat

For more info see
<https://github.com/fundamental/stoat>.

# Collecting Translations
To collect all translatable filenames into
`po/POTFILES`, generate the POT file and update
the PO files, use

    ninja -C build collect-translatables zrythm-pot zrythm-update-po

# Creating Releases
Releases are created using git hooks.
See the [README](git-packaging-hooks/README.md) in
git-packaging-hooks for more information.

# Coding Guidelines
## Commenting
Please document everything specified in header files using
Doxygen tags. At a bare minimum, every function
declaration should have a brief description.

It is extremely helpful if you document each logical block
within a function using plain comments, in a way that
one can understand what is happening in each step by
only reading the comments.

## Coding Style
Generally, try to follow the
[GNU coding standards](https://www.gnu.org/prep/standards/html_node/Writing-C.html).

## Include Order
In alphabetic order:
1. Standard C library headers
2. Local headers
3. GTK and GLib headers
4. Any other headers

## Line Length
We keep lines within 60 characters. This works
nicely with 4 columns of files open simultaneously
in a tiled editor like vim, but if you find this
difficult to work with < 80 characters is acceptable
too.

## Licensing
If you contributed significant (for copyright
purposes) amounts of code in a file, you should append your
copyright notice (name, year and optionally
email/website) at the top.

## Submitting Patches
We prefer code contributions in the form of patches. Use `git format-patch` to generate patch files and
post them in a new issue on
[Redmine](https://redmine.zrythm.org/projects/zrythm/issues) or
send them to the dev mailing list at dev@zrythm.org.
You may also use `git-send-email` for this.
If you are having difficulties creating patches
please contact us and we will guide you.

# Troubleshooting
## Getting random GUI related errors with no trace in valgrind or GTK warnings
This might happen when calling GTK code or
`g_idle_add()` from non-GUI threads. GTK runs in a single
GUI thread. Any GTK-related code must be run from
the GUI thread only.

## Values not being read properly at specific parts of the code (e.g. getting a zero value when we know the variable is non-zero)
Delete build dir and reconfigure. This is likely
an optimization problem with meson/ninja that
appears rarely.

----

Copyright (C) 2018-2020 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
