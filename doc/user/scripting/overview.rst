.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Overview
========

Zrythm offers a scripting interface using the
`GNU Guile language <https://www.gnu.org/software/guile/>`_,
an implementation of
`Scheme <https://en.wikipedia.org/wiki/Scheme_%28programming_language%29>`_,
which is a dialect of
`LISP <https://en.wikipedia.org/wiki/Lisp_(programming_language)>`_.

The next section is a comprehensive list of all
available procedures in the API. Each section
in the API corresponds to a specific Guile module,
so for example, to use the procedures in
the :scheme:`audio position` section, one would do
:scheme:`(use-modules (audio position))` at the top
of the script.

.. warning:: The Guile API is experimental.

.. note:: The Guile API is not available on Windows.
   See `this ticket <https://github.com/msys2/MINGW-packages/issues/3298>`_
   for details.
