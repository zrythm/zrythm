// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>

#include "dsp/audio_port.h"
#include "dsp/midi_port.h"
#include "dsp/parameter.h"
#include "utils/icloneable.h"
#include "utils/types.h"

#include <fmt/printf.h>

namespace zrythm::engine::session
{
class ControlRoom;
class SampleProcessor;
}

#define MONITOR_FADER (CONTROL_ROOM->monitor_fader_)

/** Causes loud volume in when < 1k. */
constexpr int FADER_DEFAULT_FADE_FRAMES_SHORT = 1024;
#define FADER_DEFAULT_FADE_FRAMES \
  (ZRYTHM_TESTING ? FADER_DEFAULT_FADE_FRAMES_SHORT : 8192)

namespace zrythm::structure::tracks
{

class Channel;
class Track;

/**
 * A Fader is a processor that is used for volume controls and pan.
 */
class Fader final : public QObject, public dsp::graph::IProcessable
{
  Q_OBJECT
  QML_ELEMENT

public:
  /**
   * Fader type.
   */
  enum class Type
  {
    None,

    /** Audio fader for the monitor. */
    Monitor,

    /** Audio fader for the sample processor. */
    SampleProcessor,

    /** Audio fader for Channel's. */
    AudioChannel,

    /* MIDI fader for Channel's. */
    MidiChannel,

    /** For generic uses. */
    Generic,
  };

  enum class MidiFaderMode
  {
    /** Multiply velocity of all MIDI note ons. */
    MIDI_FADER_MODE_VEL_MULTIPLIER,

    /** Send CC volume event on change TODO. */
    MIDI_FADER_MODE_CC_VOLUME,
  };

public:
  Fader (QObject * parent = nullptr);

  /**
   * Creates a new fader.
   *
   * This assumes that the channel has no plugins.
   *
   * @param type The Fader::Type.
   * @param ch Channel, if this is a channel Fader.
   */
  Fader (
    dsp::PortRegistry                 &port_registry,
    dsp::ProcessorParameterRegistry   &param_registry,
    Type                               type,
    bool                               passthrough,
    Track *                            track,
    engine::session::ControlRoom *     control_room,
    engine::session::SampleProcessor * sample_processor);

  /**
   * Inits fader after a project is loaded.
   */
  [[gnu::cold]] void init_loaded (
    dsp::PortRegistry                 &port_registry,
    dsp::ProcessorParameterRegistry   &param_registry,
    Track *                            track,
    engine::session::ControlRoom *     control_room,
    engine::session::SampleProcessor * sample_processor);

  /**
   * Appends the ports owned by fader to the given array.
   */
  void append_ports (std::vector<dsp::Port *> &ports) const;

  utils::Utf8String get_node_name () const override;

  /**
   * Sets the amp value with an undoable action.
   *
   * @param skip_if_equal Whether to skip the action
   *   if the amp hasn't changed.
   */
  void set_amp_with_action (float amp_from, float amp_to, bool skip_if_equal);

  void set_midi_mode (MidiFaderMode mode, bool with_action, bool fire_events);

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
   * Returns whether the fader is not soloed on its
   * own but its direct out (or its direct out's direct
   * out, etc.) or its child (or its children's child,
   * etc.) is soloed.
   */
  bool get_implied_soloed () const;

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
    return amp_param.range ().convert_from_0_to_1 (amp_param.currentValue ());
  }

  float get_default_fader_val () const
  {
    return utils::math::get_fader_val_from_amp (get_amp_param ().range ().deff_);
  }

  std::string db_string_getter () const;

  Channel * get_channel () const;

  Track * get_track () const;

  /**
   * Clears all buffers.
   */
  [[gnu::hot]] void clear_buffers (std::size_t block_length);

  void set_fader_val_with_action_from_db (const std::string &str);

  /**
   * Process the Fader.
   */
  [[gnu::hot]] void process_block (EngineProcessTimeInfo time_nfo) override;

  utils::Utf8String get_full_designation_for_port (const dsp::Port &port) const;

  static int fade_frames_for_type (Type type);

  bool has_audio_ports () const
  {
    return type_ == Type::AudioChannel || type_ == Type::Monitor
           || type_ == Type::SampleProcessor;
  }

  bool has_midi_ports () const { return type_ == Type::MidiChannel; }

  friend void
  init_from (Fader &obj, const Fader &other, utils::ObjectCloneType clone_type);

