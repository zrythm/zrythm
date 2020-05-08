.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _left-panel:

Left Panel
==========

The left panel contains the inspectors and track visibility
options.

.. image:: /_static/img/left-panel.png
   :align: center

Track Inspector
---------------

The first tab is the track inspector, which shows various
options about the selected track.

.. image:: /_static/img/track-inspector.png
   :align: center

Track Properties
~~~~~~~~~~~~~~~~

.. image:: /_static/img/track-properties.png
   :align: center

Track Name
  Name of the track
Direct Out
  The track that this track routes is output to.
Instrument
  If the track is an :ref:`instrument-track`, the instrument
  plugin for this track.

Track Inputs
~~~~~~~~~~~~

.. image:: /_static/img/track-inputs.png
   :align: center

See :ref:`track-inputs` for more information.

MIDI FX/Inserts
~~~~~~~~~~~~~~~
These are slots for dropping audio or MIDI effects that will
be applied to the signal as it passes through the track.

.. image:: /_static/img/midi-fx-inserts.png
   :align: center

See the :ref:`tracks` chapter for more information.

Aux Sends
~~~~~~~~~
`Aux sends <https://en.wikipedia.org/wiki/Aux-send>`_ to
other tracks or plugin side-chain inputs.

.. image:: /_static/img/track-sends.png
   :align: center

See the :ref:`tracks` chapter for more information.

Fader
~~~~~
Fader section to control the volume and stereo balance.

.. image:: /_static/img/track-fader.png
   :align: center

Peak
  Peak signal value.
RMS
  Root Mean Square of the signal value.

Comments
~~~~~~~~
User comments.

.. image:: /_static/img/track-comments.png
   :align: center

.. note:: This is a TODO feature.

Plugin Inspector
----------------
The plugin inspector shows various options for the selected
plugin.

.. image:: /_static/img/plugin-inspector.png
   :align: center

Plugin Properties
~~~~~~~~~~~~~~~~~
The plugin properties show the name and type of the plugin,
as well as preset and bank selectors.

.. image:: /_static/img/plugin-properties.png
   :align: center

Plugin Ports
~~~~~~~~~~~~
These are the ports that the plugin has.

.. image:: /_static/img/plugin-ports.png
   :align: center

Control input ports can be changed by clicking and dragging,
and can also be automated using automation tracks.
Ports can be routed to other ports anywhere
within Zrythm by double clicking on them.

Ports are explained in more detail in :ref:`ports`.

.. note:: Depending on the type of plugin, there may be
  different categories of ports being shown.

Track Visibility
----------------
The track visibility tab allows toggling the visibility of
tracks.

.. image:: /_static/img/track-visibility.png
   :align: center
