.. SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _track-inspector:

Track Inspector
===============

Each track will display its page in the inspector when
selected. Depending on the track, the properties shown
will be different.

.. figure:: /_static/img/track-inspector.png
   :align: center

   The track inspector

Track Properties
----------------

Track properties are basic properties that the track
has.

.. image:: /_static/img/track-properties.png
   :align: center

Track Name
  Name of the track. Can be changed by double
  clicking.
Direct Out
  The track that this track routes its output to.

  .. figure:: /_static/img/routing-to-groups.png
     :align: center

     Routing an instrument to an audio group

Instrument
  The instrument plugin from this track (only applicable to
  :ref:`instrument tracks <tracks/track-types:Instrument Track>`).

.. _track-inputs:

Inputs
------

If the track takes input, there will be an input
selection section. This is mainly used for
:ref:`playback-and-recording/recording:Recording`.

MIDI Inputs
~~~~~~~~~~~

.. figure:: /_static/img/track-inputs.png
   :align: center

   MIDI inputs on an instrument track

Input Device
  Device to read MIDI input from
MIDI Channels
  MIDI channels to listen to (other channels will be
  ignored)

Audio Inputs
~~~~~~~~~~~~

.. figure:: /_static/img/audio-track-inputs.png
   :align: center

   Audio inputs on an audio track

Left Input
  Left audio input port to listen to
Right Input
  Right audio input port to listen to
Mono toggle
  Duplicate the left signal on both channels
Gain knob
  Adjust input gain

MIDI FX/Inserts
---------------

These are slots for dropping audio or MIDI effects that will
be applied to the signal as it passes through the track.

.. figure:: /_static/img/midi-fx-inserts.png
   :align: center

   MIDI FX and insert slots

MIDI FX are processed after the input and piano roll events
and before the instruments in instrument tracks, or
before the inserts in MIDI tracks.

.. note:: Only MIDI and Instrument tracks support MIDI FX.

Inserts are processed in order. For instrument tracks,
the inserts will be added onto the signal coming from
the instrument, and for other tracks they will be added
on the incoming signal.

.. _track-sends:

Aux Sends
---------

These are
`aux sends <https://en.wikipedia.org/wiki/Aux-send>`_ to
other tracks or plugin
side-chain inputs. These are generally useful for
side-chaining or applying additional effects to
channels, such as reverb.

.. image:: /_static/img/track-sends.png
   :align: center

The first 6 slots are for pre-fader sends and the
last 3 slots are for post-fader sends.
The pre-fader slots will send the signal before
the fader is processed, and the post-fader slots
will send the signal after the fader is applied.

Fader
-----
The fader section is used to control the volume and stereo balance of a channel.

.. image:: /_static/img/track-fader.png
   :align: center

To change the fader or stereo balance amount, click and
drag their respective widgets. You can reset them to their
default positions with
:menuselection:`Right click --> Reset`.

The meter displays the amplitude of the signal in dBFS as
it is processed live. The peak value of the signal is also displayed
next to the meter for additional reference.

.. tip:: MIDI faders use MIDI volume CC (currently unimplemented - they let the signal pass through unchanged).

The following controls are available for controlling the signal
flow:

Record
  Arm the track for recording.
Mute
  Mute the track, meaning no signal will be sent to
  its direct out.

  .. warning:: The track will still be processed, so if
     you are looking to decrease :term:`DSP` usage,
     try :ref:`disabling the track <tracks/track-operations:Channel Section>` (or individual plugins) instead.

Solo
  Solo the track. If any track is soloed, only the
  soloed tracks will produce sounds.
Listen
  Similar to solo, except it dims the volume of other
  tracks instead of muting them. The dim amount can
  be controlled in the
  :ref:`Monitor section <routing/monitor-section:Monitor Section>`.
Monitor
  Listen to incoming signal when recording (only available on audio tracks).

.. Channel Settings
.. ~~~~~~~~~~~~~~~~

Comments
--------
User comments. This feature is useful for keeping
notes about tracks.

.. image:: /_static/img/track-comment.png
   :align: center

Clicking the pencil button will bring up a popup to
edit the comment.
