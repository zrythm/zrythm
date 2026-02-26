// SPDX-FileCopyrightText: © 2020-2021, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_port.h"
#include "dsp/midi_port.h"
#include "dsp/parameter.h"
#include "dsp/port.h"
#include "dsp/processor_base.h"
#include "utils/icloneable.h"

namespace zrythm::structure::tracks
{

/**
 * Channel send.
 *
 * Sends route audio or MIDI signals from a channel to a destination port.
 * The destination must be an input port of the same type (Audio or MIDI).
 */
class ChannelSend : public QObject, public dsp::ProcessorBase
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::dsp::ProcessorParameter * amountParam READ amountParam CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::ProcessorParameter * enabledParam READ enabledParam CONSTANT)
  Q_PROPERTY (
    QVariant destinationPort READ destinationPort WRITE setDestinationPort
      NOTIFY destinationPortChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

  struct ChannelSendProcessingCaches
  {
    dsp::ProcessorParameter *     amount_param_{};
    dsp::ProcessorParameter *     enabled_param_{};
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
  ~ChannelSend () override;

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

  /**
   * @brief Whether this send is enabled.
   *
   * When disabled, the send outputs silence (audio) or no events (MIDI).
   */
  dsp::ProcessorParameter * enabledParam () const;

  /**
   * @brief QML accessor for destination port.
   */
  QVariant destinationPort () const;
  void     setDestinationPort (const QVariant &port);

  Q_SIGNAL void destinationPortChanged ();

  // ============================================================================

  // ============================================================================
  // ProcessorBase Interface
  // ============================================================================

  void custom_process_block (
    EngineProcessTimeInfo  time_nfo,
    const dsp::ITransport &transport,
    const dsp::TempoMap   &tempo_map) noexcept override;

  void custom_prepare_for_processing (
    const dsp::graph::GraphNode * node,
    units::sample_rate_t          sample_rate,
    nframes_t                     max_block_length) override;

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

  /**
   * @brief Returns the destination port reference, if set.
   */
  [[nodiscard]] auto destination_port () const { return destination_port_; }

  /**
   * @brief Sets the destination port.
   *
   * @param port The destination port reference.
   * @throws std::invalid_argument if the port is not an input port of a
   *         compatible type (Audio or MIDI matching this send's type).
   */
  void set_destination_port (dsp::PortUuidReference port);

  /**
   * @brief Clears the destination port.
   */
  void clear_destination_port ();

  /**
   * @brief Checks if this send has a destination configured.
   */
  bool has_destination () const { return destination_port_.has_value (); }

private:
  static constexpr auto kSignalTypeKey = "signalType"sv;
  static constexpr auto kIsPrefaderKey = "isPrefader"sv;
  static constexpr auto kDestinationPortKey = "destinationPort"sv;
  friend void           to_json (nlohmann::json &j, const ChannelSend &p);
  friend void           from_json (const nlohmann::json &j, ChannelSend &p);

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

  /**
   * @brief Reference to the destination input port.
   *
   * Must be an input port of the same type as signal_type_ (Audio or MIDI).
   */
  std::optional<dsp::PortUuidReference> destination_port_;

  // Processing caches
  std::unique_ptr<ChannelSendProcessingCaches> processing_caches_;
};

} // namespace zrythm::structure::tracks
