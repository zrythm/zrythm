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
panel that can hold any number of modulators.

.. image:: /_static/img/modulators-tab.png
   :align: center

Modulators are associated with the
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
