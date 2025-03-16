// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_FADER_H__
#define __AUDIO_FADER_H__

#include <atomic>

#include "gui/dsp/audio_port.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/midi_port.h"

#include "utils/icloneable.h"
#include "utils/types.h"
#include <fmt/printf.h>

namespace zrythm::gui
{
class Channel;
};

class ControlRoom;
class SampleProcessor;
class Track;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define MONITOR_FADER (CONTROL_ROOM->monitor_fader_)

#define FADER_MAGIC 32548791
#define IS_FADER(f) (((Fader *) f)->magic_ == FADER_MAGIC)
#define IS_FADER_AND_NONNULL(f) ((f) && IS_FADER (f))

/** Causes loud volume in when < 1k. */
constexpr int FADER_DEFAULT_FADE_FRAMES_SHORT = 1024;
#define FADER_DEFAULT_FADE_FRAMES \
  (ZRYTHM_TESTING ? FADER_DEFAULT_FADE_FRAMES_SHORT : 8192)

/**
 * A Fader is a processor that is used for volume controls and pan.
 */
class Fader final
    : public QObject,
      public ICloneable<Fader>,
      public dsp::IProcessable,
      public utils::serialization::ISerializable<Fader>,
      public IPortOwner
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

  using Channel = zrythm::gui::Channel;

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
    PortRegistry     &port_registry,
    Type              type,
    bool              passthrough,
    Track *           track,
    ControlRoom *     control_room,
    SampleProcessor * sample_processor);

  /**
   * Inits fader after a project is loaded.
   */
  [[gnu::cold]] void init_loaded (
    PortRegistry     &port_registry,
    Track *           track,
    ControlRoom *     control_room,
    SampleProcessor * sample_processor);

  static ControlPort *
  create_swap_phase_port (PortRegistry &port_registry, bool passthrough);

  /**
   * Appends the ports owned by fader to the given array.
   */
  void append_ports (std::vector<Port *> &ports) const;

  std::string get_node_name () const override;

  /**
   * Sets the amplitude of the fader. (0.0 to 2.0)
   */
  void set_amp (float amp);

  /**
   * Sets the amp value with an undoable action.
   *
   * @param skip_if_equal Whether to skip the action
   *   if the amp hasn't changed.
   */
  void set_amp_with_action (float amp_from, float amp_to, bool skip_if_equal);

  /**
   * Adds (or subtracts if negative) to the amplitude
   * of the fader (clamped at 0.0 to 2.0).
   */
  void add_amp (float amp);

  void set_midi_mode (MidiFaderMode mode, bool with_action, bool fire_events);

  /**
   * Sets track muted and optionally adds the action
   * to the undo stack.
   */
  void set_muted (bool mute, bool fire_events);

  /**
   * Returns if the fader is muted.
   */
  bool get_muted () const { return get_mute_port ().is_toggled (); }

  /**
   * Returns if the track is soloed.
   */
  [[gnu::hot]] bool get_soloed () const
  {
    return get_solo_port ().is_toggled ();
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
  bool get_listened () const { return get_listen_port ().is_toggled (); }

  /**
   * Sets fader listen and optionally adds the action
   * to the undo stack.
   */
  void set_listened (bool listen, bool fire_events);

  /**
   * Sets track soloed and optionally adds the action
   * to the undo stack.
   */
  void set_soloed (bool solo, bool fire_events);

  /**
   * Gets the fader amplitude (not db)
   */
  float get_amp () const { return get_amp_port ().control_; }

  /**
   * Gets whether mono compatibility is enabled.
   */
  bool get_mono_compat_enabled () const
  {
    return get_mono_compat_enabled_port ().is_toggled ();
  }

  /**
   * Sets whether mono compatibility is enabled.
   */
  void set_mono_compat_enabled (bool enabled, bool fire_events);

  /**
   * Gets whether mono compatibility is enabled.
   */
  bool get_swap_phase () const { return get_swap_phase_port ().is_toggled (); }

  /**
   * Sets whether mono compatibility is enabled.
   */
  void set_swap_phase (bool enabled, bool fire_events);

  float get_fader_val () const { return fader_val_; }

  float get_default_fader_val () const
  {
    return utils::math::get_fader_val_from_amp (get_amp_port ().deff_);
  }

  std::string db_string_getter () const;

  Channel * get_channel () const;

  Track * get_track () const;

  void update_volume_and_fader_val ();

  /**
   * Clears all buffers.
   */
  [[gnu::hot]] void clear_buffers ();

  /**
   * Sets the fader levels from a normalized value
   * 0.0-1.0 (such as in widgets).
   */
  void set_fader_val (float fader_val);

  void set_fader_val_with_action_from_db (const std::string &str);

  /**
   * Disconnects all ports connected to the fader.
   */
  void disconnect_all ();
  /**
   * Process the Fader.
   */
  [[gnu::hot]] void process_block (EngineProcessTimeInfo time_nfo) override;

  bool is_in_active_project () const override;

  void set_port_metadata_from_owner (dsp::PortIdentifier &id, PortRange &range)
    const override;

  std::string
  get_full_designation_for_port (const dsp::PortIdentifier &id) const override;

  void on_control_change_event (
    const PortUuid            &port_uuid,
    const dsp::PortIdentifier &id,
    float                      val) override;

  bool should_bounce_to_master (utils::audio::BounceStep step) const override;

  static int fade_frames_for_type (Type type);

  bool has_audio_ports () const
  {
    return type_ == Type::AudioChannel || type_ == Type::Monitor
           || type_ == Type::SampleProcessor;
  }

  bool has_midi_ports () const { return type_ == Type::MidiChannel; }

  void
  init_after_cloning (const Fader &other, ObjectCloneType clone_type) override;

  ControlPort &get_amp_port () const
  {
    return *std::get<ControlPort *> (
      port_registry_->find_by_id_or_throw (amp_id_.value ()));
  }
  ControlPort &get_balance_port () const
  {
    return *std::get<ControlPort *> (
      port_registry_->find_by_id_or_throw (balance_id_.value ()));
  }
  ControlPort &get_mute_port () const
  {
    return *std::get<ControlPort *> (
      port_registry_->find_by_id_or_throw (mute_id_.value ()));
  }
  ControlPort &get_solo_port () const
  {
    return *std::get<ControlPort *> (
      port_registry_->find_by_id_or_throw (solo_id_.value ()));
  }
  ControlPort &get_listen_port () const
  {
    return *std::get<ControlPort *> (
      port_registry_->find_by_id_or_throw (listen_id_.value ()));
  }
  ControlPort &get_mono_compat_enabled_port () const
  {
    return *std::get<ControlPort *> (
      port_registry_->find_by_id_or_throw (mono_compat_enabled_id_.value ()));
  }
  ControlPort &get_swap_phase_port () const
  {
    return *std::get<ControlPort *> (
      port_registry_->find_by_id_or_throw (swap_phase_id_.value ()));
  }
  std::pair<AudioPort &, AudioPort &> get_stereo_in_ports () const
  {
    if (!has_audio_ports ())
      {
        throw std::runtime_error ("Not an audio fader");
      }
    auto * l = std::get<AudioPort *> (
      port_registry_->find_by_id_or_throw (stereo_in_left_id_.value ()));
    auto * r = std::get<AudioPort *> (
      port_registry_->find_by_id_or_throw (stereo_in_right_id_.value ()));
    return { *l, *r };
  }
  std::pair<AudioPort &, AudioPort &> get_stereo_out_ports () const
  {
    if (!has_audio_ports ())
      {
        throw std::runtime_error ("Not an audio fader");
      }
    auto * l = std::get<AudioPort *> (
      port_registry_->find_by_id_or_throw (stereo_out_left_id_.value ()));
    auto * r = std::get<AudioPort *> (
      port_registry_->find_by_id_or_throw (stereo_out_right_id_.value ()));
    return { *l, *r };
  }
  MidiPort &get_midi_in_port () const
  {
    return *std::get<MidiPort *> (
      port_registry_->find_by_id_or_throw (midi_in_id_.value ()));
  }
  MidiPort &get_midi_out_port () const
  {
    return *std::get<MidiPort *> (
      port_registry_->find_by_id_or_throw (midi_out_id_.value ()));
  }

  auto get_stereo_in_left_id () const
  {
    if (!has_audio_ports ())
      {
        throw std::logic_error ("Not an audio fader");
      }
    return stereo_in_left_id_.value ();
  }

  auto get_stereo_in_right_id () const
  {
    if (!has_audio_ports ())
      {
        throw std::logic_error ("Not an audio fader");
      }
    return stereo_in_right_id_.value ();
  }

  auto get_stereo_out_left_id () const
  {
    if (!has_audio_ports ())
      {
        throw std::logic_error ("Not an audio fader");
      }
    return stereo_out_left_id_.value ();
  }

  auto get_stereo_out_right_id () const
  {
    if (!has_audio_ports ())
      {
        throw std::logic_error ("Not an audio fader");
      }
    return stereo_out_right_id_.value ();
  }

  auto get_midi_in_id () const
  {
    if (!has_midi_ports ())
      {
        throw std::logic_error ("Not a MIDI fader");
      }
    return midi_in_id_.value ();
  }

  auto get_midi_out_id () const
  {
    if (!has_midi_ports ())
      {
        throw std::logic_error ("Not a MIDI fader");
      }
    return midi_out_id_.value ();
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /**
   * Volume in dBFS. (-inf ~ +6)
   */
  float volume_ = 0.f;

  /** Used by the phase knob (0.0 ~ 360.0). */
  float phase_ = 0.f;

  /** 0.0 ~ 1.0 for widgets. */
  float fader_val_ = 0.f;

  /**
   * Value of @ref amp during last processing.
   *
   * Used when processing MIDI faders.
   *
   * TODO
   */
  float last_cc_volume_ = 0.f;

private:
  /**
   * A control port that controls the volume in amplitude (0.0 ~ 1.5)
   */
  std::optional<PortUuid> amp_id_;

  /** A control Port that controls the balance
   * (0.0 ~ 1.0) 0.5 is center. */
  std::optional<PortUuid> balance_id_;

  /**
   * Control port for muting the (channel) fader.
   */
  std::optional<PortUuid> mute_id_;

  /** Soloed or not. */
  std::optional<PortUuid> solo_id_;

  /** Listened or not. */
  std::optional<PortUuid> listen_id_;

  /** Whether mono compatibility switch is enabled. */
  std::optional<PortUuid> mono_compat_enabled_id_;

  /** Swap phase toggle. */
  std::optional<PortUuid> swap_phase_id_;

  /**
   * Audio input ports, if audio.
   */
  std::optional<PortUuid> stereo_in_left_id_;
  std::optional<PortUuid> stereo_in_right_id_;

  /**
   * Audio output ports, if audio.
   */
  std::optional<PortUuid> stereo_out_left_id_;
  std::optional<PortUuid> stereo_out_right_id_;

  /**
   * MIDI in port, if MIDI.
   */
  std::optional<PortUuid> midi_in_id_;

  /**
   * MIDI out port, if MIDI.
   */
  std::optional<PortUuid> midi_out_id_;

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
  ControlRoom * control_room_ = nullptr;

  /** Pointer to owner sample processor, if any. */
  SampleProcessor * sample_processor_ = nullptr;

  int magic_ = FADER_MAGIC;

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

  OptionalRef<PortRegistry> port_registry_;
};

/**
 * @}
 */

#endif
