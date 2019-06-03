fat1.lv2 - AutoTune-1
=====================

fat1.lv2 is an auto-tuner based on Fons Adriaensen's zita-at1.

See [zita-at1's quickguide](http://kokkinizita.linuxaudio.org/linuxaudio/zita-at1-doc/quickguide.html) for an introduction.

The only differences to zita-at1 is that the plugin reports its latency
to the host, saves the state with the session and the MIDI input has
sidechain semantics.


Install
-------

Compiling fat1.lv2 requires the LV2 SDK, fftw, gnu-make, and a c-compiler.

```bash
  git clone git://github.com/x42/fat1.lv2.git
  cd fat1.lv2
  make submodules
  make
  sudo make install PREFIX=/usr
```

Note to packagers: The Makefile honors `PREFIX` and `DESTDIR` variables as well
as `CFLAGS`, `LDFLAGS` and `OPTIMIZATIONS` (additions to `CFLAGS`), also
see the first 10 lines of the Makefile.


Screenshots
-----------

![screenshot](https://raw.github.com/x42/fat1.lv2/master/img/fat1_v1.png "Fat1 GUI")

