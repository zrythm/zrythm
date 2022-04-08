.. SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
   This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _quantization:

Quantization
============

Objects can be quantized to specified positions by using
the Quantize and Quick-Quantize actions.

.. image:: /_static/img/quantize-buttons.png
   :align: center

Quantize
--------
Clicking the Quantize button will open up a popup that
will let you specify the settings to quantize the current
selection with.

.. image:: /_static/img/quantize-options.png
   :align: center

Quantize to
  Select the note length and type to quantize to.
Adjust
  Select if you want to quantize the start positions, the
  end positions (on objects that have length), or both.
Amount
  Select the amount to quantize by. 100% will quantize to
  the given options fully, while 0% will perform no
  quantization.
Swing
  Select a swing amount.
Randomization
  This will randomize the resulting positions by the given
  amount of ticks, and is useful for adding a "human feel"
  to quantized events.

Clicking Quantize will perform the quantization and save
the settings for quick quantization with Quick-Quantize.

Quick-Quantize
--------------
This will perform quantization based on the last known
settings.
