.. This is part of the Zrythm Manual.
   Copyright (C) 2020, 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Monitor Section
===============
The monitor section is used to edit global settings
related to mixing and routing signals. It is found
inside the
:ref:`Right Panel <zrythm-interface/right-panel:Right Panel>`.

.. figure:: /_static/img/monitor-section.png
   :align: center

   Monitor section

Channel Controls
----------------

The monitor section displays the number of tracks
currently soloed, muted or listened, along with
buttons to unsolo, unmute, or unlisten every track.

Global Levels
-------------

The monitor section includes global mute, listen
and dim level controls.

Mute Level
  The level at which muted tracks will be played
Listen Level
  The level at which listened tracks will be played
Dim Level
  The level at which unlistened tracks will be
  played when at least 1 track is listened

Monitor Output
--------------

In Zrythm, what you actually hear through your
speakers is not the output of the master track,
but the monitor output. The master track routes
its output to this special processor that processes
the signal before sending the output to the
speakers.

Monitor Knob
~~~~~~~~~~~~

The :guilabel:`Monitor` knob controls the output
volume. If set to 0dB, the volume coming from the
master track will remain unchanged.

Monitor Controls
~~~~~~~~~~~~~~~~

The following buttons exist to alter the signal
passing through the monitor processor.

Mono
  Sum the output to mono

  .. tip:: This is useful for checking how the mix
     sounds in mono.

Dim
  Dim the output to the dim level

Mute
  Mute the output

JACK Output
~~~~~~~~~~~

When using the :term:`JACK` backend, there will
be additional controls to route the output signal
to one or more user-specified devices, instead of
the default device used by JACK.

.. figure:: /_static/img/monitor-section-jack-output.png
   :align: center

   JACK output
