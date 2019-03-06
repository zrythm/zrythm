Copyright (C) 2018-2019 Alexandros Theodotou

This file is part of Zrythm

Zrythm is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Zrythm is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.

# Dependencies
### Dependencies as of Mar 6, 2019
- breeze icons (LGPLv3+): https://github.com/KDE/breeze-icons
- GTK+3 (library GPLv2+): https://gitlab.gnome.org/GNOME/gtk
- jack (LGPLv2.1+): http://jackaudio.org/
- libdazzle (GPLv3+): https://gitlab.gnome.org/GNOME/libdazzle
- lv2 (ISC): http://lv2plug.in/
- lilv (ISC): https://drobilla.net/software/lilv
- libsndfile (LGPLv3): http://www.mega-nerd.com/libsndfile
- libyaml
- libsamplerate (2-clause BSD): http://www.mega-nerd.com/libsamplerate
- portaudio (MIT): www.portaudio.com/

### Optional Dependencies
- ffmpeg (LGPL 2.1+, GPL 2+): https://ffmpeg.org/
- Qt5

Note: optional dependencies are turned on using
`--with-***`. See `./configure --help`

# Building & Installing
```
./autogen.sh
./configure
make -j8
sudo make install
```
This will generate build/zrythm. Note that it needs
to be installed first before running to install the
gschemas it requires.
