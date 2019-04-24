Tuna.LV2
============

An musical instrument tuner with strobe characteristics in LV2 plugin format.

Install
-------

Compiling this plugin requires the LV2 SDK, gnu-make, a c-compiler,
libpango, libcairo and openGL (sometimes called: glu, glx, mesa).

```bash
  git clone git://github.com/x42/tuna.lv2.git
  cd tuna.lv2
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
# Just run the stand-alone app
x42-tuna
# Some info
man x42-tuna
```

Screenshots
-----------

![screenshot](https://raw.github.com/x42/tuna.lv2/master/img/tuna1.png "Tuna Tube")
![screenshot](https://raw.github.com/x42/tuna.lv2/master/img/tuna2.png "Spectrum display")

Operation Principle
-------------------

The idea behind tuna.lv2 is to rely on the same principle as strobe-tuners and track a signal's phase.
The advantage of this is that it is very reliable, accurate and works even at very low frequencies.
While many FFT-based tuners use overtones to increase accuracy and speed. tuna.lv2 always tracks
the fundamental of the note and can be used for accurate stretch-tuning.

The principle of mechanical strobe tuners is to simply spin a wheel which has regular markings painted on it.
Then shine a stroboscope-light on the wheel which flashes at the frequency of the input signal.

If the signal is 'in-tune' the light-flashes match the frequency of the marks on the wheel and every time
the marks are illuminated they will appear on the same position despite that the wheel is moving.

Tuna.lv2 models this behaviour using a delay-line and a delay-locked loop to track the changes of the phase.
The phase-change corresponds to what appears as moving-pattern on the mechanical tuner and is visualized in
the GUI of the plugin.

Tuna takes a two step approach:

First a rough DFT is used to determine the octave and rough frequency of the signal,
this corresponds to setting the speed of the wheel in the mechanical version.

The detected frequency is rounded to the nearest note on the given scale (by default 440Hz tempered)
and used to initialize the PLL.

This is only done for convenience of providing an 'auto-tuner'. Tuna.lv2  can be set to 'manual'
in which case the user will have to choose which Note or frequency to tune to.

The input signal is band-pass filtered around the target-frequency in order to reliably
extract the phase of the signal. The PLL is used to keep track of phase-changes and calculate
the actual frequency of the input signal.
