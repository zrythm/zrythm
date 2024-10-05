// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/engine.h"

void
Transport::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("totalBars", total_bars_),
    make_field ("playheadPos", playhead_pos_), make_field ("cuePos", cue_pos_),
    make_field ("loopStartPos", loop_start_pos_),
    make_field ("loopEndPos", loop_end_pos_),
    make_field ("punchInPos", punch_in_pos_),
    make_field ("punchOutPos", punch_out_pos_), make_field ("range1", range_1_),
    make_field ("range2", range_2_), make_field ("hasRange", has_range_),
    make_field ("position", position_), make_field ("roll", roll_),
    make_field ("stop", stop_), make_field ("backward", backward_),
    make_field ("forward", forward_), make_field ("loopToggle", loop_toggle_),
    make_field ("recToggle", rec_toggle_));
}

void
AudioClip::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("name", name_), make_field ("fileHash", file_hash_),
    make_field ("bpm", bpm_), make_field ("bitDepth", bit_depth_),
    make_field ("useFlac", use_flac_), make_field ("samplerate", samplerate_),
    make_field ("poolId", pool_id_));
}

void
AudioPool::define_fields (const Context &ctx)
{
  serialize_fields (ctx, make_field ("clips", clips_));
}

void
ControlRoom::define_fields (const Context &ctx)
{
  serialize_fields (ctx, make_field ("monitorFader", monitor_fader_));
}

void
SampleProcessor::define_fields (const Context &ctx)
{
  serialize_fields (ctx, make_field ("fader", fader_));
}

void
HardwareProcessor::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("isInput", is_input_),
    make_field ("extAudioPorts", ext_audio_ports_, true),
    make_field ("extMidiPorts", ext_midi_ports_, true),
    make_field ("audioPorts", audio_ports_, true),
    make_field ("midiPorts", midi_ports_, true));
}

void
AudioEngine::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("transportType", transport_type_),
    make_field ("sampleRate", sample_rate_),
    make_field ("framesPerTick", frames_per_tick_),
    make_field ("monitorOut", monitor_out_),
    make_field ("midiEditorManualPress", midi_editor_manual_press_),
    make_field ("midiIn", midi_in_), make_field ("transport", transport_),
    make_field ("pool", pool_), make_field ("controlRoom", control_room_),
    make_field ("sampleProcessor", sample_processor_),
    make_field ("hwInProcessor", hw_in_processor_),
    make_field ("hwOutProcessor", hw_out_processor_));
}