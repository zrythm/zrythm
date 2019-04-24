midifilter.lv2
==============

LV2 plugins to filter MIDI events.

So far 29 MIDI event filters have been implemented:

*   CC2Note -- translate control-commands to note-on/off messages
*   Channel Filter -- discard messages per channel
*   Channel Map -- map any MIDI-channel to another MIDI-channel
*   Enforce Scale -- force midi notes on given musical scale
*   Eventblocker -- notch style message filter. Suppress specific messages
*   Keyrange -- discard notes-on/off events outside a given range
*   Keysplit -- change midi-channel number depending on note (and optionally transpose)
*   MapCC -- change one control message into another
*   Mapscale -- flexible 12-tone note map
*   Mapkeychannel -- 12-tone channel map.
*   Chord -- harmonizer - create chords from a single note in a given musical scale
*   Delay -- delay MIDI events with optional randomization
*   Dup -- unisono - duplicate MIDI events from one channel to another
*   Strum -- arpeggio effect intended to simulate strumming a stringed instrument (e.g. guitar)
*   Transpose -- chromatic transpose MIDI notes
*   Legato -- hold a note until the next note arrives
*   NoSensing -- strip MIDI Active-Sensing events
*   NoDup -- MIDI duplicate blocker. Filter out overlapping note on/off and duplicate messages
*   Note2CC -- convert MIDI note-on messages to control change messages
*   NoteToggle -- toggle notes: play a note to turn it on, play it again to turn it off
*   nTabDelay -- repeat notes N times (incl tempo-ramps -- eurotechno hell yeah)
*   Simple 1 Channel Filter -- convenient MIDI channel filter
*   Passthru -- no operation, just pass the MIDI event through (example plugin)
*   Quantize -- live midi event quantization
*   Velocity Randomizer -- randomly change velocity of note-on events
*   ScaleCC -- modify the value (data-byte) of a MIDI control change message
*   Sostenuto -- delay note-off messages, emulate a piano sostenuto pedal
*   Velocity Range -- filter MIDI note events according to velocity
*   Velocity Scale -- modify note velocity by constant factor and offset

Install
-------

Compiling the plugins requires LV2 SDK, gnu-make and a c-compiler.

```bash
  git clone git://github.com/x42/midifilter.lv2.git
  cd midifilter.lv2
  make
  sudo make install PREFIX=/usr
```
