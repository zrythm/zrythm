<!---
SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

Zrythm User Manual
==================

# Dependencies
- [Sphinx doc](http://sphinx-doc.org/)
- [furo](https://pypi.org/project/furo/) theme for HTML output
- sphinx-intl
- [sphinx-copybutton](https://pypi.org/project/sphinx-copybutton/)
- [sphinxcontrib-svg2pdfconverter](https://pypi.org/project/sphinxcontrib-svg2pdfconverter/)
- guile
- `guix build utils` guile module (installed with guix package)
- texlive (for PDF)
- texi2html (for Guile API)
- pandoc (for Guile API)

# Building
This is built using `ninja -C build manual_bundle`

# Tips
- To reference the ``Plugin Ports`` section in the
file `plugin-info.rst`, use

    :ref:`plugins-files/plugins/plugin-info:Plugin Ports`

- To reference `editing/clip-editors/ruler.rst`,
depending on where the current document is use

    :doc:`../editing/clip-editors/ruler`

- To reference a specific subsection anywhere, add
`.. _audio-bus-track:` right before it, and
reference it with

    :ref:`audio-bus-track`

- To use zrythm-dark icons, use

    |icon_zoom-in|


Generally, prefer the 3rd option because files may
move around, unless the reference needs to be
more specific.

# License
This manual is licensed under the GNU Free Documentation License, version 1.3 or later. See the
[index.rst](index.rst) file for details.

The full text of the license can be found in the
root of this distribution.
