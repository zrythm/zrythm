Contributing Guidelines
=======================

# Code Structure

    .
    ├── AUTHORS                      # Author information
    ├── CHANGELOG.md                 # Changelog
    ├── CONTRIBUTING.md              # Contributing guidelines
    ├── COPYING                      # Main license
    ├── COPYING.*                    # Other licenses
    ├── data                         # Data files to be installed
    │   ├── fonts                    # Fonts
    │   ├── org.zrythm.Zrythm.gschema.xml # GSettings schema
    │   ├── org.zrythm.Zrythm-mime.xml # MIME type specification for project files
    │   ├── samples                  # Audio samples
    │   └── zrythm.desktop.in        # Desktop file
    ├── doc                          # Documentation
    │   ├── dev                      # Developer docs
    │   ├── man                      # Manpage
    │   └── user                     # User manual
    ├── ext                          # External libs
    │   ├── audio_decoder            # Audio decoder by rgareus
    │   ├── libcyaml                 # Yaml serialization
    │   ├── midilib                  # MIDI file serialization
    │   └── zix                      # Data struct utils
    ├── git-packaging-hooks          # Git hooks for packaging
    ├── inc                          # Include dir
    │   ├── actions                  # Actions (undo, quit, etc.)
    │   ├── audio                    # Audio-related headers
    │   ├── gui                      # Gui-related headers
    │   │   ├── backend              # Backend for serialization
    │   │   └── widgets              # Gui widgets
    │   ├── plugins                  # Plugin handling
    │   │   ├── lv2                  # LV2 plugin handling
    │   │   └── vst                  # VST plugin handling
    │   ├── settings                 # Settings
    │   ├── utils                    # Various utils
    │   └── zrythm.h                 # Main zrythm struct
    ├── INSTALL.md                   # Installation instructions
    ├── meson.build                  # Meson conf
    ├── meson.options                # Meson options
    ├── plugins                      # Bundled plugins
    ├── po                           # I18n
    │   ├── *.po                     # Translations
    │   ├── LINGUAS                  # Languages
    │   ├── POTFILES                 # List of translatable files
    │   └── zrythm.pot               # Source strings
    ├── README.md                    # Main README file
    ├── resources                    # Bundled resources
    │   ├── gen-gtk-resources-xml.py # Gtk resources generator
    │   ├── icons                    # Icons
    │   ├── theme                    # Parent GTK theme
    │   ├── theme.css                # Zrythm GTK theme
    │   └── ui                       # GTK ui files for widgets
    ├── scripts                      # various scripts
    │   ├── collect_translatables.sh # Prints the translatable files
    │   └── meson_post_install.py    # Commands to run after install
    ├── src                          # Source (.c) counterparts of inc
    ├── tests                        # Unit tests
    │   └── helpers                  # Test helpers
    ├── THANKS                       # Thanks notice
    ├── THIRDPARTY_LICENSE           # Linked library license information
    ├── tools                        # Various tools
    └── TRANSLATORS                  # List of translators

# Debugging
Use `GDK_SYNCHRONIZE=1 G_DEBUG=fatal_warnings gdb build/src/zrythm`.
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
https://developer.gnome.org/glib/stable/glib-Warnings-and-Assertions.html

Additional options can be added to `G_DEBUG`, like
`G_DEBUG=fatal_warnings,signals,actions`.

There is also `G_ENABLE_DIAGNOSTIC=1` to break
at deprecation warnings for GTK+3 (must all be fixed
before porting to GTK+4).

## Environment Variables
In addition to GTK/GLib variables, Zrythm
understands the following environment variables.
- `ZRYTHM_DSP_THREADS` - number of threads
  to use for DSP, including the main one
- `NO_SCAN_PLUGINS` - disable plugin scanning
- `ZRYTHM_DEBUG` - shows additional debug info about
  objects

## UI Debugging
GTK comes with a very powerful tool called
GTK inspector that can be used to inspect UI widgets
and change their properties live (while the
application is running).

This is very useful when looking to try out different
widget properties to see what works before
implementing them. Adding custom CSS is also possible.
For example, the following CSS can be entered live
to color all the track backgrounds red.

    track {
      background-color: red;
    }

More info on how to enable and use the inspector can be
found here:
https://wiki.gnome.org/action/show/Projects/GTK/Inspector

# Tests and Coverage
To run the test suite, use

    meson build -Denable_tests

followed by

    ninja -C build test

To get a coverage report use

    meson build -Denable_tests=true -Denable_coverage=true

followed by the test command above, followed by

    gcovr -r .

or for html output

    gcovr -r . --html -o coverage.html

# Profiling
## gprof
To profile with gprof,
use the `enable_profiling` option when running meson,
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
https://docs.kde.org/stable5/en/kdesdk/kcachegrind/using-kcachegrind.html and

# Collecting Translations
To collect all translatable filenames into
`po/POTFILES`, generate the POT file and update
the PO files, use

    ninja -C build collect-translatables zrythm-pot zrythm-update-po

# OBS Packaging
Binary packages are created on [OBS (Open Build System)](https://build.opensuse.org/package/show/home:alextee/zrythm#) using git hooks.
See the [README](git-packaging-hooks/README.md) in git-packaging-hooks and the `packaging` branch.

# Coding
These are some guidelines for contributing code.
They are not strict rules.

## Commenting
- Document how to use the function in the header file
- Document how the function works (if it's not
obvious from the code) in the source file
- Document each logical block within a function (what
it does so one can understand what the function does
at each step by only reading the comments)

## Coding Style
GNU style is preferable.

## Include Order
In alphabetic order:
1. Standard C library headers
2. Local headers
3. GTK and GLib headers
4. Any other headers

## Line Length
Please keep lines within 60 characters. This works
nicely with 8 files open simultaneously in a tiled
editor like vim.

## Tips
- Generally, backend files should have a `*_new()`
function to create a new instance or an `*_init()`
function to initialize an existing instance,
depending on if they are to be created dynamically
or not.
- Widgets should have a `*_new()` function if they
should be created dynamically. They should have a
`*_setup()` function to initialize them or "set them
up" and a `*_refresh()` function to refresh their
internal state. `*_setup()` is to be called only once
after the instance is created and `*_refresh()` to be
called multiple times as needed.
- Zrythm has a UI event system that redraws only the
relevant widgets based on the type of event.

## Licensing
If you contributed significant (for copyright
purposes) amounts of code, you may add your
copyright notice (name, year and optionally
email/website) at the top of the file, otherwise
you agree to assign all copyrights to Alexandros
Theodotou, the main author.

All your changes should be licensed
under the AGPLv3+ like the rest of the project, or
at least a compatible Free Software license of
your choice.

## Common Problems
### Getting random GUI related errors with no trace in valgrind or GTK warnings
This might happen when calling GTK code or
`g_idle_add()` from non-GUI threads. GTK runs in a single
GUI thread. Any GTK-related code must be run from
the GUI thread only.

## Submitting Patches
Please use [Redmine](https://redmine.zrythm.org/projects/zrythm/issues)

# Translating Zrythm
Zrythm is translated on Weblate:
<https://hosted.weblate.org/engage/zrythm/?utm_source=widget>

Alternatively, you can edit the PO files directly
and submit a patch.

# Testing and Feedback
Test Zrythm and report bugs, ideas, feedback and
suggestions on the issue tracker.

# Troubleshooting
TODO

----

Copyright (C) 2018-2019 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
