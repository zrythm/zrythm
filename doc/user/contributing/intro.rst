.. This is part of the Zrythm Manual.
   Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Contributing
============
We appreciate contributions of any type and size. Tell us
how you would like to help, and we will do our best to
guide you.

We want to provide a friendly and harassment-free environment
so that everyone can contribute to the best of their
abilities. To this end our project uses a `Contributor
Covenant` which was adapted from https://contributor-covenant.org/. You can find the full pledge in our
`Code of Conduct <https://forum.zrythm.org/t/code-of-conduct>`_.

Coding
------
Please see the
:download:`HACKING <../../../HACKING.md>` file
in the source distribution.

Web Design
----------
You are welcome to help us improve the looks and
usability of the Zrythm website. Please join our chatrooms
and talk to us.

Artwork and Themes
------------------
Zrythm is CSS themable and also makes use of
many icons, some customized and some borrowed from
other themes. If you would like to contribute
artwork such as icons you can start by creating your own
themes and icons as discussed in
:ref:`icon-themes` and :ref:`css`.

Testing
-------
You are welcome to join us in testing
the latest features in Zrythm and reporting
bugs and feedback on
`Sourcehut <https://todo.sr.ht/~alextee>`_.

The latest development branch for new features is
``development`` in our
`git repository <https://git.zrythm.org/cgit/zrythm>`_.

Editing this Manual
-------------------
Click on the :guilabel:`View Source` button at the
top of the page to find which file the page exists
in, find the file in the source distribution and
edit it.

You will need to have
`Sphinx <https://www.sphinx-doc.org/en/master/>`_
installed to compile this manual.

Translation
-----------
Zrythm is available for translation at `Weblate
<https://hosted.weblate.org/engage/zrythm/?utm_source=widget>`_.

The Zrythm translation project contains the translations
for the Zrythm program, the website as well as sections
of this manual.

Click on the project you wish to work on, and
then select a language in
the screen that follows.
For more information on using Weblate,
please see the
`official documentation <https://docs.weblate.org/en/latest/user/translating.html>`_.

User Manual
~~~~~~~~~~~
When translating the user manual, please make sure
you follow the following Sphinx/RST syntax, to avoid
errors when applying your translations.

General Syntax
++++++++++++++
When you see something inside ``:``, such as
``:term:``, it should be left untranslated. The
part following should be left untranslated as well.
For example:

.. code-block:: text

  A plugin has parameters (see :term:`Parameter`).

should be translated in French as

.. code-block:: text

  Un plugin a des paramètres (:term:`Parameter`).

This is because this value will replaced
automatically by Sphinx with the translation for
``Parameter``. The only exception to this rule is
when you see the ``<>`` characters. For example:

.. code-block:: text

  A plugin has :term:`parameters <Parameter>`.

should be translated in French as

.. code-block:: text

  Un plugin a des :term:`paramètres <Parameter>`.

.. important::
  Please make sure you do not insert or remove
  spaces. The following examples will cause errors.

  .. code-block:: text

    :term: `Parameter`
          ^ space
    : term:`Parameter`
     ^ space
    :term :`Parameter`
         ^ space

  Also, there should be space, comma or period
  after such syntax, like below.

  .. code-block:: text

    :term:`Parameter` other information
    :term:`Parameter`, other information
    :term:`Parameter`. Other information

  The following examples will cause errors.

  .. code-block:: text

    :term:`Parameter`other information
                     ^ missing space/punctuation

.. note:: The following syntax usually refers to a
  path, so please keep it unchanged,
  otherwise the file it refers to will not be found.

  .. code-block:: text

    :doc:`../../example`

Donations
---------
We use the following services for receiving donations.
Any amount, small or large is appreciated and helps
sustain continuous development:

* `LiberaPay <https://liberapay.com/Zrythm>`_
* `PayPal <https://paypal.me/zrythm>`_
* `Open Collective <https://opencollective.com/zrythm>`_

Alternatively, you can `purchase a Zrythm installer
<https://www.zrythm.org/en/download.html>`_.