  dsp::ProcessorParameter &get_amp_param () const
  {
    return *amp_id_->get_object_as<dsp::ProcessorParameter> ();
  }
  dsp::ProcessorParameter &get_balance_param () const
  {
    return *balance_id_->get_object_as<dsp::ProcessorParameter> ();
  }
  dsp::ProcessorParameter &get_mute_param () const
  {
    return *mute_id_->get_object_as<dsp::ProcessorParameter> ();
  }
  dsp::ProcessorParameter &get_solo_param () const
  {
    return *solo_id_->get_object_as<dsp::ProcessorParameter> ();
  }
  dsp::ProcessorParameter &get_listen_param () const
  {
    return *listen_id_->get_object_as<dsp::ProcessorParameter> ();
  }
  dsp::ProcessorParameter &get_mono_compat_enabled_param () const
  {
    return *std::get<dsp::ProcessorParameter *> (
      mono_compat_enabled_id_->get_object ());
  }
  dsp::ProcessorParameter &get_swap_phase_param () const
  {
    return *swap_phase_id_->get_object_as<dsp::ProcessorParameter> ();
  }
  std::pair<dsp::AudioPort &, dsp::AudioPort &> get_stereo_in_ports () const
  {
    if (!has_audio_ports ())
      {
        throw std::runtime_error ("Not an audio fader");
      }
    auto * l = std::get<dsp::AudioPort *> (stereo_in_left_id_->get_object ());
    auto * r = std::get<dsp::AudioPort *> (stereo_in_right_id_->get_object ());
    return { *l, *r };
  }
  std::pair<dsp::AudioPort &, dsp::AudioPort &> get_stereo_out_ports () const
  {
    if (!has_audio_ports ())
      {
        throw std::runtime_error ("Not an audio fader");
      }
    auto * l = std::get<dsp::AudioPort *> (stereo_out_left_id_->get_object ());
    auto * r = std::get<dsp::AudioPort *> (stereo_out_right_id_->get_object ());
    return { *l, *r };
  }
  dsp::MidiPort &get_midi_in_port () const
  {
    return *std::get<dsp::MidiPort *> (midi_in_id_->get_object ());
  }
  dsp::MidiPort &get_midi_out_port () const
  {
    return *std::get<dsp::MidiPort *> (midi_out_id_->get_object ());
  }

  auto get_stereo_in_left_id () const
  {
    if (!has_audio_ports ())
      {
        throw std::logic_error ("Not an audio fader");
      }
    return stereo_in_left_id_.value ().id ();
  }

  auto get_stereo_in_right_id () const
  {
    if (!has_audio_ports ())
      {
        throw std::logic_error ("Not an audio fader");
      }
    return stereo_in_right_id_.value ().id ();
  }

  auto get_stereo_out_left_id () const
  {
    if (!has_audio_ports ())
      {
        throw std::logic_error ("Not an audio fader");
      }
    return stereo_out_left_id_.value ().id ();
  }

  auto get_stereo_out_right_id () const
  {
    if (!has_audio_ports ())
      {
        throw std::logic_error ("Not an audio fader");
      }
    return stereo_out_right_id_.value ().id ();
  }

  auto get_midi_in_id () const
  {
    if (!has_midi_ports ())
      {
        throw std::logic_error ("Not a MIDI fader");
      }
    return midi_in_id_.value ().id ();
  }

  auto get_midi_out_id () const
  {
    if (!has_midi_ports ())
      {
        throw std::logic_error ("Not a MIDI fader");
      }
    return midi_out_id_.value ().id ();
  }

private:
  static constexpr auto kTypeKey = "type"sv;
  static constexpr auto kVolumeKey = "volume"sv;
  static constexpr auto kAmpKey = "amp"sv;
  static constexpr auto kPhaseKey = "phase"sv;
  static constexpr auto kBalanceKey = "balance"sv;
  static constexpr auto kMuteKey = "mute"sv;
  static constexpr auto kSoloKey = "solo"sv;
  static constexpr auto kListenKey = "listen"sv;
  static constexpr auto kMonoCompatEnabledKey = "monoCompatEnabled"sv;
  static constexpr auto kSwapPhaseKey = "swapPhase"sv;
  static constexpr auto kMidiInKey = "midiIn"sv;
  static constexpr auto kMidiOutKey = "midiOut"sv;
  static constexpr auto kStereoInLKey = "stereoInL"sv;
  static constexpr auto kStereoInRKey = "stereoInR"sv;
  static constexpr auto kStereoOutLKey = "stereoOutL"sv;
  static constexpr auto kStereoOutRKey = "stereoOutR"sv;
  static constexpr auto kMidiModeKey = "midiMode"sv;
  static constexpr auto kPassthroughKey = "passthrough"sv;
  friend void           to_json (nlohmann::json &j, const Fader &fader)
  {
    j = nlohmann::json{
      { kTypeKey,              fader.type_                   },
      { kAmpKey,               fader.amp_id_                 },
      { kPhaseKey,             fader.phase_                  },
      { kBalanceKey,           fader.balance_id_             },
      { kMuteKey,              fader.mute_id_                },
      { kSoloKey,              fader.solo_id_                },
      { kListenKey,            fader.listen_id_              },
      { kMonoCompatEnabledKey, fader.mono_compat_enabled_id_ },
      { kSwapPhaseKey,         fader.swap_phase_id_          },
      { kMidiInKey,            fader.midi_in_id_             },
      { kMidiOutKey,           fader.midi_out_id_            },
      { kStereoInLKey,         fader.stereo_in_left_id_      },
      { kStereoInRKey,         fader.stereo_in_right_id_     },
      { kStereoOutLKey,        fader.stereo_out_left_id_     },
      { kStereoOutRKey,        fader.stereo_out_right_id_    },
      { kMidiModeKey,          fader.midi_mode_              },
      { kPassthroughKey,       fader.passthrough_            }
    };
  }
  friend void from_json (const nlohmann::json &j, Fader &fader);

public:
  /**
   * Volume in dBFS. (-inf ~ +6)
   */
  // float volume_ = 0.f;

