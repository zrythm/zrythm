Simple Scope
============

A simple audio oscilloscope with variable time scale, triggering, cursors
and numeric readout in LV2 plugin format.

The minimum grid resolution is 50 micro-seconds - or a 32 times oversampled
signal. The maximum buffer-time is 15 seconds.

Currently variants up to four channels are available.

For documentation please see http://x42.github.io/sisco.lv2/


Install
-------

Compiling this plugin requires the LV2 SDK, gnu-make, a c-compiler,
libpango, libcairo and openGL (sometimes called: glu, glx, mesa).

```bash
  git clone git://github.com/x42/sisco.lv2.git
  cd sisco.lv2
  make submodules
  make
  sudo make install PREFIX=/usr
```

Note to packagers: The Makefile honors `PREFIX` and `DESTDIR` variables as well
as `CFLAGS`, `LDFLAGS` and `OPTIMIZATIONS` (additions to `CFLAGS`), also
see the first 10 lines of the Makefile.
You really want to package the superset of [x42-plugins](https://github.com/x42/x42-plugins).

Usage
-------
```bash
# Just run the stand-alone jack app
x42-scope
# Some info
man x42-scope
```

Screenshots
-----------

![screenshot](https://raw.github.com/x42/sisco.lv2/master/img/sisco1.png "Single Channel Slow")
![screenshot](https://raw.github.com/x42/sisco.lv2/master/img/sisco4.png "Four Channels Variant")


Oscilloscope vs Waveform Display
--------------------------------

![screenshot](https://raw.github.com/x42/sisco.lv2/master/img/scopeVSwave.png "oscilloscope vs waveform")


Background Information
----------------------

This project was created to test and exemplify LV2 Atom-Vector communication
and demonstrate the short-comings of LV2 thread synchronization and LV2Atoms
for visualization UIs:

LV2Atom messages are written into a ringbuffer in the LVhost in the DSP-thread.
This ringbuffer is sent to the UI in another thread (jalv and ardour use
`g_timeout()` usually at 40ms ~ 25fps), and finally things are painted in the
main thread.

Accurate (low-latency, high-speed) visualization is a valid use-case for LV2
instance access in particular if visual sync to v-blank is of importance.
This is not the case for a scope. A ringbuffer using message-passing is sufficient
since signal acquisition is usually perform on a trigger condition and subject to
hold-off times.

The basic structure of this plugin is now available as eg05-scope example plugin
from the official lv2plug.in repository.

Compared to the example, this plugin goes to some length to add features in
order to make it use-able beyond simple visualization and make it useful
for scientific measurements. It is however still rather simple compared to
a fully fledged oscilloscope. See the TODO file included with the source.
