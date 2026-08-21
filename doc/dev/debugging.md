<!---
SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

Debugging
=========

# GDB Cheatsheet

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

# Valgrind

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

# Sanitizers

The generated `conanbuild/<config>/generators/conanrun.sh` carries the
runtime options and suppressions file paths for the sanitizer enabled at
`conan install` time (composed by `conanfile.py`; suppressions live in
`tools/*_suppressions.supp`). Source it before running tests or binaries:

    source conanbuild/Debug/generators/conanrun.sh

Sanitizers are selected either by the `compiler.sanitizer` profile setting
(e.g. `-pr:h clang_tsan`, which also instruments dependencies) or by the
consumer-scoped `-o sanitizer=...` option (which instruments only Zrythm
itself). The recipe rejects combinations that would produce a silently
half-instrumented build: a `thread` selection without the profile setting,
or an option that disagrees with the profile. ThreadSanitizer builds must
use the profile because uninstrumented Qt aborts the TSan runtime (its
`pthread_clockjoin_np`-based thread join is not intercepted by TSan).

ASan and TSan builds also get `QV4_FORCE_INTERPRETER=1` in the composed
environment: the QML JIT is incompatible with ASan (shadow memory vs
JIT-allocated code) and crashes under TSan.

The qt recipe revision pinned in `conan.lock` comes from the vendored
recipe in `ext/conan-center-index` and exists in the local Conan cache only
after `tools/export_conan_recipes.py` runs (CI does this in
`before_script`). Conan commands that skip the export fail to resolve the
pin.

For gdb sessions, `tools/conanrun_wrapper.sh` (used by
`.vscode/launch.json` as an exec-wrapper) sources the same file and adds
gdb-specific overrides: `detect_leaks=0` (LSan aborts under ptrace) and
`QV4_FORCE_INTERPRETER=1` unconditionally (the same launch configs serve
all build types).

# Profiling

## perf
To profile with perf (recommended), set optimization to 2 and debug to true
and run:

    perf record --event=cache-references,cache-misses,cycles,instructions,branches,faults,migrations,cpu-clock --stat build/src/zrythm --dummy

Then examine the report with:

    perf report --input=perf.data --hide-unresolved

See also:
* <http://sandsoftwaresound.net/perf/perf-tutorial-hot-spots/>

## gprof
To profile with gprof,
enable the `ZRYTHM_PROFILING` CMake option
(`-DZRYTHM_PROFILING=ON`) when configuring,
then build and run the program normally. The program
must end gracefully (ie, not Ctrl-C). When the
program ends, run

    gprof --flat-profile --annotated-source -B --exec-counts --directory-path="$pwd)" --print-path --graph --table-length=16 --function-ordering --min-count=100 conanbuild/Debug/products/bin/zrythm > results

and check the results file for the profiling results.

[gprof2dot](https://github.com/jrfonseca/gprof2dot)
along with
[xdot.py](https://github.com/jrfonseca/xdot.py) can
be used for a graphical representation of the
results.

gprof has the advantage of being fast (the program runs at normal speed).

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
tool from valgrind can be used. Run the test binary
under valgrind directly (test binaries live in
`conanbuild/Debug/products/bin/`; use `--gtest_filter`
to select tests). For example:

    valgrind --tool=massif --num-callers=160 --suppressions=$(pwd)/tools/vg.sup ./conanbuild/Debug/products/bin/zrythm_actions_unit_tests --gtest_filter='*ArrangerObjectSelection*'

This will create a file called `massif.out.<pid>`
that contains memory snapshots in the current
directory. This file can be opened with
[Massif-Visualizer](https://apps.kde.org/en-gb/massif-visualizer/)
for inspection.

If the test takes too long, it can be stopped with
`SIGTERM` and results will be collected until
termination.

*Note: massif runs the program 20 times slower*

Alternatively, valgrind's leak check can be used on the
whole suite (or a `-R` subset) via CTest's memcheck
mode:

    ctest --test-dir conanbuild/Debug -T memcheck -R "SomeTest"

`include(CTest)` sets `MEMORYCHECK_COMMAND` when
valgrind is found at configure time; customize the run
via the `MEMORYCHECK_COMMAND_OPTIONS` and
`MEMORYCHECK_SUPPRESSIONS_FILE` CMake cache entries.
Logs are written to
`conanbuild/Debug/Testing/Temporary/MemoryChecker.<#>.log`.

For ad-hoc runs with custom flags, run the test binary
under valgrind directly:

    valgrind --num-callers=160 --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --suppressions=$(pwd)/tools/vg.sup ./conanbuild/Debug/products/bin/zrythm_actions_unit_tests --gtest_filter='*ArrangerObjectSelection*'

# Real-time safety

TL;DR: use clang's RealtimeSanitizer facilities.

TODO: write this section.

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
    cmake --build builddir --target manual_bundle

# Coding Guidelines

## Coding Style

TODO: explain contributors need to run clang-format

## Licensing
For license headers, we use SPDX license
identifiers.

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

See [commit_messages.md](commit_messages.md) for the commit message
conventions (subject and body formatting, the required `Signed-off-by:`
sign-off, and other trailers).
