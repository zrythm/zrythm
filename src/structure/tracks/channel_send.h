// SPDX-FileCopyrightText: © 2020-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_port.h"
#include "dsp/midi_port.h"
#include "dsp/parameter.h"
#include "dsp/processor_base.h"
#include "utils/icloneable.h"

namespace zrythm::structure::tracks
{

/**
 * Channel send.
 *
 * The actual connection is tracked separately by PortConnectionsManager.
 */
class ChannelSend : public QObject, public dsp::ProcessorBase
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::dsp::ProcessorParameter * amountParam READ amountParam CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

  struct ChannelSendProcessingCaches
  {
    dsp::ProcessorParameter *     amount_param_{};
    std::vector<dsp::AudioPort *> audio_ins_rt_;
    std::vector<dsp::AudioPort *> audio_outs_rt_;
    dsp::MidiPort *               midi_in_rt_{};
    dsp::MidiPort *               midi_out_rt_{};
  };

public:
  /**
   * @param slot Slot, used only in parameter/port names.
   */
  ChannelSend (
    dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
    dsp::PortType                                 signal_type,
    int                                           slot,
    bool                                          is_prefader,
    QObject *                                     parent = nullptr);
  Z_DISABLE_COPY_MOVE (ChannelSend)
  ~ChannelSend () override = default;

  // ============================================================================
  // QML Interface
  // ============================================================================

  /**
   * @brief Send amount (amplitude), 0 to 2 for audio, velocity multiplier for
   * MIDI.
   */
  dsp::ProcessorParameter * amountParam () const
  {
    return get_parameters ().front ().get_object_as<dsp::ProcessorParameter> ();
  }

  // ============================================================================

  // ============================================================================
  // ProcessorBase Interface
  // ============================================================================

  void custom_process_block (EngineProcessTimeInfo time_nfo) noexcept override;

  void custom_prepare_for_processing (
    sample_rate_t sample_rate,
    nframes_t     max_block_length) override;

  void custom_release_resources () override;

  // ============================================================================

  bool is_prefader () const { return is_prefader_; }
  bool is_audio () const { return signal_type_ == dsp::PortType::Audio; }
  bool is_midi () const { return signal_type_ == dsp::PortType::Midi; }

  dsp::AudioPort &get_stereo_in_port () const
  {
    assert (is_audio ());
    return *get_input_ports ().at (0).get_object_as<dsp::AudioPort> ();
  }
  dsp::MidiPort &get_midi_in_port () const
  {
    assert (is_midi ());
    return *get_input_ports ().front ().get_object_as<dsp::MidiPort> ();
  }
  dsp::AudioPort &get_stereo_out_port () const
  {
    assert (is_audio ());
    return *get_output_ports ().front ().get_object_as<dsp::AudioPort> ();
  }
  dsp::MidiPort &get_midi_out_port () const
  {
    assert (is_midi ());
    return *get_output_ports ().front ().get_object_as<dsp::MidiPort> ();
  }

private:
  static constexpr auto kSignalTypeKey = "signalType"sv;
  static constexpr auto kIsPrefaderKey = "isPrefader"sv;
  friend void           to_json (nlohmann::json &j, const ChannelSend &p)
  {
    to_json (j, static_cast<const dsp::ProcessorBase &> (p));
    j[kSignalTypeKey] = p.signal_type_;
    j[kIsPrefaderKey] = p.is_prefader_;
  }
  friend void from_json (const nlohmann::json &j, ChannelSend &p);

  friend void init_from (
    ChannelSend           &obj,
    const ChannelSend     &other,
    utils::ObjectCloneType clone_type);

private:
  dsp::PortType signal_type_;

  /**
   * @brief Whether this is a prefader send (as opposed to post-fader).
   */
  bool is_prefader_{};

  // Processing caches
  std::unique_ptr<ChannelSendProcessingCaches> processing_caches_;
};

} // namespace zrythm::structure::tracks
