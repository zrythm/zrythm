.. SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _scanning-plugins:

Scanning for Plugins
====================

Zrythm will scan for plugins on startup and will remember those plugins until
it is closed. Zrythm supports `CLAP`_, :term:`LV2`, :term:`VST2`,
:term:`VST3`, LADSPA, DSSI and :term:`AU` plugins.

:term:`SFZ` and :term:`SF2` instruments are also supported, and they are
scanned as instrument plugins.

.. tip:: Plugin scanning can be disabled by passing the
   :envvar:`ZRYTHM_SKIP_PLUGIN_SCAN` environment variable when running Zrythm.

Scan Paths
----------

Zrythm will scan for plugins in the plugin search paths specified in the
:ref:`preferences <configuration/preferences:Preferences>`.
If the search path is left empty, Zrythm will scan for plugins in the
standard paths according to the specifications of each plugin format (if any).

.. hint:: You can use environment variables (e.g., ``LV2_PATH``) when
   specifying search paths in the preferences. The allowed syntax is
   ``${ENV_VAR_NAME}`` where ``ENV_VAR_NAME`` is the name of the environment
   variable.

Plugin Cache
------------

Zrythm will remember scanned VST plugins and save this information in
:file:`cached_plugin_descriptors.yaml` in the :ref:`Zrythm user path <configuration/preferences:Zrythm User Path>`.

New plugins will be scanned on each start-up, and you can delete or edit this
file to force a re-scan of previously scanned plugins.

AU Scan
-------
On MacOS, :term:`AU` plugins will be scanned at their standard location at
:file:`/Library/Audio/Plug-Ins/Components`.

SFZ/SF2 Scan
------------
:term:`SFZ` and :term:`SF2` instruments will be scanned in all directories
and subdirectories specified in the
:ref:`preferences <configuration/preferences:Preferences>`.

About Flatpak
-------------

When using Zrythm as a Flatpak, only Flatpak plugins should be used (i.e.,
plugins using the `Linux Audio base extension`_).

.. warning:: While Zrythm allows the user to use plugins not
   packaged as Flatpaks using the environment variables above,
   this is not recommended and we do not offer support if it
   causes issues.

.. note:: Flatpak builds have no access to :file:`/usr`
   so it is not possible to use system plugins, even when
   using the environment variables above.

.. _CLAP: https://cleveraudio.org/
.. _Linux Audio base extension: https://github.com/flathub/org.freedesktop.LinuxAudio.BaseExtension
