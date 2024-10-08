# juce_clap_hosting

This repository contains a JUCE module, which allows the user to host
CLAP plugins within a JUCE app/plugin.

**Note: this software should be considered super-alpha! I'm hoping to 
get it working at a production level at some point, but we're definitely
not at that point just yet. If you're here because you want to host
CLAP plugins in your JUCE app, I'd recommend coming back in the next few
weeks/months to see how far along we've gotten. If you're here because
you'd like to contribute to the project, welcome and please read on!**

## Strategy

JUCE makes it fairly straightforward to add support for hosting new
or custom plugin formats, by creating subclasses of `juce::AudioPluginFormat`.
This module exports a single top-level class, `CLAPPluginFormat`, which
satisfies this requirement. The idea is that the end user should have a
very easy time of adding CLAP support to their app, ideally the only required change would be something like:
```cpp
pluginFormatManager.addDefaultFormats(); // registers default formats (VST3 and AU on Mac)
pluginFormatManager.addFormat (new juce::CLAPPluginFormat); // adds CLAP support!
```

## Development

This repository is set up so that when it is being worked with as a top-level
folder (i.e. not a submodule), it sets up an example project, based on JUCE's
[HostPluginDemo](https://github.com/juce-framework/JUCE/blob/master/examples/Plugins/HostPluginDemo.h). To build the example with CMake, run:
```bash
$ cmake -Bbuild
$ cmake --build build --target TestHostPlugin_Standalone
```

## Status

At the moment, the test host can scan for plugins and process audio
through the plugin (so far I've only tested with plugins that so stereo
input and output).

There's a long list of things TODO:
- Scanning:
  - Check the `CLAP_PATH` [environment variable](https://github.com/free-audio/clap/blob/main/include/clap/entry.h) for plugins to scan.
- Audio I/O
  - Check audio port configurations
- Add MIDI support
- Parameters
  - Implement `clapParamsClear()` and `clapParamsFlush()`
  - Improve `clapParamsRescan()` to actually use the rescanning flags
- Implement `clapRequestProcess()` and `clapRequestRestart()`
- Finish implementing CLAP state extension
- Implement CLAP GUI extension
- Implement CLAP latency extension
- Implement CLAP thread-check extension
- Implement CLAP render extension
- Implement CLAP tail extension

## License

This repository is licensed under the MIT license.
