## Release Notes

- Fix some missing icons
- Update Japanese and French translations (thanks sub26nico and trebmuh)
- Add meson command to build dev docs
- Simplify MIDI note and region operations (internally)
- Add tests for midi notes and regions
- Make panels foldable by clicking on their tabs (#294)
- Add selection info bar for piano roll
- Add support for ALSA audio and MIDI (sequencer) backends
- Rewrite internal MIDI operations to be agnostic of the MIDI backend
- Fix issues with plugins crashing or not loading (due to missing compilation flags)
- Add live MIDI activity indicator to each track
- Fix MIDI note continuing to get played if moved while it's playing (WIP)
- Fix various minor bugs in the arrangers (ctrl+zoom, playhead z-position and others)
