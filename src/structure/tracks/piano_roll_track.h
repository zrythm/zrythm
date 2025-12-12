// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/midi_event.h"
#include "utils/types.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::structure::tracks
{
/**
 * @brief Mixin for a piano roll-based track.
 */
class PianoRollTrackMixin : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    bool drumMode READ drumMode WRITE setDrumMode NOTIFY drumModeChanged)
  Q_PROPERTY (
    bool passthroughMidiInput READ passthroughMidiInput WRITE
      setPassthroughMidiInput NOTIFY passthroughMidiInputChanged)
  Q_PROPERTY (
    std::uint8_t midiChannel READ midiChannel WRITE setMidiChannel NOTIFY
      midiChannelChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")
public:
  PianoRollTrackMixin (QObject * parent = nullptr);
  Z_DISABLE_COPY_MOVE (PianoRollTrackMixin)
  ~PianoRollTrackMixin () override = default;

  // ========================================================================
  // QML Interface
  // ========================================================================

  std::uint8_t midiChannel () const { return midi_ch_; }
  void         setMidiChannel (std::uint8_t midi_ch)
  {
    midi_ch = std::clamp (
      midi_ch, static_cast<std::uint8_t> (1), static_cast<std::uint8_t> (16));
    if (midi_ch_ == midi_ch)
      return;

    midi_ch_ = midi_ch;
    Q_EMIT midiChannelChanged (midi_ch);
  }
  Q_SIGNAL void midiChannelChanged (std::uint8_t midi_ch);

  bool passthroughMidiInput () const { return passthrough_midi_input_; }
  void setPassthroughMidiInput (bool passthrough)
  {
    if (passthrough_midi_input_ == passthrough)
      return;

    passthrough_midi_input_ = passthrough;
    Q_EMIT passthroughMidiInputChanged (passthrough);
  }
  Q_SIGNAL void passthroughMidiInputChanged (bool passthrough);

  bool drumMode () const { return drum_mode_; }
  void setDrumMode (bool drum_mode)
  {
    if (drum_mode_ == drum_mode)
      return;

    drum_mode_ = drum_mode;
    Q_EMIT drumModeChanged (drum_mode);
  }
  Q_SIGNAL void drumModeChanged (bool drum_mode);

  // ========================================================================

  bool allow_only_specific_midi_channels () const
  {
    return midi_channels_.has_value ();
  }
  auto &midi_channels_to_allow () const { return midi_channels_.value (); }

  /**
   * @brief Callback to be used as TrackProcessor's transform function.
   */
  void transform_midi_inputs_func (dsp::MidiEventVector &events) const
  {
    // change the MIDI channel on the midi input to the channel set on the track
    if (!passthrough_midi_input_)
      {
        events.set_channel (midi_ch_);
      }
  }

private:
  static constexpr auto kDrumModeKey = "drumMode"sv;
  static constexpr auto kMidiChKey = "midiCh"sv;
  static constexpr auto kPassthroughMidiInputKey = "passthroughMidiInput"sv;
  static constexpr auto kMidiChannelsKey = "midiChannels"sv;
  friend void to_json (nlohmann::json &j, const PianoRollTrackMixin &track)
  {
    j[kDrumModeKey] = track.drum_mode_;
    j[kMidiChKey] = track.midi_ch_;
    j[kPassthroughMidiInputKey] = track.passthrough_midi_input_.load ();
    j[kMidiChannelsKey] = track.midi_channels_;
  }
  friend void from_json (const nlohmann::json &j, PianoRollTrackMixin &track)
  {
    j.at (kDrumModeKey).get_to (track.drum_mode_);
    j.at (kMidiChKey).get_to (track.midi_ch_);
    bool passthrough_midi_input{};
    j.at (kPassthroughMidiInputKey).get_to (passthrough_midi_input);
    track.passthrough_midi_input_.store (passthrough_midi_input);
    if (j.contains (kMidiChannelsKey))
      {
        j.at (kMidiChannelsKey).get_to (track.midi_channels_);
      }
  }

  friend void init_from (
    PianoRollTrackMixin       &obj,
    const PianoRollTrackMixin &other,
    utils::ObjectCloneType     clone_type);

private:
  /**
   * Whether drum mode in the piano roll is enabled for this track.
   */
  bool drum_mode_ = false;

  /** MIDI channel (1-16) */
  midi_byte_t midi_ch_ = 1;

  /**
   * If true, the input received will not be changed to the selected MIDI
   * channel.
   *
   * If false, all input received will have its channel changed to the
   * selected MIDI channel.
   */
  std::atomic_bool passthrough_midi_input_ = false;

  /**
   * 1 or 0 flags for each channel to enable it or disable it.
   *
   * If nullopt, the channel will accept MIDI messages from all MIDI channels.
   */
  std::optional<std::array<bool, 16>> midi_channels_;
};
}
