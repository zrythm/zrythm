.. Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>

   This file is part of Zrythm

   Zrythm is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   Zrythm is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU General Affero Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.

Automation Lanes
================

Tracks which have automatable controls, such
as Fader, Pan and parameters of Plugins they
contain will have an option to show their
Automation Lanes.

You can choose which parameter you want to
automate in each Automation Lane.

Types of Automatable Parameters
-------------------------------

Zrythm will draw the automation differently
depending on the type of the parameter being
automated. The following types of parameters
exist.

Float
  Parameters with a Float type can have a value
  of any decimal within their given range and
  are the most common ones. Zrythm will draw
  editable curves for these types of
  parameters.
Step
  Step parameters have values that can only
  be changed in increments. Zrythm will draw
  ladders for these types of parameters.
Boolean
  These types of parameters only have two
  values: On or Off. Zrythm will draw a big
  square for these.

Automation Points
-----------------

TODO

Editing Curves
--------------

TODO
