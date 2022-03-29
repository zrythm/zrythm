.. This is part of the Zrythm Manual.
   Copyright (C) 2020, 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. sectionauthor:: Alexandros Theodotou <alex@zrythm.org>

Modulators
==========
Modulators are any plugins that have at least one
:term:`CV` output port. These CV ports can be routed
to any automatable parameter inside Zrythm. For
example, you can route an LFO to a filter cutoff
parameter to modulate its value.

Modulators Tab
--------------
Zrythm has a modulators tab in the bottom
panel that can hold any number of modulators and
a fixed number of macro knobs.

.. image:: /_static/img/modulators-tab.png
   :align: center

Modulators and macro knobs are associated with the
:ref:`Modulator Track <tracks/track-types:Modulator Track>`,
which contains all modulators in the :term:`Project`.

Adding Modulators
-----------------
Modulator plugins can be drag-n-dropped from the
:doc:`../plugins-files/plugins/plugin-browser` into
the Modulators tab. Modulator plugins can be
:ref:`filtered inside the plugin browser <plugins-files/plugins/plugin-browser:Filter Buttons>`.

Routing Modulators
------------------
Modulator outputs can be routed to automatable
:term:`controls <Control>` by clicking on the
arrow and following the prompt to select the
destination.

.. image:: /_static/img/modulator-route.png
   :align: center

.. image:: /_static/img/modulator-route2.png
   :align: center

Macro Knobs
-----------

Macro knobs are simple controls that can be used to
control multiple parameters. They can be automated
in the
:ref:`Modulator Track <tracks/track-types:Modulator Track>`,
and you can also route envelopes like LFOs to them.

Adding Inputs
~~~~~~~~~~~~~

Click the :guilabel:`Add` button on the left side
of a macro knob to add inputs to modulate the macro
knob.

Routing to Outputs
~~~~~~~~~~~~~~~~~~

Click the arrow button on the right side of a macro
knob to route it to one or more controls. The process
is similar to modulator output routing.
