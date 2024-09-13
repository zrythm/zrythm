// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_FADER_H__
#define __AUDIO_FADER_H__

#include <atomic>

#include "dsp/audio_port.h"
#include "dsp/control_port.h"
#include "dsp/midi_port.h"
#include "utils/icloneable.h"
#include "utils/types.h"

#include <fmt/printf.h>

class StereoPorts;
class MidiPort;
class ControlPort;
class Channel;
class Port;
class ControlRoom;
class SampleProcessor;
class PortIdentifier;
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
 *
 * It does not necessarily have to correspond to a FaderWidget. It can be used
 * as a backend to KnobWidget's.
 */
class Fader final : public ICloneable<Fader>, public ISerializable<Fader>
{
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
  Fader () = default;

  /**
   * Creates a new fader.
   *
   * This assumes that the channel has no plugins.
   *
   * @param type The Fader::Type.
   * @param ch Channel, if this is a channel Fader.
   */
  Fader (
    Type              type,
    bool              passthrough,
    Track *           track,
    ControlRoom *     control_room,
    SampleProcessor * sample_processor);

  /**
   * Inits fader after a project is loaded.
   */
  ATTR_COLD void init_loaded (
    Track *           track,
    ControlRoom *     control_room,
    SampleProcessor * sample_processor);

  static Fader * find_from_port_identifier (const PortIdentifier &id);

  static std::unique_ptr<ControlPort> create_swap_phase_port (bool passthrough);

  /**
   * Appends the ports owned by fader to the given array.
   */
  void append_ports (std::vector<Port *> &ports) const;

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
  bool get_muted () const { return mute_->is_toggled (); }

  /**
   * Returns if the track is soloed.
   */
  ATTR_HOT bool get_soloed () const { return solo_->is_toggled (); }

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
  bool get_listened () const { return listen_->is_toggled (); }

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
  float get_amp () const { return amp_->control_; }

  /**
   * Gets whether mono compatibility is enabled.
   */
  bool get_mono_compat_enabled () const
  {
    return mono_compat_enabled_->is_toggled ();
  }

  /**
   * Sets whether mono compatibility is enabled.
   */
  void set_mono_compat_enabled (bool enabled, bool fire_events);

  /**
   * Gets whether mono compatibility is enabled.
   */
  bool get_swap_phase () const { return swap_phase_->is_toggled (); }

  /**
   * Sets whether mono compatibility is enabled.
   */
  void set_swap_phase (bool enabled, bool fire_events);

  float get_fader_val () const { return fader_val_; }

  float get_default_fader_val () const
  {
    return math_get_fader_val_from_amp (amp_->deff_);
  }

  std::string db_string_getter () const
  {
    return fmt::sprintf ("%.1f", math_amp_to_dbfs (amp_->control_));
  }

  Channel * get_channel () const;

  Track * get_track () const;

  void update_volume_and_fader_val ();

  /**
   * Clears all buffers.
   */
  ATTR_HOT void clear_buffers ();

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
   * Copy the fader values from another fader.
   *
   * Used when cloning channels.
   */
  void copy_values (const Fader &other);

  /**
   * Process the Fader.
   */
  ATTR_HOT void process (const EngineProcessTimeInfo time_nfo);

  bool is_in_active_project () const;

  static int fade_frames_for_type (Type type);

  void init_after_cloning (const Fader &other) override;

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

  /**
   * A control port that controls the volume in amplitude (0.0 ~ 1.5)
   */
  std::unique_ptr<ControlPort> amp_;

  /** A control Port that controls the balance
   * (0.0 ~ 1.0) 0.5 is center. */
  std::unique_ptr<ControlPort> balance_;

  /**
   * Control port for muting the (channel) fader.
   */
  std::unique_ptr<ControlPort> mute_;

  /** Soloed or not. */
  std::unique_ptr<ControlPort> solo_;

  /** Listened or not. */
  std::unique_ptr<ControlPort> listen_;

  /** Whether mono compatibility switch is enabled. */
  std::unique_ptr<ControlPort> mono_compat_enabled_;

  /** Swap phase toggle. */
  std::unique_ptr<ControlPort> swap_phase_;

  /**
   * L & R audio input ports, if audio.
   */
  std::unique_ptr<StereoPorts> stereo_in_;

  /**
   * L & R audio output ports, if audio.
   */
  std::unique_ptr<StereoPorts> stereo_out_;

  /**
   * MIDI in port, if MIDI.
   */
  std::unique_ptr<MidiPort> midi_in_;

  /**
   * MIDI out port, if MIDI.
   */
  std::unique_ptr<MidiPort> midi_out_;

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
};

/**
 * @}
 */

#endif
