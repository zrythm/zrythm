/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Ports that transfer audio/midi/other signals to
 * one another.
 */

#ifndef __AUDIO_PORTS_H__
#define __AUDIO_PORTS_H__

#include "zrythm-config.h"

#include <stdbool.h>

#include "audio/port_identifier.h"
#include "plugins/lv2/lv2_evbuf.h"
#include "utils/types.h"
#include "zix/sem.h"

#include <lilv/lilv.h>

#ifdef HAVE_JACK
#include "weak_libjack.h"
#endif

#ifdef HAVE_RTMIDI
#include <rtmidi/rtmidi_c.h>
#endif

#ifdef HAVE_RTAUDIO
#include <rtaudio/rtaudio_c.h>
#endif

typedef struct Plugin Plugin;
typedef struct MidiEvents MidiEvents;
typedef struct Fader Fader;
typedef struct SampleProcessor SampleProcessor;
typedef struct PassthroughProcessor
  PassthroughProcessor;
typedef struct ZixRingImpl ZixRing;
typedef struct WindowsMmeDevice WindowsMmeDevice;
typedef struct Lv2Port Lv2Port;
typedef struct Channel Channel;
typedef struct Track Track;
typedef struct SampleProcessor SampleProcessor;
typedef struct TrackProcessor TrackProcessor;
typedef struct RtMidiDevice RtMidiDevice;
typedef struct RtAudioDevice RtAudioDevice;
typedef struct AutomationTrack AutomationTrack;
typedef struct TruePeakDsp TruePeakDsp;
typedef struct ExtPort ExtPort;
typedef struct AudioClip AudioClip;
typedef struct PluginGtkController
  PluginGtkController;
typedef enum PanAlgorithm PanAlgorithm;
typedef enum PanLaw PanLaw;

/**
 * @addtogroup audio
 *
 * @{
 */

#define PORT_SCHEMA_VERSION 1
#define STEREO_PORTS_SCHEMA_VERSION 1

#define PORT_MAGIC 456861194
#define IS_PORT(_p) \
  (((Port *) (_p))->magic == PORT_MAGIC)
#define IS_PORT_AND_NONNULL(x) \
  ((x) && IS_PORT (x))

#define FOREACH_SRCS(port) \
  for (int i = 0; i < port->num_srcs; i++)
#define FOREACH_DESTS(port) \
  for (int i = 0; i < port->num_dests; i++)

#define TIME_TO_RESET_PEAK 4800000

/**
 * Special ID for owner_pl, owner_ch, etc. to indicate that
 * the port is not owned.
 */
#define PORT_NOT_OWNED -1

/**
 * What the internal data is.
 */
typedef enum PortInternalType
{
  INTERNAL_NONE,

  /** Pointer to Lv2Port. */
  INTERNAL_LV2_PORT,

  /** Pointer to jack_port_t. */
  INTERNAL_JACK_PORT,

  /** TODO */
  INTERNAL_PA_PORT,

  /** Pointer to snd_seq_port_info_t. */
  INTERNAL_ALSA_SEQ_PORT,
} PortInternalType;

/**
 * Scale point.
 */
typedef struct PortScalePoint
{
  float  val;
  char * label;
} PortScalePoint;

int
port_scale_point_cmp (
  const void * _a,
  const void * _b);

/**
 * Must ONLY be created via port_new()
 */
