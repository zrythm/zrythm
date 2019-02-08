```
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

```

# Dependencies
The dependencies as of Feb 8, 2019 are as follows:
```
breeze icons (LGPLv3+): https://github.com/KDE/breeze-icons
GTK+3 (library GPLv2+): https://gitlab.gnome.org/GNOME/gtk
jack (LGPLv2.1+): http://jackaudio.org/
libcyaml (ISC): https://github.com/tlsa/libcyaml
libdazzle (GPLv3+): https://gitlab.gnome.org/GNOME/libdazzle
libsmf (BSD): https://sourceforge.net/projects/libsmf/
suil (ISC): https://drobilla.net/software/suil/
lv2 (ISC): http://lv2plug.in/
lilv (ISC): https://drobilla.net/software/lilv
libsndfile (LGPLv3): http://www.mega-nerd.com/libsndfile
libsamplerate (2-clause BSD): http://www.mega-nerd.com/libsamplerate
libxml2 (MIT): http://www.xmlsoft.org/
portaudio (MIT): www.portaudio.com/
ffmpeg (LGPL 2.1+, GPL 2+): https://ffmpeg.org/
```
Other dependencies should show up when `configure` is run if they are missing.

# Compile
```
./autogen.sh
./configure
make
```
This will generate build/debug/zrythm or build/release/zrythm

# Run
```
build/debug/zrythm

# Install
```
sudo make install
```
