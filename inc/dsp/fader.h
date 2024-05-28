// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Backend for faders or other volume/gain controls.
 */

#ifndef __AUDIO_FADER_H__
#define __AUDIO_FADER_H__

#include "dsp/port.h"
#include "utils/types.h"
#include "utils/yaml.h"

typedef struct StereoPorts StereoPorts;
class Port;
typedef struct Channel         Channel;
typedef struct AudioEngine     AudioEngine;
typedef struct ControlRoom     ControlRoom;
typedef struct SampleProcessor SampleProcessor;
typedef struct PortIdentifier  PortIdentifier;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define FADER_SCHEMA_VERSION 2

#define MONITOR_FADER (CONTROL_ROOM->monitor_fader)

#define FADER_MAGIC 32548791
#define IS_FADER(f) (f->magic == FADER_MAGIC)
#define IS_FADER_AND_NONNULL(f) (f && f->magic == FADER_MAGIC)

/** Causes loud volume in when < 1k. */
#define FADER_DEFAULT_FADE_FRAMES_SHORT 1024
#define FADER_DEFAULT_FADE_FRAMES \
  (ZRYTHM_TESTING ? FADER_DEFAULT_FADE_FRAMES_SHORT : 8192)

#define FADER_FADE_FRAMES_FOR_TYPE(f) \
  ((f)->type == FaderType::FADER_TYPE_MONITOR \
     ? FADER_DEFAULT_FADE_FRAMES \
     : FADER_DEFAULT_FADE_FRAMES_SHORT)

#define fader_is_in_active_project(self) \
  ((self->track != NULL && track_is_in_active_project (self->track)) || (self->sample_processor != NULL && sample_processor_is_in_active_project (self->sample_processor)) || (self->control_room != NULL && control_room_is_in_active_project (self->control_room)))

/**
 * Fader type.
 */
enum class FaderType
{
  FADER_TYPE_NONE,

  /** Audio fader for the monitor. */
  FADER_TYPE_MONITOR,

  /** Audio fader for the sample processor. */
  FADER_TYPE_SAMPLE_PROCESSOR,

  /** Audio fader for Channel's. */
  FADER_TYPE_AUDIO_CHANNEL,

  /* MIDI fader for Channel's. */
  FADER_TYPE_MIDI_CHANNEL,

  /** For generic uses. */
  FADER_TYPE_GENERIC,
};

enum class MidiFaderMode
{
  /** Multiply velocity of all MIDI note ons. */
  MIDI_FADER_MODE_VEL_MULTIPLIER,

  /** Send CC volume event on change TODO. */
  MIDI_FADER_MODE_CC_VOLUME,
};

/**
 * A Fader is a processor that is used for volume controls and pan.
 *
 * It does not necessarily have to correspond to a FaderWidget. It can be used
 * as a backend to KnobWidget's.
 */
typedef struct Fader
{
  int schema_version;

  /**
   * Volume in dBFS. (-inf ~ +6)
   */
  float volume;

  /** Used by the phase knob (0.0 ~ 360.0). */
  float phase;

  /** 0.0 ~ 1.0 for widgets. */
  float fader_val;

  /**
   * Value of \ref amp during last processing.
   *
   * Used when processing MIDI faders.
   *
   * TODO
   */
  float last_cc_volume;

  /**
   * A control port that controls the volume in
   * amplitude (0.0 ~ 1.5)
   */
  Port * amp;

  /** A control Port that controls the balance
   * (0.0 ~ 1.0) 0.5 is center. */
  Port * balance;

  /**
   * Control port for muting the (channel)
   * fader.
   */
  Port * mute;

  /** Soloed or not. */
  Port * solo;

  /** Listened or not. */
  Port * listen;

  /** Whether mono compatibility switch is enabled. */
  Port * mono_compat_enabled;

  /** Swap phase toggle. */
  Port * swap_phase;

  /**
   * L & R audio input ports, if audio.
   */
  StereoPorts * stereo_in;

  /**
   * L & R audio output ports, if audio.
   */
  StereoPorts * stereo_out;

  /**
   * MIDI in port, if MIDI.
   */
  Port * midi_in;

  /**
   * MIDI out port, if MIDI.
   */
  Port * midi_out;

  /**
   * Current dBFS after processing each output port.
   *
   * Transient variables only used by the GUI.
   */
  float l_port_db;
  float r_port_db;

  FaderType type;

  /** MIDI fader mode. */
  MidiFaderMode midi_mode;

  /** Whether this is a passthrough fader (like a prefader). */
  bool passthrough;

  /** Pointer to owner track, if any. */
  Track * track;

  /** Pointer to owner control room, if any. */
  ControlRoom * control_room;

  /** Pointer to owner sample processor, if any. */
  SampleProcessor * sample_processor;

  int magic;

  bool is_project;

  /* TODO Caches to be set when recalculating the
   * graph. */
  bool implied_soloed;
  bool soloed;

  /** Number of samples left to fade in. */
  int fade_in_samples;

  /**
   * Number of samples left to fade out.
   */
  int fade_out_samples;

  /** Whether currently fading out.
   *
   * When true, if fade_out_samples becomes 0 then the output
   * will be silenced.
   */
  int fading_out;

  /** Cache. */
  bool was_effectively_muted;
} Fader;

