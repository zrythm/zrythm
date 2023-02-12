.. SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later

   This file incorporates work by the Audacity Team covered by
   the Creative Commons Attribution 3.0 license (specifically,
   the ASIO section).
   SPDX-License-Identifier: CC-BY-3.0

.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. sectionauthor:: Alexandros Theodotou <alex@zrythm.org>

Frequently Asked Questions
==========================

About
+++++

What is Zrythm?
---------------

Zrythm is software for composing, recording, editing, arranging,
mixing and mastering audio and MIDI data. Such software is
commonly referred to as a digital audio workstation (DAW).
Zrythm provides all the tools needed to create entire tracks.

How do I pronounce it?
----------------------

Zee-rhythm.

..
  How does Zrythm compare to other DAWs?
  --------------------------------------

  Freedom
  ~~~~~~~

  Most DAWs are proprietary. This means that they place restrictions
  on running, copying, distributing, studying, changing and
  improving them, and you are at the mercy of their developers.

  In contrast, Zrythm is *free/libre software* (*"free" as in "freedom"*).
  This means that Zrythm provides users with the following freedoms:

  * The freedom to run the program as you wish, for any purpose
  * The freedom to study how the program works and change it (access to the source code is a precondition for this)
  * The freedom to redistribute exact copies, even commercially
  * The freedom to distribute copies of your modified versions to others, even commercially

  .. important:: That the word Zrythm and the Zrythm logo are
     trademarks, so you have to abide by our trademark policy or
     remove them if you plan to distribute modified versions.

  Comparison with other libre DAWs
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Ardour
    Ardour is the most advanced libre DAW available and is
    great for recording and editing audio, but not as finetuned as
    Zrythm is for composing electronic music
  QTractor/Rosegarden
    Great basic feature set but in our view the
    user interface is not as intuitive as Zrythm
  LMMS
    Basic feature set and an easy-to-use interface, making it
    suitable for beginners, but lacks many features needed for
    professional music production

Is it free?
-----------

Zrythm is free as in *freedom* (:term:`Free software`).
If you purchase a binary from us, there is a cost involved but
you can also get Zrythm for free as in *free beer* if you build
it yourself or get it from a distribution.

When will v1 be released?
-------------------------

When all bugs and features marked as `v1rc` are completed:

* https://todo.sr.ht/~alextee/zrythm-feature
* https://todo.sr.ht/~alextee/zrythm-bug

Usage
+++++

Can I use ASIO?
---------------

*Short answer: Yes, but you have to do it yourself and you may
not distribute the build to anyone else.*

The ASIO technology was developed by German company Steinberg
and is protected by a licensing agreement which prevents
redistribution of its source code.

Zrythm contains code under AGPL/GPL which requires all source
code (including source code of used libraries) to be
disclosed, and therefore it is illegal to provide
builds with ASIO support.

However, Zrythm supports the JACK and RtAudio backends, which
optionally support ASIO. As we cannot distribute these libraries
with ASIO support along with Zrythm, you must obtain them
yourself (for example by building them from source with ASIO
support) and make Zrythm use those.

.. warning:: Zrythm with ASIO support is NON-DISTRIBUTABLE.
   You may NOT copy or distribute builds including ASIO support
   to anyone else. The build is strictly for your own personal
   (private or commercial) use. For the same reasons, the
   Zrythm team cannot distribute builds of Zrythm including
   ASIO support.

How do I add external plugins?
------------------------------

See :ref:`plugins-files/plugins/scanning:Scanning for Plugins` and
:ref:`plugins-files/plugins/plugin-browser:Instantiating Plugins`.

How do I import an audio/MIDI file?
-----------------------------------

See :ref:`plugins-files/audio-midi-files/file-browser:Importing Files`.
