// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>

#include "dsp/audio_port.h"
#include "dsp/midi_port.h"
#include "dsp/parameter.h"
#include "dsp/processor_base.h"
#include "utils/icloneable.h"
#include "utils/types.h"

namespace zrythm::structure::tracks
{
/**
 * A Fader is a processor that is used for volume controls and pan.
 */
class Fader : public QObject, public dsp::ProcessorBase
{
  Q_OBJECT
  Q_PROPERTY (
    MidiFaderMode midiMode READ midiMode WRITE setMidiMode NOTIFY midiModeChanged)
  Q_PROPERTY (zrythm::dsp::ProcessorParameter * gain READ gain CONSTANT)
  Q_PROPERTY (zrythm::dsp::ProcessorParameter * balance READ balance CONSTANT)
  Q_PROPERTY (zrythm::dsp::ProcessorParameter * mute READ mute CONSTANT)
  Q_PROPERTY (zrythm::dsp::ProcessorParameter * solo READ solo CONSTANT)
  Q_PROPERTY (zrythm::dsp::ProcessorParameter * listen READ listen CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::ProcessorParameter * monoToggle READ monoToggle CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::ProcessorParameter * swapPhaseToggle READ swapPhaseToggle
      CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  enum class MidiFaderMode : std::uint_fast8_t
  {
    /** Multiply velocity of all MIDI note ons. */
    MIDI_FADER_MODE_VEL_MULTIPLIER,

    /** Send CC volume event on change TODO. */
    MIDI_FADER_MODE_CC_VOLUME,
  };
  Q_ENUM (MidiFaderMode)

public:
  /**
   * @brief A callback to check if the fader should be muted based on various
   * external factors (such as solo status of other tracks).
   *
   * @note If the fader itself is muted this will not be called - fader is
   * assumed to be muted.
   *
   * @param current_solo_status The fader's own solo status.
   */
  using ShouldBeMutedCallback = std::function<bool (bool fader_solo_status)>;

  /**
   * @brief Callback to get the gain that should be used if the fader is
   * effectively muted.
   */
  using MuteGainCallback = std::function<float ()>;

  /**
   * @brief Callback to pre-process the incoming audio before applying gain,
   * pan, etc.
   *
   * Mainly for use by the monitor fader.
   */
  using PreProcessAudioCallback = std::function<void (
    std::pair<std::span<float>, std::span<float>> stereo_bufs,
    const EngineProcessTimeInfo                  &time_nfo)>;

  /**
   * Creates a new fader.
   *
   * @param hard_limit_output Whether the output should be hard-limited (only
   * applies to audio faders). This should be true for the master track fader,
   * the monitor fader and the sample processor (deprecated?) fader.
   * @param make_params_automatable Whether to make (a subset of) the parameters
   * automatable. If false, no parameter will be automatable.
   */
  Fader (
    dsp::ProcessorBase::ProcessorBaseDependencies      dependencies,
    dsp::PortType                                      signal_type,
    bool                                               hard_limit_output,
    bool                                               make_params_automatable,
    std::optional<std::function<utils::Utf8String ()>> owner_name_provider,
    ShouldBeMutedCallback                              should_be_muted_cb,
    QObject *                                          parent = nullptr);

  // ============================================================================
  // QML Interface
  // ============================================================================

  MidiFaderMode midiMode () const { return midi_mode_; }
  void          setMidiMode (MidiFaderMode mode)
  {
    if (midi_mode_ == mode)
      return;

    midi_mode_ = mode;
    Q_EMIT midiModeChanged (mode);
  }
  Q_SIGNAL void midiModeChanged (MidiFaderMode mode);

  zrythm::dsp::ProcessorParameter * gain () const
  {
    if (amp_id_.has_value ())
      {
        return &get_amp_param ();
      }
    return nullptr;
  }
  zrythm::dsp::ProcessorParameter * balance () const
  {
    if (balance_id_.has_value ())
      {
        return &get_balance_param ();
      }
    return nullptr;
  }
  zrythm::dsp::ProcessorParameter * mute () const
  {
    if (mute_id_.has_value ())
      {
        return &get_mute_param ();
      }
    return nullptr;
  }
  zrythm::dsp::ProcessorParameter * solo () const
  {
    if (solo_id_.has_value ())
      {
        return &get_solo_param ();
      }
    return nullptr;
  }
  zrythm::dsp::ProcessorParameter * listen () const
  {
    if (listen_id_.has_value ())
      {
        return &get_listen_param ();
      }
    return nullptr;
  }
  zrythm::dsp::ProcessorParameter * monoToggle () const
  {
    if (mono_compat_enabled_id_.has_value ())
      {
        return &get_mono_compat_enabled_param ();
      }
    return nullptr;
  }
  zrythm::dsp::ProcessorParameter * swapPhaseToggle () const
  {
    if (swap_phase_id_.has_value ())
      {
        return &get_swap_phase_param ();
      }
    return nullptr;
  }

  // ============================================================================

  /**
   * Returns if the fader is muted.
   */
  bool currently_muted () const
  {
    const auto &mute_param = get_mute_param ();
    return mute_param.range ().is_toggled (mute_param.currentValue ());
  }

  /**
   * Returns if the track is soloed.
   */
  [[gnu::hot]] bool currently_soloed () const
  {
    const auto &solo_param = get_solo_param ();
    return solo_param.range ().is_toggled (solo_param.currentValue ());
  }

  /**
   * Returns whether the fader is listened.
   */
  bool currently_listened () const
  {
    const auto &listened_param = get_listen_param ();
    return listened_param.range ().is_toggled (listened_param.currentValue ());
  }

  /**
   * Gets the fader amplitude (not db)
   */
  float get_current_amp () const
  {
    const auto &amp_param = get_amp_param ();
    return amp_param.range ().convertFrom0To1 (amp_param.currentValue ());
  }

  std::string db_string_getter () const;

  void set_mute_gain_callback (MuteGainCallback cb)
  {
    mute_gain_cb_ = std::move (cb);
  }

  void set_preprocess_audio_callback (PreProcessAudioCallback cb)
  {
    preprocess_audio_cb_ = std::move (cb);
  }

  // ============================================================================
  // ProcessorBase Interface
  // ============================================================================

  void custom_prepare_for_processing (
    sample_rate_t sample_rate,
    nframes_t     max_block_length) override;

  [[gnu::hot]] void
  custom_process_block (EngineProcessTimeInfo time_nfo) noexcept override;

  // ============================================================================

  bool is_audio () const { return signal_type_ == dsp::PortType::Audio; }
  bool is_midi () const { return signal_type_ == dsp::PortType::Midi; }

  friend void
  init_from (Fader &obj, const Fader &other, utils::ObjectCloneType clone_type);

  dsp::ProcessorParameter &get_amp_param () const
  {
    return *std::get<dsp::ProcessorParameter *> (
      dependencies ().param_registry_.find_by_id_or_throw (*amp_id_));
  }
  dsp::ProcessorParameter &get_balance_param () const
  {
    return *std::get<dsp::ProcessorParameter *> (
      dependencies ().param_registry_.find_by_id_or_throw (*balance_id_));
  }
  dsp::ProcessorParameter &get_mute_param () const
  {
    return *std::get<dsp::ProcessorParameter *> (
      dependencies ().param_registry_.find_by_id_or_throw (*mute_id_));
  }
  dsp::ProcessorParameter &get_solo_param () const
  {
    return *std::get<dsp::ProcessorParameter *> (
      dependencies ().param_registry_.find_by_id_or_throw (*solo_id_));
  }
  dsp::ProcessorParameter &get_listen_param () const
  {
    return *std::get<dsp::ProcessorParameter *> (
      dependencies ().param_registry_.find_by_id_or_throw (*listen_id_));
  }
  dsp::ProcessorParameter &get_mono_compat_enabled_param () const
  {
    return *std::get<dsp::ProcessorParameter *> (
      dependencies ().param_registry_.find_by_id_or_throw (
        *mono_compat_enabled_id_));
  }
  dsp::ProcessorParameter &get_swap_phase_param () const
  {
    return *std::get<dsp::ProcessorParameter *> (
      dependencies ().param_registry_.find_by_id_or_throw (*swap_phase_id_));
  }
  std::pair<dsp::AudioPort &, dsp::AudioPort &> get_stereo_in_ports () const
  {
    if (!is_audio ())
      {
        throw std::runtime_error ("Not an audio fader");
      }
    auto * l = get_input_ports ().at (0).get_object_as<dsp::AudioPort> ();
    auto * r = get_input_ports ().at (1).get_object_as<dsp::AudioPort> ();
    return { *l, *r };
  }
  std::pair<dsp::AudioPort &, dsp::AudioPort &> get_stereo_out_ports () const
  {
    if (!is_audio ())
      {
        throw std::runtime_error ("Not an audio fader");
      }
    auto * l = get_output_ports ().at (0).get_object_as<dsp::AudioPort> ();
    auto * r = get_output_ports ().at (1).get_object_as<dsp::AudioPort> ();
    return { *l, *r };
  }
  dsp::MidiPort &get_midi_in_port () const
  {
    return *get_input_ports ().front ().get_object_as<dsp::MidiPort> ();
  }
  dsp::MidiPort &get_midi_out_port () const
  {
    return *get_output_ports ().front ().get_object_as<dsp::MidiPort> ();
  }

private:
  static constexpr auto kMidiModeKey = "midiMode"sv;
  friend void           to_json (nlohmann::json &j, const Fader &fader)
  {
    to_json (j, static_cast<const dsp::ProcessorBase &> (fader));
    j[kMidiModeKey] = fader.midi_mode_;
  }
  friend void from_json (const nlohmann::json &j, Fader &fader);

  /**
   * @brief Initializes various parameter caches used during processing.
   *
   * @note This is only needed after construction. Logic that needs to be called
   * on sample rate/buffer size changes must go in prepare_for_processing().
   */
  void init_param_caches ();

  /**
   * @brief Calculates the gain based on the current gain parameter value and
   * the fader's mute/solo/etc. status.
   */
  float calculate_target_gain () const;

  bool effectively_muted () const
  {
    return currently_muted () || should_be_muted_cb_ (currently_soloed ());
  }

private:
  dsp::PortType signal_type_;

  bool hard_limit_output_{};

  /**
   * Value of @ref amp during last processing.
   *
   * Used when processing MIDI faders.
   *
   * TODO
   */
  float last_cc_volume_ = 0.f;

  /**
   * A control port that controls the volume in amplitude (0.0 ~ 1.5)
   */
  std::optional<dsp::ProcessorParameter::Uuid> amp_id_;

  /** A control Port that controls the balance (0.0 ~ 1.0) 0.5 is center. */
  std::optional<dsp::ProcessorParameter::Uuid> balance_id_;

  /**
   * Control port for muting the (channel) fader.
   */
  std::optional<dsp::ProcessorParameter::Uuid> mute_id_;

  /** Soloed or not. */
  std::optional<dsp::ProcessorParameter::Uuid> solo_id_;

  /** Listened or not. */
  std::optional<dsp::ProcessorParameter::Uuid> listen_id_;

  /** Whether mono compatibility switch is enabled. */
  std::optional<dsp::ProcessorParameter::Uuid> mono_compat_enabled_id_;

  /** Swap phase toggle. */
  std::optional<dsp::ProcessorParameter::Uuid> swap_phase_id_;

  /** MIDI fader mode. */
  MidiFaderMode midi_mode_{ MidiFaderMode::MIDI_FADER_MODE_VEL_MULTIPLIER };

  /**
   * @brief Current gain to apply after taking into account the gain parameter
   * and mute/listen/dim status.
   *
   * This is used to prevent artifacts when the actual gain quickly changes
   * (such as when muting/unmuting the fader).
   */
  juce::SmoothedValue<float> current_gain_{ 0.f };

  ShouldBeMutedCallback should_be_muted_cb_;

  std::optional<PreProcessAudioCallback> preprocess_audio_cb_;

  MuteGainCallback mute_gain_cb_ = [] () { return 0.f; };
};
}
