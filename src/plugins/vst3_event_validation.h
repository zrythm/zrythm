// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <string_view>

#include <pluginterfaces/vst/ivstevents.h>

namespace zrythm::plugins
{

/**
 * @brief Validates a plugin-emitted VST3 event against the host contract.
 *
 * Events must lie inside the current processing block, and note events must
 * carry in-range MIDI fields (the host forwards them to MIDI ports as raw
 * bytes). Events outside the contract are plugin bugs: the caller drops them
 * rather than propagating garbage (a negative sample offset would otherwise
 * wrap to a huge timestamp, and out-of-range float-to-byte conversions are
 * undefined).
 *
 * @param event The event from the plugin's output event list.
 * @param nframes Number of samples in the current processing block.
 * @return std::nullopt if the event is valid, otherwise a static string
 * describing the violation (suitable for logging).
 */
inline std::optional<std::string_view>
validate_vst3_output_event (
  const Steinberg::Vst::Event &event,
  Steinberg::int32             nframes)
{
  using namespace std::literals;

  if (event.sampleOffset < 0)
    return "negative sample offset"sv;
  if (event.sampleOffset >= nframes)
    return "sample offset beyond processing block"sv;

  const auto valid_note_fields =
    [] (Steinberg::int16 channel, Steinberg::int16 pitch, float velocity)
    -> std::optional<std::string_view> {
    if (channel < 0 || channel > 15)
      return "channel out of range"sv;
    if (pitch < 0 || pitch > 127)
      return "pitch out of range"sv;
    // Written as a negation so NaN velocities are rejected too
    if (!(velocity >= 0.0f && velocity <= 1.0f))
      return "velocity out of range"sv;
    return std::nullopt;
  };

  switch (event.type)
    {
    case Steinberg::Vst::Event::kNoteOnEvent:
      return valid_note_fields (
        event.noteOn.channel, event.noteOn.pitch, event.noteOn.velocity);
    case Steinberg::Vst::Event::kNoteOffEvent:
      return valid_note_fields (
        event.noteOff.channel, event.noteOff.pitch, event.noteOff.velocity);
    default:
      return std::nullopt;
    }
}

} // namespace zrythm::plugins
