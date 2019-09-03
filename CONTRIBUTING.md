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
    │   ├── COPYING                  # License information
    │   ├── org.zrythm.Zrythm.gschema.xml # GSettings schema
    │   └── zrythm.desktop           # Desktop file
    ├── doc                          # Documentation
    │   ├── *.h                      # Doxygen pages
    │   ├── Doxyfile.cfg.in          # Doxygen conf
    │   ├── Doxyfile-mcss.in         # Doxygen mcss conf
    │   ├── group_defs.dox           # TODO
    │   └── m.css                    # m.css theme for html docs
    ├── ext                          # External libs
    │   ├── audio_decoder            # Audio decoder by rgareus
    │   ├── libcyaml                 # Yaml serialization
    │   ├── midilib                  # MIDI file serialization
    │   ├── README                   # Disclaimer
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
    │   ├── fonts                    # Fonts FIXME move to data
    │   ├── gen-gtk-resources-xml.py # Gtk resources generator
    │   ├── icons                    # Icons
    │   ├── samples                  # Samples FIXME move to data
    │   ├── theme                    # Parent GTK theme
    │   ├── theme.css                # Zrythm GTK theme
    │   └── ui                       # GTK ui files for widgets
    ├── scripts                      # various scripts
    │   ├── collect_translatables.sh # Prints the translatable files
    │   ├── meson_post_install.py    # Commands to run after install
    ├── src                          # Source (.c) counterparts of inc
    ├── tests                        # Automated tests
    ├── THANKS                       # Thanks notice
    ├── THIRDPARTY_LICENSE           # Linked library license information
    ├── tools                        # TODO
    └── TRANSLATORS                  # List of translators

# Debugging
Use `gdb build/src/zrythm`

There is also

    G_DEBUG=fatal_warnings,signals,actions G_ENABLE_DIAGNOSTIC=1 gdb build/src/zrythm

`G_DEBUG` will trigger break points at GTK warning
messages and `G_ENABLE_DIAGNOSTIC` is used to break
at deprecation warnings for GTK+3 (must all be fixed
before porting to GTK+4).

# Tests and Coverage
To run the test suite, use

    meson _build -Denable_tests

followed by

    meson test -C _build

To get a coverage report use

    meson _build -Denable_tests=true -Denable_coverage=true

followed by the test command above, followed by

    gcovr -r .

or for html output

    gcovr -r . --html -o coverage.html

# Translations
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
You can use whatever method you like for sending
patches. There's [Savannah](https://savannah.nongnu.org/support/?group=zrythm), mailing lists and IRC.

# Profiling
Use the `enable profiling` option when running meson,
then build and run the program normally. The program
must end gracefully (ie, not Ctrl-C). When the
program ends, run

    gprof build/src/zrythm > results

and check the results file for the profiling results.

# Translating
Zrythm is translated on Weblate:
<https://hosted.weblate.org/engage/zrythm/?utm_source=widget>

Alternatively, you can edit the PO files directly
and submit a patch.

# Testing
Test Zrythm and report bugs, ideas and suggestions
on [Savannah](https://savannah.nongnu.org/support/?group=zrythm).

# Troubleshooting
TODO

----

Copyright (C) 2018-2019 Alexandros Theodotou

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
