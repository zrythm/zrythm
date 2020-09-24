.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _scanning-plugins:

Scanning for Plugins
====================

Zrythm will scan for plugins on startup and will
remember
those plugins until it is closed. Zrythm supports
:term:`LV2`, :term:`VST2`, :term:`VST3` and
:term:`AU` plugins. LV2 is the recommended
standard.

:term:`SFZ` and :term:`SF2` instruments are also
supported, and they are
scanned as instrument plugins.

.. tip:: Plugin scanning can be disabled by passing
   :envvar:`NO_SCAN_PLUGINS` when running Zrythm.

LV2 Scan
--------

Zrythm will scan for LV2 plugins in the
`standard paths specified here <https://lv2plug.in/pages/filesystem-hierarchy-standard.html>`_.
You can bypass this behavior by passing
:envvar:`LV2_PATH`.

VST2/VST3 Scan
--------------

GNU/Linux and FreeBSD
~~~~~~~~~~~~~~~~~~~~~
Zrythm will scan for VST2 plugins in the following
paths,

- :file:`{$HOME}/.vst`
- :file:`{$HOME}/vst`
- :file:`/usr/local/lib/vst`
- :file:`/usr/lib/vst`

and VST3 plugins in the following paths.

- :file:`{$HOME}/.vst3`
- :file:`/usr/local/lib/vst3`
- :file:`/usr/lib/vst3`

You can bypass this behavior by passing
:envvar:`VST_PATH` and :envvar:`VST3_PATH`.

.. note:: If your system uses a libdir other than
   ``lib`` (for example ``lib64``), Zrythm will scan
   for plugins in both locations.

Windows
~~~~~~~
Zrythm will scan for VST plugins in the paths
specified in :ref:`vst-paths`.

MacOS
~~~~~
Zrythm will scan for VST plugins in the paths
specified in `VST plug-in locations on Mac OS X and macOS <https://helpcenter.steinberg.de/hc/en-us/articles/115000171310>`_.

* :file:`/Library/Audio/Plug-Ins/VST` for VST2
* :file:`/Library/Audio/Plug-Ins/VST3` for VST3

.. note:: Zrythm
  will remember scanned VST plugins and save this
  information in
  :file:`cached_plugin_descriptors.yaml` in the
  :term:`Zrythm user path`.

  New plugins will be scanned on each start-up, and
  you can delete or edit this file to force a
  re-scan of previously scanned plugins.

AU Scan
-------
On MacOS, :term:`AU` plugins will be scanned at
their standard location at
:file:`/Library/Audio/Plug-Ins/Components`.

SFZ/SF2 Scan
------------
:term:`SFZ` and :term:`SF2` instruments will be
scanned in all  directories
and subdirectories specified in :ref:`vst-paths`.