  /** Used by the phase knob (0.0 ~ 360.0). */
  float phase_ = 0.f;

  /** 0.0 ~ 1.0 for widgets. */
  // float fader_val_ = 0.f;

  /**
   * Value of @ref amp during last processing.
   *
   * Used when processing MIDI faders.
   *
   * TODO
   */
  float last_cc_volume_ = 0.f;

private:
  OptionalRef<dsp::PortRegistry>               port_registry_;
  OptionalRef<dsp::ProcessorParameterRegistry> param_registry_;

  /**
   * A control port that controls the volume in amplitude (0.0 ~ 1.5)
   */
  std::optional<dsp::ProcessorParameterUuidReference> amp_id_;

  /** A control Port that controls the balance (0.0 ~ 1.0) 0.5 is center. */
  std::optional<dsp::ProcessorParameterUuidReference> balance_id_;

  /**
   * Control port for muting the (channel) fader.
   */
  std::optional<dsp::ProcessorParameterUuidReference> mute_id_;

  /** Soloed or not. */
  std::optional<dsp::ProcessorParameterUuidReference> solo_id_;

  /** Listened or not. */
  std::optional<dsp::ProcessorParameterUuidReference> listen_id_;

  /** Whether mono compatibility switch is enabled. */
  std::optional<dsp::ProcessorParameterUuidReference> mono_compat_enabled_id_;

  /** Swap phase toggle. */
  std::optional<dsp::ProcessorParameterUuidReference> swap_phase_id_;

  /**
   * Audio input ports, if audio.
   */
  std::optional<dsp::PortUuidReference> stereo_in_left_id_;
  std::optional<dsp::PortUuidReference> stereo_in_right_id_;

  /**
   * Audio output ports, if audio.
   */
  std::optional<dsp::PortUuidReference> stereo_out_left_id_;
  std::optional<dsp::PortUuidReference> stereo_out_right_id_;

  /**
   * MIDI in port, if MIDI.
   */
  std::optional<dsp::PortUuidReference> midi_in_id_;

  /**
   * MIDI out port, if MIDI.
   */
  std::optional<dsp::PortUuidReference> midi_out_id_;

public:
  /**
   * Current dBFS after processing each output port.
   *
   * Transient variables only used by the GUI.
   */
  float l_port_db_ = 0.f;
  float r_port_db_ = 0.f;

  Type type_ = (Type) 0;

  /** MIDI fader mode. */
  MidiFaderMode midi_mode_ = (MidiFaderMode) 0;

  /** Whether this is a passthrough fader (like a prefader). */
  bool passthrough_ = false;

  /** Pointer to owner track, if any. */
  Track * track_ = nullptr;

  /** Pointer to owner control room, if any. */
  engine::session::ControlRoom * control_room_ = nullptr;

  /** Pointer to owner sample processor, if any. */
  engine::session::SampleProcessor * sample_processor_ = nullptr;

  bool is_project_ = false;

  /* TODO Caches to be set when recalculating the
   * graph. */
  bool implied_soloed_ = false;
  bool soloed_ = false;

  /**
   * Number of samples left to fade out.
   *
   * This is atomic because it is used during processing (@ref process()) and
   * also checked in the main thread by @ref Engine.wait_for_pause().
   */
  std::atomic<int> fade_out_samples_ = 0;

  /** Number of samples left to fade in. */
  std::atomic<int> fade_in_samples_ = 0;

  /**
   * Whether currently fading out.
   *
   * When true, if fade_out_samples becomes 0 then the output will be silenced.
   */
  std::atomic<bool> fading_out_ = false;

  /** Cache. */
  bool was_effectively_muted_ = false;
};

}
