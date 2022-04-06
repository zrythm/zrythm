.. SPDX-FileCopyrightText: © 2019-2020, 2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Contributing
============
We appreciate contributions of any type and size.
Tell us how you would like to help, and we will do
our best to guide you.

Coding
------
Please see the
:download:`HACKING <../../../HACKING.md>` file
in the source distribution.

Web Design
----------
You are welcome to help us improve the looks and
usability of the Zrythm website. Please get in touch
with us to discuss.

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
`git repository <https://git.zrythm.org/zrythm/zrythm>`_.

Editing this Manual
-------------------
Edit the RST files under :file:`doc/user` and
submit a patch when finished.

You will need to have
`Sphinx <https://www.sphinx-doc.org/en/master/>`_
installed to compile this manual.

Translation
-----------
Zrythm is available for translation at `Weblate
<https://hosted.weblate.org/engage/zrythm/?utm_source=widget>`_.

The Zrythm translation project contains the
translations for the Zrythm program, the website,
the accounts site, and sections of this manual.

Click on the project you wish to work on, and
then select a language in
the screen that follows.
For more information on using Weblate,
please see the
`official documentation <https://docs.weblate.org/en/latest/user/translating.html>`_.

.. hint:: Zrythm contains terminology similar to
   existing audio software. You may find it useful
   to borrow terms from them. See the
   `Cubase Pro Artist manual <https://steinberg.help/cubase_pro_artist/v9/fr/>`_ for example (replace
   `fr` in the URL with your language code).

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

We also accept cryptocurrency donations at the
following addresses:

.. code-block:: text

   Bitcoin (BTC): bc1qjfyu2ruyfwv3r6u4hf2nvdh900djep2dlk746j
   Litecoin (LTC): ltc1qpva5up8vu8k03r8vncrfhu5apkd7p4cgy4355a
   Monero (XMR): 87YVi6nqwDhAQwfAuB8a7UeD6wrr81PJG4yBxkkGT3Ri5ng9D1E91hdbCCQsi3ZzRuXiX3aRWesS95S8ij49YMBKG3oEfnr

Alternatively, you can `purchase a Zrythm installer
<https://www.zrythm.org/en/download.html>`_.