/**
 * Inits fader after a project is loaded.
 */
COLD NONNULL_ARGS (1) void fader_init_loaded (
  Fader *           self,
  Track *           track,
  ControlRoom *     control_room,
  SampleProcessor * sample_processor);

/**
 * Creates a new fader.
 *
 * This assumes that the channel has no plugins.
 *
 * @param type The FaderType.
 * @param ch Channel, if this is a channel Fader.
 */
COLD Fader *
fader_new (
  FaderType         type,
  bool              passthrough,
  Track *           track,
  ControlRoom *     control_room,
  SampleProcessor * sample_processor);

Fader *
fader_find_from_port_identifier (const PortIdentifier * id);

Port *
fader_create_swap_phase_port (Fader * self, bool passthrough);

/**
 * Appends the ports owned by fader to the given
 * array.
 */
NONNULL void
fader_append_ports (const Fader * self, GPtrArray * ports);

/**
 * Sets the amplitude of the fader. (0.0 to 2.0)
 */
void
fader_set_amp (void * self, float amp);

/**
 * Sets the amp value with an undoable action.
 *
 * @param skip_if_equal Whether to skip the action
 *   if the amp hasn't changed.
 */
void
fader_set_amp_with_action (
  Fader * self,
  float   amp_from,
  float   amp_to,
  bool    skip_if_equal);

/**
 * Adds (or subtracts if negative) to the amplitude
 * of the fader (clamped at 0.0 to 2.0).
 */
void
fader_add_amp (void * self, float amp);

NONNULL void
fader_set_midi_mode (
  Fader *       self,
  MidiFaderMode mode,
  bool          with_action,
  bool          fire_events);

/**
 * Sets track muted and optionally adds the action
 * to the undo stack.
 */
void
fader_set_muted (Fader * self, bool mute, bool fire_events);

/**
 * Returns if the fader is muted.
 */
NONNULL bool
fader_get_muted (const Fader * const self);

/**
 * Returns if the track is soloed.
 */
HOT NONNULL bool
fader_get_soloed (const Fader * const self);

/**
 * Returns whether the fader is not soloed on its
 * own but its direct out (or its direct out's direct
 * out, etc.) or its child (or its children's child,
 * etc.) is soloed.
 */
bool
fader_get_implied_soloed (Fader * self);

/**
 * Returns whether the fader is listened.
 */
#define fader_get_listened(self) (control_port_is_toggled (self->listen))

/**
 * Sets fader listen and optionally adds the action
 * to the undo stack.
 */
void
fader_set_listened (Fader * self, bool listen, bool fire_events);

/**
 * Sets track soloed and optionally adds the action
 * to the undo stack.
 */
void
fader_set_soloed (Fader * self, bool solo, bool fire_events);

/**
 * Gets the fader amplitude (not db)
 * FIXME is void * necessary? do it in the caller.
 */
NONNULL float
fader_get_amp (void * self);

/**
 * Gets whether mono compatibility is enabled.
 */
bool
fader_get_mono_compat_enabled (Fader * self);

/**
 * Sets whether mono compatibility is enabled.
 */
void
fader_set_mono_compat_enabled (Fader * self, bool enabled, bool fire_events);

/**
 * Gets whether mono compatibility is enabled.
 */
bool
fader_get_swap_phase (Fader * self);

/**
 * Sets whether mono compatibility is enabled.
 */
void
fader_set_swap_phase (Fader * self, bool enabled, bool fire_events);

float
fader_get_fader_val (void * self);

float
fader_get_default_fader_val (void * self);

void
fader_db_string_getter (void * obj, char * buf);

Channel *
fader_get_channel (Fader * self);

NONNULL Track *
fader_get_track (Fader * self);

void
fader_update_volume_and_fader_val (Fader * self);

/**
 * Clears all buffers.
 */
HOT NONNULL void
fader_clear_buffers (Fader * self);

/**
 * Sets the fader levels from a normalized value
 * 0.0-1.0 (such as in widgets).
 */
void
fader_set_fader_val (Fader * self, float fader_val);

/**
 * Disconnects all ports connected to the fader.
 */
void
fader_disconnect_all (Fader * self);

/**
 * Copy the fader values from source to dest.
 *
 * Used when cloning channels.
 */
void
fader_copy_values (Fader * src, Fader * dest);

/**
 * Process the Fader.
 */
NONNULL HOT void
fader_process (Fader * self, const EngineProcessTimeInfo * const time_nfo);

#if 0
/**
 * Updates the track pos of the fader.
 */
void
fader_update_track_pos (
  Fader * self,
  int     pos);
#endif

Fader *
fader_clone (const Fader * src);

/**
 * Frees the fader members.
 */
void
fader_free (Fader * self);

/**
 * @}
 */

#endif