typedef struct Port
{
  int                 schema_version;

  PortIdentifier      id;

  /**
   * Flag to indicate that this port is exposed
   * to the backend.
   */
  int                 exposed_to_backend;

  /**
   * Buffer to be reallocated every time the buffer
   * size changes.
   *
   * The buffer size is AUDIO_ENGINE->block_length.
   */
  float *             buf;

  /**
   * Contains raw MIDI data (MIDI ports only)
   */
  MidiEvents *        midi_events;

  /**
   * Inputs and Outputs.
   *
   * These should be serialized, and when loading
   * they shall be used to find the original ports
   * and replace the pointer (also freeing the
   * current one).
   */
  struct Port **      srcs;
  struct Port **      dests;
  PortIdentifier *    src_ids;
  PortIdentifier *    dest_ids;

  /** These are the multipliers for port connections.
   *
   * They range from 0.f to 1.f and the default is
   * 1.f. They correspond to each destination.
   */
  float *             multipliers;

  /** Same as above for sources. */
  float *             src_multipliers;

  /**
   * These indicate whether the destination Port
   * can be removed or the multiplier edited by the
   * user.
   *
   * These are ignored when connecting things
   * internally and are only used to deter the user
   * from breaking necessary connections.
   *
   * 0 == unlocked, 1 == locked.
   */
  int *               dest_locked;

  /** Same as above for sources. */
  int *               src_locked;

  /**
   * These indicate whether the connection is
   * enabled.
   *
   * The user can disable port connections only if
   * they are not locked.
   *
   * 0 == disabled (disconnected),
   * 1 == enabled (connected).
   */
  int *               dest_enabled;

  /** Same as above for sources. */
  int *               src_enabled;

  /** Counters. */
  int                 num_srcs;
  int                 num_dests;

  /** Allocated sizes. */
  size_t              srcs_size;
  size_t              dests_size;

  /**
   * Indicates whether data or lv2_port should be
   * used.
   */
  PortInternalType    internal_type;

  /**
   * Minimum, maximum and zero values for this
   * port.
   *
   * Note that for audio, this is the amp (0 - 2)
   * and not the actual values.
   */
  float               minf;
  float               maxf;

  /**
   * The zero position of the port.
   *
   * For example, in balance controls, this will
   * be the middle. In audio ports, this will be
   * 0 amp (silence), etc.
   */
  float               zerof;

  /** Default value, only used for controls. */
  float               deff;

  /** VST parameter index, if VST control port. */
  int                vst_param_id;

  /** Index of the control parameter (for Carla
   * plugin ports). */
  int                carla_param_id;

  /**
   * Pointer to arbitrary data.
   *
   * Use internal_type to check what data it is.
   *
   * FIXME just add the various data structs here
   * and remove this ambiguity.
   */
  void *              data;

#ifdef _WOE32
  /**
   * Connections to WindowsMmeDevices.
   *
   * These must be pointers to \ref
   * AudioEngine.mme_in_devs or \ref
   * AudioEngine.mme_out_devs and must not be
   * allocated or free'd.
   */
  WindowsMmeDevice *  mme_connections[40];
  int                 num_mme_connections;
#else
  void *              mme_connections[40];
  int                 num_mme_connections;
#endif

  /** Semaphore for changing the connections
   * atomically. */
  ZixSem              mme_connections_sem;

  /**
   * Last time the port finished dequeueing
   * MIDI events.
   *
   * Used for some backends only.
   */
  gint64              last_midi_dequeue;

#ifdef HAVE_RTMIDI
  /**
   * RtMidi pointers for input ports.
   *
   * Each RtMidi port represents a device, and this
   * Port can be connected to multiple devices.
   */
  RtMidiDevice *      rtmidi_ins[128];
  int                 num_rtmidi_ins;

  /** RtMidi pointers for output ports. */
  RtMidiDevice *      rtmidi_outs[128];
  int                 num_rtmidi_outs;
#else
  void *      rtmidi_ins[128];
  int                 num_rtmidi_ins;
  void *      rtmidi_outs[128];
  int                 num_rtmidi_outs;
#endif

#ifdef HAVE_RTAUDIO
  /**
   * RtAudio pointers for input ports.
   *
   * Each port can have multiple RtAudio devices.
   */
  RtAudioDevice *    rtaudio_ins[128];
  int                num_rtaudio_ins;
#else
  void *    rtaudio_ins[128];
  int                num_rtaudio_ins;
#endif

  /** Scale points. */
  PortScalePoint *   scale_points;
  int                num_scale_points;

  /**
   * The control value if control port, otherwise
   * 0.0f.
   *
   * FIXME for fader, this should be the
   * fader_val (0.0 to 1.0) and not the
   * amplitude.
   *
   * This value will be snapped (eg, if integer or
   * toggle).
   */
  float               control;

  /** Unsnapped value, used by widgets. */
  float               unsnapped_control;

  /** Flag that the value of the port changed from
   * reading automation. */
  bool                value_changed_from_reading;

  /**
   * Last timestamp the control changed.
   *
   * This is used when recording automation in
   * "touch" mode.
   */
  gint64              last_change;

  /* ====== flags to indicate port owner ====== */

  /**
   * Temporary plugin pointer (used when the
   * plugin doesn't exist yet in its supposed slot).
   */
  Plugin *            tmp_plugin;

  /**
   * Temporary track (used when the track doesn't
   * exist yet in its supposed position).
   */
  //Track *             tmp_track;

  SampleProcessor *   sample_processor;

  /** used when loading projects FIXME needed? */
  int                 initialized;

  /**
   * For control ports, when a modulator is
   * attached to the port the previous value will
   * be saved here.
   *
   * Automation in AutomationTrack's will overwrite
   * this value.
   */
  float               base_value;

  /**
   * When a modulator is attached to a control port
   * this will be set to 1, and set back to 0 when
   * all modulators are disconnected.
   */
  //int                 has_modulators;

  /**
   * Capture latency.
   *
   * See page 116 of "The Ardour DAW - Latency
   * Compensation and Anywhere-to-Anywhere Signal
   * Routing Systems".
   */
  long                capture_latency;

  /**
   * Playback latency.
   *
   * See page 116 of "The Ardour DAW - Latency
   * Compensation and Anywhere-to-Anywhere Signal
   * Routing Systems".
   */
  long                playback_latency;

  /** Port undergoing deletion. */
  int                 deleting;

  /**
   * Flag to indicate if the ring buffers below
   * should be filled or not.
   *
   * If a UI element that needs them becomes
   * mapped (visible), this should be set to
   * 1, and when unmapped (invisible) it should
   * be set to 0.
   */
  bool                write_ring_buffers;

  /** Whether the port has midi events not yet
   * processed by the UI. */
  volatile int        has_midi_events;

  /** Used by the UI to detect when unprocessed
   * MIDI events exist. */
  gint64              last_midi_event_time;

  /**
   * Ring buffer for saving the contents of the
   * audio buffer to be used in the UI instead of
   * directly accessing the buffer.
   *
   * This should contain blocks of block_length
   * samples and should maintain at least 10
   * cycles' worth of buffers.
   *
   * This is also used for CV.
   */
  ZixRing *           audio_ring;

  /**
   * Ring buffer for saving MIDI events to be
   * used in the UI instead of directly accessing
   * the events.
   *
   * This should keep pushing MidiEvent's whenever
   * they occur and the reader should empty it
   * after cheking if there are any events.
   *
   * Currently there is only 1 reader for each port
   * so this wont be a problem for now, but we
   * should have one ring for each reader.
   */
  ZixRing *           midi_ring;

  /** Max amplitude during processing, if audio
   * (fabsf). */
  float               peak;

  /** Last time \ref Port.max_amp was set. */
  gint64              peak_timestamp;

  /**
   * Last known MIDI status byte received.
   *
   * Used for running status (see
   * http://midi.teragonaudio.com/tech/midispec/run.htm).
   *
   * Not needed for JACK.
   */
  midi_byte_t         last_midi_status;

  /* --- ported from Lv2Port --- */

  /** LV2 port. */
  const LilvPort* lilv_port;

  /** Float for LV2 control ports, declared type for
   * LV2 parameters. */
  LV2_URID        value_type;

  /** For MIDI ports, otherwise NULL. */
  LV2_Evbuf *     evbuf;

  /**
   * Control widget, if applicable.
   *
   * Only used for generic UIs.
   */
  PluginGtkController * widget;

  /**
   * Minimum buffer size that the port wants, or
   * 0.
   *
   * Used by LV2 plugin ports.
   */
  size_t          min_buf_size;

  /** Port index in the lilv plugin, if
   * non-parameter. */
  int             lilv_port_index;

  /**
   * Whether the port received a UI event from
   * the plugin UI in this cycle.
   *
   * This is used to avoid re-sending that event
   * to the plugin.
   *
   * @note for control ports only.
   */
  bool            received_ui_event;

  /**
   * The last known control value sent to the UI
   * (if control).
   *
   * Also used by track processor for MIDI
   * controls.
   *
   * @seealso lv2_ui_send_control_val_event_from_plugin_to_ui().
   */
  float           last_sent_control;

  /** Whether this value was set via automation. */
  bool            automating;

  /** True for event, false for atom. */
  bool            old_api;

  /* --- end Lv2Port --- */

  /**
   * Automation track this port is attached to.
   *
   * To be set at runtime only (not serialized).
   */
  AutomationTrack *   at;

  /** Pointer to ExtPort, if hw, for quick
   * access (cache). */
  ExtPort *           ext_port;

  /** Whether this is a project port. */
  bool                is_project;

  /** Magic number to identify that this is a
   * Port. */
  int                 magic;
} Port;

