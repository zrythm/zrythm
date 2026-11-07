// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cassert>
#include <ranges>

#include "plugins/modulators/engines/midi_to_cv_engine.h"
#include "plugins/modulators/modulator_runtime_context.h"
#include "utils/hash.h"
#include "utils/midi.h"

namespace zrythm::plugins::modulators
{

std::unique_ptr<PluginDescriptor>
MidiToCvEngine::make_descriptor ()
{
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"MIDI to CV";
  descriptor->author_ = u8"Zrythm";
  descriptor->category_ = PluginCategory::MODULATOR;
  descriptor->category_str_ =
    PluginDescriptor::category_to_string (descriptor->category_);
  descriptor->protocol_ = Protocol::ProtocolType::Internal;
  descriptor->path_or_id_ =
    utils::Utf8String::from_utf8_encoded_string (kDescriptorId);
  descriptor->unique_id_ = static_cast<int64_t> (utils::hash::get_custom_hash (
    kDescriptorId.data (), kDescriptorId.size ()));
  descriptor->num_midi_ins_ = 1;
  descriptor->num_cv_outs_ = 3;
  return descriptor;
}

ModulatorLayout
MidiToCvEngine::layout () const
{
  return ModulatorLayout{
    .input_ports_ = { ModulatorPortSpec{
      .symbol_ = utils::Utf8String::from_utf8_encoded_string (kInputSymbol),
      .label_ = u8"MIDI In",
      .type_ = dsp::PortType::Midi,
    } },
    .output_ports_ = {
      ModulatorPortSpec{
        .symbol_ = utils::Utf8String::from_utf8_encoded_string (kPitchSymbol),
        .label_ = u8"Pitch",
        .type_ = dsp::PortType::CV,
      },
      ModulatorPortSpec{
        .symbol_ = utils::Utf8String::from_utf8_encoded_string (kGateSymbol),
        .label_ = u8"Gate",
        .type_ = dsp::PortType::CV,
      },
      ModulatorPortSpec{
        .symbol_ =
          utils::Utf8String::from_utf8_encoded_string (kVelocitySymbol),
        .label_ = u8"Velocity",
        .type_ = dsp::PortType::CV,
      },
    },
    .parameters_ = {},
  };
}

void
MidiToCvEngine::prepare (const ModulatorRuntimeContext &context) noexcept
{
  midi_in_ = context.midi_input (0);
  pitch_out_ = context.cv_output (0);
  gate_out_ = context.cv_output (1);
  velocity_out_ = context.cv_output (2);
  tracker_.prepare_resources ();
  held_note_ = -1;
  held_velocity_ = 0.f;
  assert (midi_in_ != nullptr && pitch_out_ != nullptr);
  assert (gate_out_ != nullptr && velocity_out_ != nullptr);
}

void
MidiToCvEngine::release_resources () noexcept
{
  midi_in_ = nullptr;
  pitch_out_ = nullptr;
  gate_out_ = nullptr;
  velocity_out_ = nullptr;
  tracker_.release_resources ();
  held_note_ = -1;
  held_velocity_ = 0.f;
}

void
MidiToCvEngine::process (
  dsp::graph::ProcessBlockInfo time_info,
  const dsp::ITransport &,
  const dsp::TempoMap &) noexcept
{
  const auto offset = time_info.buffer_offset_.in (units::samples);
  const auto nframes = time_info.nframes_.in (units::samples);

  float pitch = held_note_ >= 0 ? held_note_ / 127.f : 0.f;
  float gate = held_note_ >= 0 ? 1.f : 0.f;
  float velocity = held_velocity_;

  tracker_.collect (*midi_in_, offset, nframes);

  auto pitch_span = std::span (pitch_out_->buf_).subspan (offset, nframes);
  auto gate_span = std::span (gate_out_->buf_).subspan (offset, nframes);
  auto velocity_span = std::span (velocity_out_->buf_).subspan (offset, nframes);

  std::size_t segment_start = 0;
  const auto  fill_through = [&] (std::size_t end) {
    for (const auto i : std::views::iota (segment_start, end))
      {
        pitch_span[i] = pitch;
        gate_span[i] = gate;
        velocity_span[i] = velocity;
      }
    segment_start = end;
  };

  for (const auto &event : tracker_.events ())
    {
      const auto event_index = std::min<std::size_t> (
        event.time ().in<std::size_t> (units::samples) - offset, nframes - 1);
      fill_through (event_index);

      const auto data = event.data ();
      if (utils::midi::midi_is_note_on (data))
        {
          held_note_ =
            static_cast<int> (utils::midi::midi_get_note_number (data));
          pitch = held_note_ / 127.f;
          gate = 1.f;
          held_velocity_ = utils::midi::midi_get_velocity (data) / 127.f;
          velocity = held_velocity_;
        }
      else if (
        utils::midi::midi_is_note_off (data)
        && static_cast<int> (utils::midi::midi_get_note_number (data))
             == held_note_)
        {
          held_note_ = -1;
          gate = 0.f;
        }
      else if (utils::midi::midi_is_all_notes_off (data))
        {
          held_note_ = -1;
          gate = 0.f;
        }
    }
  fill_through (nframes);
}

} // namespace zrythm::plugins::modulators
