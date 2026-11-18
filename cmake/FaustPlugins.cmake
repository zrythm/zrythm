# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# Shared list of bundled Faust plugins, consumed by:
#   - data/plugins/CMakeLists.txt (Faust DSP/C++ generation)
#   - src/plugins/CMakeLists.txt (faust_registry.cpp generation)
#
# Entry format: "Name|Description|underscored_name|category|is_instrument"
#   - Name: display name (used for the generated .dsp file)
#   - Description: short description (used for the generated .dsp file)
#   - underscored_name: identifier used for file/class/factory names and the
#     plugin key ("zrythm.faust.<underscored_name>")
#   - category: a plugins::PluginCategory enumerator
#   - is_instrument: "true" or "false"
set(zrythm_faust_plugin_defs
  "Compressor|Basic compressor|compressor|Compressor|false"
  "Cubic Distortion|Cubic distortion|cubic_distortion|Distortion|false"
  "Flanger|Flanger effect|flanger|Flanger|false"
  "Gate Stereo|Stereo gate|gate_stereo|Gate|false"
  "Highpass Filter|2nd-order Butterworth highpass filter|highpass_filter|HighpassFilter|false"
  "Lowpass Filter|2nd-order Butterworth lowpass filter|lowpass_filter|LowpassFilter|false"
  "White Noise|White noise generator|white_noise|Generator|false"
  "Parametric EQ|Parametric equalizer with high/low shelves|parametric_eq|ParaEQ|false"
  "Peak Limiter|1176 Peak limiter|peak_limiter|Limiter|false"
  "Phaser|Phaser effect|phaser|Phaser|false"
  "Smooth Delay|Delay plugin|smooth_delay|Delay|false"
  "Triple Synth|Synth with 3 detuned oscillators|triple_synth|Instrument|true"
  "Wah4|Wah pedal|wah4|Filter|false"
  "Zita Rev1|Zita reverb algorithm|zita_rev1|Reverb|false")
