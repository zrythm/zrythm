JUCETICE / EQinox v0.2.5 / 6 band para-graphic equalizer

Copyright (C) 2009 kRAkEn/gORe (www.anticore.org/jucetice)


Information-------------------------------------------------------------

EQinox EQ is a 6-band stereo para-graphic equalizer capable of shaping your
sound or sculpt your vocals. It provides a low shelf, 4 peaking filters and
a hi shelf: all filters have gain frequency and bandwidth controls.
This equalizer sounds very clean and crisp at high frequencies, and is good
for give some more air to your tracks. It sounds good even in the low
frequencies, but you have to be more careful on how you use the volume.
It will be updated constantly, so a lot of things may change, and even presets
could be different in the future.


License ---------------------------------------------------------------

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.


Changelog --------------------------------------------------------------

Version [0.2.5] - 2009-xx-xx

Version [0.2.4] - 2009-06-26
+ aligned to latest juce in svn repository
~ fixed jack and alsa dependancies for VST version

Version [0.2.3] - 2008-11-09
+ aligned to latest juce in svn repository
+ added juce_amalgamated instead of libjuce to improve build time (and to favour gcc optimizations)
+ added midi learn capability for parameters with reset (right click on the slider)
+ added standalone jack version
~ improved GUI drawing speed of the impulse response
~ improved internal class structure
~ decreased overall shared library size
~ fixed to work better with QTractor (latest cvs) and Renoise... i hope finally !

Version [0.2.2] - 2007-04-18
+ Finished adding tooltips on controls
~ Deep fix into juce linux binding !
~ The message thread was killed abruptly on plugins exit, now fixed
~ Updated the vst wrapper
~ Avoided a possible lock when plugin was deleted by the host

Version [0.2.1] - 2007-04-18
+ Multiple instance is now a REALITY (without crashes) !
+ The message thread for GUI plugins now is a singleton thread (cpu improvement)
+ Fixed problems with message manager not locked when it should
~ Removed linking to unused libraries (mainly fftw3, GLU/GL and Xinerama)
~ Now the frequency response graph responds to parameter automations
~ Improved colours and drawing of frequency response

Version [0.2.0] - 2007-04-13
+ Update sliders to be like JUCETICE sliders
+ Sliders now are parameter listeners and they listen independently to parameter changes
~ Reduced gui size and placed controls to be more organic
~ Removed the check for multiple instance (be calm and don't open 2 GUI in energyXT2!)
~ Reduced a bit the cpu in GUI paint method
~ Minor parameter bug fixes
~ Min/max gain for each band minimized so you can't BLOW your speakers (well you can anyway!)

Version [0.1.3] - 2007-03-02
~ Fixed some issues when the plugin process jack buffers directly (hangs)
~ Now the plugin works seamlessly with Jost
~ Raised output volume by a small amount

Version [0.1.2] - 2007-02-21
~ Fixed the problem with cpu spikes when the editor is open (there is still lot
  more to do, but actually this is the beta and i need some feedback)
~ Output now have analog style clipping, audible when overloading the input gain.
  In the future this will be selectable (to allow save some cpu cycles) when using low volume.

Version [0.1.1] - 2007-02-13
+ Added check for more than one instance
+ Introduced automatable parameters
~ Now parameter changes are saved along with the song
~ Fixed plugin unique ID bug

Version [0.1.0] - 2007-02-12
- Initial release

