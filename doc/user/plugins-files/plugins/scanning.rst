.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Scanning for Plugins
====================

Zrythm will scan for plugins on startup and will remember
those plugins until it is closed. Zrythm supports both
LV2 and VST plugins, although LV2 is the recommended
standard.

.. tip:: Plugin scanning can be disableld by passing
   NO_SCAN_PLUGINS=1 when running Zrythm.

LV2 Scan
--------

Zrythm will scan for LV2 plugins in the standard paths
specified
`here <https://lv2plug.in/pages/filesystem-hierarchy-standard.html>`_. You can bypass this behavior by passing the
``LV2_PATH`` environment variable to specify custom
paths. For example, ``LV2_PATH=~/custom-lv2-dir zrythm``.

VST Scan
--------

On UNIX, Zrythm will scan for VST plugins in the
following paths.

- ``~/.vst``
- ``~/vst``
- ``/usr/local/lib/vst``
- ``/usr/lib/vst``

You can bypass this behavior by passing the
``VST_PATH`` environment variable to specify custom
paths. For example, ``VST_PATH=~/custom-vst-dir zrythm``.

.. note:: ``lib`` will be replaced with ``lib64`` depending
   on your system.

On Windows, Zrythm will scan for plugins in the paths
specified in `VST plug-in locations on Windows <https://helpcenter.steinberg.de/hc/en-us/articles/115000177084-VST-plug-in-locations-on-Windows>`_.

- ``C:\Program Files\VSTPlugins``
- ``C:\Program Files\Steinberg\VSTPlugins``
- ``C:\Program Files\Common Files\VST2``
- ``C:\Program Files\Common Files\Steinberg\VST2``

Since scanning VST plugins takes a long time, Zrythm
will remember scanned VST plugins and save this
information in ``<<ZRYTHM PATH>>/cached_vst_descriptors.yaml``.
New plugins will be scanned on each start-up, and
you can delete or edit this file to force a re-scan of
previously scanned plugins.
