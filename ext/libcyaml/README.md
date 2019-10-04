LibCYAML: Schema-based YAML parsing and serialisation
=====================================================

[![Build Status](https://travis-ci.org/tlsa/libcyaml.svg?branch=master)](https://travis-ci.org/tlsa/libcyaml) [![Code Coverage](https://codecov.io/gh/tlsa/libcyaml/branch/master/graph/badge.svg)](https://codecov.io/gh/tlsa/libcyaml) [![Code Quality](https://img.shields.io/lgtm/grade/cpp/g/tlsa/libcyaml.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/tlsa/libcyaml/alerts/)

LibCYAML is a C library for reading and writing structured YAML documents.
It is written in ISO C11 and licensed under the ISC licence.

Overview
--------

The fundamental idea behind CYAML is to allow applications to construct
schemas which describe both the permissible structure of the YAML documents
to read/write, and the C data structure(s) in which the loaded data is
arranged in memory.

### Goals

* Make it easy to load YAML into client's custom C data structures.
* Good compromise between flexibility and simplicity.

### Features

* Load, Save and Free functions.
* Reads and writes arbitrary client data structures.
* Schema-driven, allowing control over permitted YAML, for example:
    - Required / optional mapping fields.
    - Allowed / disallowed values.
    - Minimum / maximum sequence entry count.
    - etc...
* Enumerations and flag words.
* YAML backtraces make it simple for users to fix their YAML to
  conform to your schema.
* Uses standard [`libyaml`](https://github.com/yaml/libyaml) library for
  low-level YAML read / write.
* Support for YAML aliases and anchors.

Status
------

Until version 1.0.0 is released, the API and ABI are subject to change.
Feedback welcome.

Building
--------

To build the library (debug mode), run:

    make

Another debug build variant which is built with address sanitiser (incompatible
with valgrind) can be built with:

    make VARIANT=san

To build a release version:

    make VARIANT=release

Installation
------------

To install a release version of the library, run:

    make install VARIANT=release

It will install to the PREFIX `/usr/local` by default, and it will use
DESTDIR and PREFIX from the environment if set.

Testing
-------

To run the tests, run any of the following, which generate various
levels of output verbosity (optionally setting `VARIANT=release`, or
`VARIANT=san`):

    make test
    make test-quiet
    make test-verbose
    make test-debug

To run the tests under `valgrind`, a similar set of targets is available:

    make valgrind
    make valgrind-quiet
    make valgrind-verbose
    make valgrind-debug

To generate a test coverage report, `gcovr` is required:

    make coverage

Documentation
-------------

To generate both public API documentation, and documentation of CYAML's
internals, `doxygen` is required:

    make docs

Alternatively, the read the API documentation directly from the
[cyaml.h](https://github.com/tlsa/libcyaml/blob/master/include/cyaml/cyaml.h)
header file.

There is also a [tutorial](docs/guide.md).

Examples
--------

In addition to the documentation, you can study the [examples](examples/).
