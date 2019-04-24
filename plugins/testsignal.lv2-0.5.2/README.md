testsignal.lv2
==============

testsignal.lv2 is an audio-plugin for generating test-signals
in [LV2](http://lv2plug.in) format.

It has 9 operation modes:
*   Sine Wave 1kHz
*   Square Wave 1kHz
*   Sine Sweep 20Hz to 20KHz (at most to samplerate / 2) in 10 seconds
*   Uniform White Noise
*   Gaussian Shaped White Noise
*   Pink Noise
*   Impulses (1 sample spike) 100Hz, 0dBFS
*   Impulses (1 sample spike) 1Hz, 0dBFS
*   Impulses (1 sample spike) 5s (0.2Hz), 0dBFS


The signal level can be varied between -24dBFS and -9dBFS and defaults to -18dBFS.
*   For sine level defines the peak-signal (RMS is identical)
*   For square-wave generator the level defines the peak-signal (RMS is +3dB)
*   For uniform white noise, the level defines the absolute peak (RMS is about -1.8dB below peak)
*   For Gaussian shaped white noise and pink-noise, the level sets the RMS (peak is unlimited, though usually less than +12dB above RMS)

Install
-------

Compiling this plugin requires the LV2 SDK, gnu-make and a c-compiler.

```bash
  git clone git://github.com/x42/testsignal.lv2.git
  cd testsignal.lv2
  make
  sudo make install PREFIX=/usr
```

Note to packagers: The Makefile honors `PREFIX` and `DESTDIR` variables as well
as `CFLAGS`, `LDFLAGS` and `OPTIMIZATIONS` (additions to `CFLAGS`).
