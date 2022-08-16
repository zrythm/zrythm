.. SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _scanning-plugins:

Scanning for Plugins
====================

Zrythm will scan for plugins on startup and will
remember
those plugins until it is closed. Zrythm supports
:term:`LV2`, :term:`VST2`, :term:`VST3`, LADSPA,
DSSI and :term:`AU` plugins. LV2 is the recommended
standard.

:term:`SFZ` and :term:`SF2` instruments are also
supported, and they are
scanned as instrument plugins.

.. tip:: Plugin scanning can be disabled by passing
   :envvar:`ZRYTHM_SKIP_PLUGIN_SCAN` when running
   Zrythm.

LV2 Scan
--------

Zrythm will scan for LV2 plugins in the
`standard paths specified here <https://lv2plug.in/pages/filesystem-hierarchy-standard.html>`_.

On Flatpak builds, Zrythm will scan for LV2 plugins
in the following paths.

- :file:`${HOME}/.lv2`
- :file:`/app/lib/lv2`
- :file:`/app/extensions/Plugins/lv2`

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

.. note:: If your system uses a libdir other than
   ``lib`` (for example ``lib64``), Zrythm will scan
   for plugins in both locations.

On Flatpak builds, Zrythm will scan for VST2 plugins
in the following paths

- :file:`/app/extensions/Plugins/lxvst`

and VST3 plugins in the following paths.

- :file:`/app/extensions/Plugins/vst3`

You can bypass this behavior by passing
:envvar:`VST_PATH` and :envvar:`VST3_PATH`,
respectively.

Windows
~~~~~~~
Zrythm will scan for VST plugins in the paths
specified in the
:ref:`preferences <configuration/preferences:Preferences>`.

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

DSSI Scan
---------
Zrythm will scan for DSSI plugins in the following
paths,

- :file:`/usr/local/lib/dssi`
- :file:`/usr/lib/dssi`

On Flatpak builds, Zrythm will scan for DSSI plugins
in the following paths.

- :file:`/app/extensions/Plugins/dssi`

You can bypass this behavior by passing
:envvar:`DSSI_PATH`.

.. note:: If your system uses a libdir other than
   ``lib`` (for example ``lib64``), Zrythm will scan
   for plugins in both locations.

LADSPA Scan
-----------
Zrythm will scan for LADSPA plugins in the following
paths,

- :file:`/usr/local/lib/ladspa`
- :file:`/usr/lib/ladspa`

On Flatpak builds, Zrythm will scan for LADSPA
plugins in the following paths.

- :file:`/app/extensions/Plugins/ladspa`

You can bypass this behavior by passing
:envvar:`LADSPA_PATH`.

.. note:: If your system uses a libdir other than
   ``lib`` (for example ``lib64``), Zrythm will scan
   for plugins in both locations.

AU Scan
-------
On MacOS, :term:`AU` plugins will be scanned at
their standard location at
:file:`/Library/Audio/Plug-Ins/Components`.

SFZ/SF2 Scan
------------
:term:`SFZ` and :term:`SF2` instruments will be
scanned in all  directories
and subdirectories specified in the
:ref:`preferences <configuration/preferences:Preferences>`.

About Flatpak
-------------

When using Flatpak builds, only plugins installed
as Flatpaks should be used (i.e., plugins using the
`Linux Audio base extension <https://github.com/flathub/org.freedesktop.LinuxAudio.BaseExtension>`_.

.. warning:: While Zrythm allows the user to use plugins not
   packaged as Flatpaks using the environment variables above,
   this is not recommended and we do not offer support if it
   causes issues.

.. note:: Flatpak builds have no access to :file:`/usr`
   so it is not possible to use system plugins, even when
   using the environment variables above.
