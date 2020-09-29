.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Overview
========

Zrythm offers a scripting interface using the
`Guile language <https://www.gnu.org/software/guile/>`_, an implementation of Scheme.

The next section is a comprehensive list of all
available procedures in the API. Each section
in the API corresponds to a specific Guile module,
so for example, to use the procedures in
the :scheme:`audio position` section, one would do
:scheme:`(use-modules (audio track))` at the top
of the script.

.. warning:: The Guile API is experimental.

.. note:: The Guile API is not available on Windows.
