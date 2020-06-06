.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _scanning-plugins:

Scanning for Plugins
====================

Zrythm will scan for plugins on startup and will remember
those plugins until it is closed. Zrythm supports
LV2, VST2, VST3 and AU plugins. LV2 is the recommended
standard.

SFZ and SF2 instruments are also supported, and they are
scanned as instrument plugins.

.. tip:: Plugin scanning can be disabled by passing
   ``NO_SCAN_PLUGINS=1`` when running Zrythm.

LV2 Scan
--------

Zrythm will scan for LV2 plugins in the `standard paths
specified
here <https://lv2plug.in/pages/filesystem-hierarchy-standard.html>`_. You can bypass this behavior by passing the
:envvar:`LV2_PATH` environment variable to specify custom
paths. For example,
``LV2_PATH=$HOME/custom-lv2-dir zrythm``.

VST2/VST3 Scan
--------------

GNU/Linux and FreeBSD
~~~~~~~~~~~~~~~~~~~~~
Zrythm will scan for VST2 and VST3 plugins in the
following paths.

- :file:`{$HOME}/.vst`
- :file:`{$HOME}/vst`
- :file:`/usr/local/lib/vst`
- :file:`/usr/lib/vst`

You can bypass this behavior by passing the
:envvar:`VST_PATH` environment variable to specify custom
paths. For example, ``VST_PATH=$HOME/custom-vst-dir zrythm``.

.. note:: ``lib`` will be replaced with ``lib64`` depending
   on your system.

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

.. note:: Since scanning VST plugins takes a long time, Zrythm
  will remember scanned VST plugins and save this
  information in
  ``$ZRYTHM_PATH/cached_vst_descriptors.yaml``.

  New plugins will be scanned on each start-up, and
  you can delete or edit this file to force a re-scan of
  previously scanned plugins.

AU Scan
-------
On MacOS, AUs will be scanned at their default location ``/Library/Audio/Plug-Ins/Components``.

SFZ/SF2 Scan
------------
SFZ and SF2 instruments will be scanned in all directories
and subdirectories specified in :ref:`vst-paths`.
