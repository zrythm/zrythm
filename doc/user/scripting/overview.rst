.. SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
.. SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Overview
========

Zrythm offers a scripting interface implemented
using
`GNU Guile <https://www.gnu.org/software/guile/>`_
that supports multiple scripting languages.
Zrythm currently supports the following scripting
languages:

- `ECMAScript <https://en.wikipedia.org/wiki/ECMAScript>`_
- `Scheme <https://en.wikipedia.org/wiki/Scheme_%28programming_language%29>`_

.. - `Emacs Lisp <https://en.wikipedia.org/wiki/Emacs_Lisp>`_

.. warning:: The Guile API is experimental.

.. note:: The Guile API is not available on Windows.
   See `this ticket <https://github.com/msys2/MINGW-packages/issues/3298>`_
   for details.

The :ref:`API section <scripting/api/intro:API>`
is a comprehensive list of all
available procedures in the API. Each section
in the API corresponds to a specific Guile module.

Using Modules
~~~~~~~~~~~~~

To use the :scheme:`audio position` module from
Scheme, use the following code:

.. code-block:: scheme

  (use-modules (audio position))
  (let* ((pos (position-new 1 1 1 0 0)))
    (position-print pos))

To use it from ECMAScript, use:

.. code-block:: javascript

  var position_module = require ('audio.position');
  var position_new = position_module['position-new'];
  var pos = position_new (1, 1, 1, 0, 0);
  var position_print = position_module['position-print'];
  position_print (pos);