static const cyaml_strval_t
port_internal_type_strings[] =
{
  { "LV2 port",       INTERNAL_LV2_PORT    },
  { "JACK port",      INTERNAL_JACK_PORT   },
};

static const cyaml_schema_field_t
port_fields_schema[] =
{
  YAML_FIELD_INT (
    Port, schema_version),
  YAML_FIELD_MAPPING_EMBEDDED (
    Port, id,
    port_identifier_fields_schema),
  YAML_FIELD_INT (
    Port, exposed_to_backend),
  CYAML_FIELD_SEQUENCE_COUNT (
    "dest_ids",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Port, dest_ids, num_dests,
    &port_identifier_schema_default,
    0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "src_ids",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Port, src_ids, num_srcs,
    &port_identifier_schema_default,
    0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "multipliers",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Port, multipliers, num_dests,
    &float_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "src_multipliers",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Port, src_multipliers, num_srcs,
    &float_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "dest_locked",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Port, dest_locked, num_dests,
    &int_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "src_locked",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Port, src_locked, num_srcs,
    &int_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "dest_enabled",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Port, dest_enabled, num_dests,
    &int_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "src_enabled",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Port, src_enabled, num_srcs,
    &int_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_ENUM (
    "internal_type", CYAML_FLAG_DEFAULT,
    Port, internal_type, port_internal_type_strings,
    CYAML_ARRAY_LEN (port_internal_type_strings)),
  YAML_FIELD_FLOAT (
    Port, control),
  YAML_FIELD_FLOAT (
    Port, minf),
  YAML_FIELD_FLOAT (
    Port, maxf),
  YAML_FIELD_FLOAT (
    Port, zerof),
  YAML_FIELD_FLOAT (
    Port, deff),
  YAML_FIELD_INT (
    Port, vst_param_id),
  YAML_FIELD_INT (
    Port, carla_param_id),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
port_schema =
{
  YAML_VALUE_PTR_NULLABLE (
    Port, port_fields_schema),
};

/**
 * L & R port, for convenience.
 *
 * Must ONLY be created via stereo_ports_new()
 */
typedef struct StereoPorts
{
  int          schema_version;
  Port       * l;
  Port       * r;
} StereoPorts;

static const cyaml_schema_field_t
  stereo_ports_fields_schema[] =
{
  YAML_FIELD_INT (
    StereoPorts, schema_version),
  YAML_FIELD_MAPPING_PTR (
    StereoPorts, l,
    port_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    StereoPorts, r,
    port_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  stereo_ports_schema = {
  YAML_VALUE_PTR (
    StereoPorts, stereo_ports_fields_schema),
};

/**
 * This function finds the Ports corresponding to
 * the PortIdentifiers for srcs and dests.
 *
 * Should be called after the ports are loaded from
 * yml.
 */
NONNULL
void
port_init_loaded (
  Port * self,
  bool   is_project);

NONNULL
Port *
port_find_from_identifier (
  PortIdentifier * id);

NONNULL
void
stereo_ports_init_loaded (
  StereoPorts * sp,
  bool          is_project);

/**
 * Creates port.
 */
NONNULL
Port *
port_new_with_type (
  PortType     type,
  PortFlow     flow,
  const char * label);

#if 0
/**
 * Creates port and adds given data to it.
 *
 * @param internal_type The internal data format.
 * @param data The data.
 */
Port *
port_new_with_data (
  PortInternalType internal_type,
  PortType     type,
  PortFlow     flow,
  const char * label,
  void *       data);
#endif

/**
 * Creates blank stereo ports.
 */
NONNULL
StereoPorts *
stereo_ports_new_from_existing (Port * l, Port * r);

/**
 * Creates stereo ports for generic use.
 *
 * @param in 1 for in, 0 for out.
 * @param owner Pointer to the owner. The type is
 *   determined by owner_type.
 */
StereoPorts *
stereo_ports_new_generic (
  int           in,
  const char *  name,
  PortOwnerType owner_type,
  void *        owner);

/**
 * Connects the internal ports using
 * port_connect().
 *
 * @param locked Lock the connection.
 *
 * @return Non-zero if error.
 */
NONNULL
void
stereo_ports_connect (
  StereoPorts * src,
  StereoPorts * dest,
  int           locked);

NONNULL
void
stereo_ports_disconnect (
  StereoPorts * self);

NONNULL
void
stereo_ports_fill_from_clip (
  StereoPorts * self,
  AudioClip *   clip,
  long          g_start_frames,
  nframes_t     start_frame,
  nframes_t     nframes);

NONNULL
void
stereo_ports_free (
  StereoPorts * self);

/**
 * Function to get a port's value from its string
 * symbol.
 *
 * Used when saving the LV2 state.
 * This function MUST set size and type
 * appropriately.
 */
const void *
port_get_value_from_symbol (
  const char * port_sym,
  void       * user_data,
  uint32_t   * size,
  uint32_t   * type);

#ifdef HAVE_JACK
/**
 * Receives MIDI data from the port's exposed
 * JACK port (if any) into the port.
 */
NONNULL
void
port_receive_midi_events_from_jack (
  Port *          self,
  const nframes_t start_frames,
  const nframes_t nframes);

/**
 * Receives audio data from the port's exposed
 * JACK port (if any) into the port.
 */
NONNULL
void
port_receive_audio_data_from_jack (
  Port *          self,
  const nframes_t start_frames,
  const nframes_t nframes);
#endif

/**
 * Copies a full designation of \p self in the
 * format "Track/Port" or "Track/Plugin/Port" in
 * \p buf.
 */
NONNULL
void
port_get_full_designation (
  Port * self,
  char * buf);

/**
 * Gathers all ports in the project and puts them
 * in the given array and size.
 *
 * @param size Current array count.
 * @param is_dynamic Whether the array can be
 *   dynamically resized.
 * @param max_size Current array size, if dynamic.
 */
void
port_get_all (
  Port *** ports,
  size_t * max_size,
  bool     is_dynamic,
  int *    size);

NONNULL
Track *
port_get_track (
  const Port * self,
  int          warn_if_fail);

NONNULL
Plugin *
port_get_plugin (
  Port * self,
  bool   warn_if_fail);

/**
 * To be called when the port's identifier changes
 * to update corresponding identifiers.
 *
 * @param track The track that owns this port.
 * @param update_automation_track Whether to update
 *   the identifier in the corresponding automation
 *   track as well. This should be false when
 *   moving a plugin.
 */
void
port_update_identifier (
  Port *  self,
  Track * track,
  bool    update_automation_track);

/**
 * Returns the index of the destination in the dest
 * array.
 */
NONNULL
static inline int
port_get_dest_index (
  Port * self,
  Port * dest)
{
  g_return_val_if_fail (
    IS_PORT (self) && IS_PORT (dest) &&
    self->is_project && dest->is_project, -1);

  for (int i = 0; i < self->num_dests; i++)
    {
      if (self->dests[i] == dest)
        return i;
    }
  g_return_val_if_reached (-1);
}

/**
 * Returns the index of the source in the source
 * array.
 */
NONNULL
static inline int
port_get_src_index (
  Port * self,
  Port * src)
{
  g_return_val_if_fail (
    IS_PORT (self) && IS_PORT (src) &&
    self->is_project && src->is_project, -1);

  for (int i = 0; i < self->num_srcs; i++)
    {
      if (self->srcs[i] == src)
        return i;
    }
  g_return_val_if_reached (-1);
}

/**
 * Set the multiplier for a destination by its
 * index in the dest array.
 */
NONNULL
static inline void
port_set_multiplier_by_index (
  Port * port,
  int    idx,
  float  val)
{
  port->multipliers[idx] = val;
}

/**
 * Set the multiplier for a destination by its
 * index in the dest array.
 */
NONNULL
static inline void
port_set_src_multiplier_by_index (
  Port * port,
  int    idx,
  float  val)
{
  port->src_multipliers[idx] = val;
}

/**
 * Get the multiplier for a destination by its
 * index in the dest array.
 */
NONNULL
static inline float
port_get_multiplier_by_index (
  Port * port,
  int    idx)
{
  return port->multipliers[idx];
}

NONNULL
void
port_set_multiplier (
  Port * src,
  Port * dest,
  float  val);

NONNULL
float
port_get_multiplier (
  Port * src,
  Port * dest);

NONNULL
void
port_set_enabled (
  Port * src,
  Port * dest,
  bool   enabled);

NONNULL
bool
port_get_enabled (
  Port * src,
  Port * dest);

#ifdef HAVE_RTMIDI
/**
 * Dequeue the midi events from the ring
 * buffers into \ref RtMidiDevice.events.
 */
NONNULL
void
port_prepare_rtmidi_events (
  Port * self);

/**
 * Sums the inputs coming in from RtMidi
 * before the port is processed.
 */
NONNULL
void
port_sum_data_from_rtmidi (
  Port * self,
  const nframes_t start_frame,
  const nframes_t nframes);
#endif

#ifdef HAVE_RTAUDIO
/**
 * Dequeue the audio data from the ring
 * buffers into \ref RtAudioDevice.buf.
 */
NONNULL
void
port_prepare_rtaudio_data (
  Port * self);

/**
 * Sums the inputs coming in from RtAudio
 * before the port is processed.
 */
NONNULL
void
port_sum_data_from_rtaudio (
  Port * self,
  const nframes_t start_frame,
  const nframes_t nframes);
#endif

/**
 * Disconnects all hardware inputs from the port.
 */
NONNULL
void
port_disconnect_hw_inputs (
  Port * self);

/**
 * Deletes port, doing required cleanup and updating counters.
 */
NONNULL
void
port_free (Port * port);

/**
 * Sets whether to expose the port to the backend
 * and exposes it or removes it.
 *
 * It checks what the backend is using the engine's
 * audio backend or midi backend settings.
 */
NONNULL
void
port_set_expose_to_backend (
  Port * self,
  int    expose);

/**
 * Returns if the port is exposed to the backend.
 */
NONNULL
int
port_is_exposed_to_backend (
  const Port * self);

/**
 * Renames the port on the backend side.
 */
NONNULL
void
port_rename_backend (
  Port * self);

#ifdef HAVE_ALSA

/**
 * Returns the Port matching the given ALSA
 * sequencer port ID.
 */
Port *
port_find_by_alsa_seq_id (
  const int id);
#endif

/**
 * Sets the given control value to the
 * corresponding underlying structure in the Port.
 *
 * Note: this is only for setting the base values
 * (eg when automating via an automation lane). For
 * CV automations this should not be used.
 *
 * @param is_normalized Whether the given value is
 *   normalized between 0 and 1.
 * @param forward_event Whether to forward a port
 *   control change event to the plugin UI. Only
 *   applicable for plugin control ports.
 *   If the control is being changed manually or
 *   from within Zrythm, this should be true to
 *   notify the plugin of the change.
 */
NONNULL
void
port_set_control_value (
  Port *      self,
  const float val,
  const bool  is_normalized,
  const bool  forward_event);

/**
 * Gets the given control value from the
 * corresponding underlying structure in the Port.
 *
 * @param normalize Whether to get the value
 *   normalized or not.
 */
NONNULL
float
port_get_control_value (
  Port *      self,
  const bool  normalize);

NONNULL
void
port_set_is_project (
  Port * self,
  bool   is_project);

/**
 * Connets src to dest.
 *
 * @param locked Lock the connection or not.
 */
NONNULL
int
port_connect (
  Port * src,
  Port * dest,
  const int locked);

/**
 * Disconnects src from dest.
 */
NONNULL
int
port_disconnect (Port * src, Port * dest);

/**
 * Disconnects dests of port.
 */
NONNULL
static inline int
port_disconnect_dests (Port * src)
{
  g_warn_if_fail (src);

  for (int i = 0; i < src->num_dests; i++)
    {
      port_disconnect (src, src->dests[i]);
    }
  return 0;
}

/**
 * Returns the number of unlocked (user-editable)
 * sources.
 */
NONNULL
int
port_get_num_unlocked_srcs (Port * port);

/**
 * Returns the number of unlocked (user-editable)
 * destinations.
 */
NONNULL
int
port_get_num_unlocked_dests (Port * port);

/**
 * Updates the track pos on a track port and
 * all its source/destination identifiers.
 *
 * @param track The track that owns this port.
 */
void
port_update_track_pos (
  Port *  port,
  Track * track,
  int     pos);

/**
 * Apply given fader value to port.
 *
 * @param start_frame The start frame offset from
 *   0 in this cycle.
 * @param nframes The number of frames to process.
 */
NONNULL
static inline void
port_apply_fader (
  Port *    port,
  float     amp,
  nframes_t start_frame,
  const nframes_t nframes)
{
  nframes_t end = start_frame + nframes;
  while (start_frame < end)
    {
      port->buf[start_frame++] *= amp;
    }
}

/**
 * First sets port buf to 0, then sums the given
 * port signal from its inputs.
 *
 * @param start_frame The start frame offset from
 *   0 in this cycle.
 * @param nframes The number of frames to process.
 * @param noroll Clear the port buffer in this
 *   range.
 */
HOT
NONNULL
void
port_process (
  Port *          port,
  const long      g_start_frames,
  const nframes_t start_frame,
  const nframes_t nframes,
  const bool      noroll);

/**
 * Sets the owner track & its ID.
 */
NONNULL
void
port_set_owner_track (
  Port *    port,
  Track *   track);

/**
 * Sets the owner track & its ID.
 */
NONNULL
void
port_set_owner_track_from_channel (
  Port *    port,
  Channel * ch);

/**
 * Sets the owner track & its ID.
 */
NONNULL
void
port_set_owner_track_processor (
  Port *           port,
  TrackProcessor * track_processor);

/**
 * Sets the owner sample processor.
 */
NONNULL
void
port_set_owner_sample_processor (
  Port *   port,
  SampleProcessor * sample_processor);

/**
 * Sets the owner fader & its ID.
 */
NONNULL
void
port_set_owner_fader (
  Port *    port,
  Fader *   fader);

#if 0
/**
 * Sets the owner fader & its ID.
 */
void
port_set_owner_prefader (
  Port *                 port,
  PassthroughProcessor * fader);
#endif

/**
 * Sets the owner plugin & its ID.
 */
NONNULL
void
port_set_owner_plugin (
  Port *   port,
  Plugin * pl);

/**
 * Returns if the connection from \p src to \p
 * dest is locked or not.
 */
NONNULL
int
port_is_connection_locked (
  Port * src,
  Port * dest);

/**
 * Returns if the two ports are connected or not.
 */
NONNULL
bool
ports_connected (
  Port * src, Port * dest);

/**
 * Returns whether the Port's can be connected (if
 * the connection will be valid and won't break the
 * acyclicity of the graph).
 */
NONNULL
bool
ports_can_be_connected (
  const Port * src,
  const Port *dest);

/**
 * Disconnects all the given ports.
 */
NONNULL
void
ports_disconnect (
  Port ** ports,
  int     num_ports,
  int     deleting);

/**
 * Copies the metadata from a project port to
 * the given port.
 *
 * Used when doing delete actions so that ports
 * can be restored on undo.
 */
NONNULL
void
port_copy_metadata_from_project (
  Port * self,
  Port * project_port);

/**
 * Reverts the data on the corresponding project
 * port for the given non-project port.
 *
 * This restores src/dest connections and the
 * control value.
 *
 * @param self Project port.
 * @param non_project Non-project port.
 */
NONNULL
void
port_restore_from_non_project (
  Port * self,
  Port * non_project);

/**
 * Prints all connections.
 */
void
port_print_connections_all (void);

/**
 * Clears the port buffer.
 */
HOT
NONNULL
void
port_clear_buffer (Port * port);

/**
 * Disconnects all srcs and dests from port.
 */
NONNULL
int
port_disconnect_all (Port * port);

/**
 * Verifies that the srcs and dests are correct
 * for project ports.
 */
NONNULL
void
port_verify_src_and_dests (
  Port * self);

/**
 * Applies the pan to the given L/R ports.
 */
NONNULL
void
port_apply_pan_stereo (Port *       l,
                       Port *       r,
                       float        pan,
                       PanLaw       pan_law,
                       PanAlgorithm pan_algo);

/**
 * Applies the pan to the given port.
 *
 * @param start_frame The start frame offset from
 *   0 in this cycle.
 * @param nframes The number of frames to process.
 */
NONNULL
void
port_apply_pan (
  Port *       port,
  float        pan,
  PanLaw       pan_law,
  PanAlgorithm pan_algo,
  nframes_t start_frame,
  const nframes_t nframes);

/**
 * @}
 */

#endif
