/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "utils/types.h"
#include "utils/yaml.h"
#include "zix/sem.h"

#ifdef HAVE_JACK
#include <jack/jack.h>
#endif

typedef struct Plugin Plugin;
typedef struct MidiEvents MidiEvents;
typedef struct Fader Fader;
typedef struct SampleProcessor SampleProcessor;
typedef struct PassthroughProcessor
  PassthroughProcessor;
typedef struct ZixRingImpl ZixRing;
typedef struct WindowsMmeDevice WindowsMmeDevice;

/**
 * @addtogroup audio
 *
 * @{
 */

#define MAX_DESTINATIONS 600
#define FOREACH_SRCS(port) \
  for (int i = 0; i < port->num_srcs; i++)
#define FOREACH_DESTS(port) \
  for (int i = 0; i < port->num_dests; i++)

/**
 * Special ID for owner_pl, owner_ch, etc. to indicate that
 * the port is not owned.
 */
#define PORT_NOT_OWNED -1

/**
 * Direction of the signal.
 */
typedef enum PortFlow {
  FLOW_UNKNOWN,
  FLOW_INPUT,
  FLOW_OUTPUT
} PortFlow;

/**
 * Type of signals the Port handles.
 */
typedef enum PortType {
  TYPE_UNKNOWN,
  TYPE_CONTROL,
  TYPE_AUDIO,
  TYPE_EVENT,
  TYPE_CV
} PortType;

/**
 * Type of owner.
 */
typedef enum PortOwnerType
{
  //PORT_OWNER_TYPE_NONE,
  PORT_OWNER_TYPE_BACKEND,
  PORT_OWNER_TYPE_PLUGIN,
  PORT_OWNER_TYPE_TRACK,
  PORT_OWNER_TYPE_PREFADER,

  /* track fader */
  PORT_OWNER_TYPE_FADER,

  /* monitor fader */
  PORT_OWNER_TYPE_MONITOR_FADER,

  /* TrackProcessor. */
  PORT_OWNER_TYPE_TRACK_PROCESSOR,

  PORT_OWNER_TYPE_SAMPLE_PROCESSOR,
} PortOwnerType;

/**
 * Port flags.
 */
typedef enum PortFlags
{
  PORT_FLAG_STEREO_L = 0x01,
  PORT_FLAG_STEREO_R = 0x02,
  PORT_FLAG_PIANO_ROLL = 0x04,
  /** See http://lv2plug.in/ns/ext/port-groups/port-groups.html#sideChainOf. */
  PORT_FLAG_SIDECHAIN = 0x08,
  /** See http://lv2plug.in/ns/ext/port-groups/port-groups.html#mainInput
   * and http://lv2plug.in/ns/ext/port-groups/port-groups.html#mainOutput. */
  PORT_FLAG_MAIN_PORT = 0x10,
  PORT_FLAG_MANUAL_PRESS = 0x20,
  PORT_FLAG_AMPLITUDE = 0x40,
  PORT_FLAG_PAN = 0x80,
} PortFlags;

static const cyaml_bitdef_t
flags_bitvals[] =
{
  { .name = "stereo_l", .offset =  0, .bits =  1 },
  { .name = "stereo_r", .offset =  1, .bits =  1 },
  { .name = "piano_roll", .offset = 2, .bits = 1 },
  { .name = "sidechain", .offset = 3, .bits =  1 },
  { .name = "main_port", .offset = 4, .bits = 1 },
  { .name = "manual_press", .offset = 5, .bits = 1 },
  { .name = "amplitude", .offset = 6, .bits = 1 },
  { .name = "pan", .offset = 7, .bits = 1 },
};

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

typedef struct Lv2Port Lv2Port;
typedef struct Channel Channel;
typedef struct Track Track;
typedef struct SampleProcessor SampleProcessor;
typedef enum PanAlgorithm PanAlgorithm;
typedef enum PanLaw PanLaw;

/**
 * Struct used to identify Ports in the project.
 *
 * This should include some members of the original
 * struct enough to identify the port. To be used
 * for sources and dests.
 *
 * This must be filled in before saving and read from
 * while loading to fill in the srcs/dests.
 */
typedef struct PortIdentifier
{
  /** Human readable label. */
  char *              label;
  /** Owner type. */
  PortOwnerType       owner_type;
  /** Data type (e.g. AUDIO). */
  PortType            type;
  /** Flow (IN/OUT). */
  PortFlow            flow;
  /** Flags (e.g. is side chain). */
  PortFlags           flags;

  /** Index of Plugin in the Channel. */
  int                 plugin_slot;

  /** Index of Track in the Tracklist. */
  int                 track_pos;
  /** Index (e.g. in plugin's output ports). */
  int                 port_index;
} PortIdentifier;

/**
 * Must ONLY be created via port_new()
 */
typedef struct Port
{
  PortIdentifier      identifier;

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
  struct Port *       srcs[MAX_DESTINATIONS];
  struct Port *       dests[MAX_DESTINATIONS];
  PortIdentifier      src_ids[MAX_DESTINATIONS];
  PortIdentifier      dest_ids[MAX_DESTINATIONS];

  /** These are the multipliers for port connections.
   *
   * They range from 0.f to 1.f and the default is
   * 1.f. They correspond to each destination.
   */
  float               multipliers[MAX_DESTINATIONS];

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
  int                 dest_locked[MAX_DESTINATIONS];

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
  int                 dest_enabled[MAX_DESTINATIONS];

  /** Counters. */
  int                 num_srcs;
  int                 num_dests;

  /**
   * Indicates whether data or lv2_port should be
   * used.
   */
  PortInternalType    internal_type;

  /** Used for LV2. */
  Lv2Port *          lv2_port;

  /** VST parameter index, if VST control port. */
  int                vst_param_id;

  /**
   * Pointer to arbitrary data.
   *
   * Use internal_type to check what data it is.
   */
  void *              data;

#ifdef _WIN32
  /** Connections to WindowsMmeDevices.
   *
   * These must be pointers to \ref
   * AudioEngine.mme_in_devs or \ref
   * AudioEngine.mme_out_devs and must not be
   * allocated or free'd.
   */
  WindowsMmeDevice *  mme_connections[40];
  int                 num_mme_connections;

  /** Semaphore for changing the connections
   * atomically. */
  ZixSem              mme_connections_sem;

  /** Last time the port finished dequeueing
   * MIDI events. */
  gint64              last_midi_dequeue;
#endif

  /* TODO move these from lv2_control */
  /** Minimum value. */
  //float               minf;

  /** Maximum value. */
  //float               maxf;

  /** Default value, if applicable. */
  //float               maxf;

  /**
   * The control value if control port, otherwise
   * 0.0f.
   *
   * FIXME for fader, this should be the
   * fader_val (0.0 to 1.0) and not the
   * amplitude.
   */
  float               control;

  /* ====== flags to indicate port owner ====== */

  Plugin *            plugin;
  Track *             track;
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
  int                 write_ring_buffers;

  /** Whether the port has midi events not yet
   * processed by the UI. */
  volatile int        has_midi_events;

  /**
   * Ring buffer for saving the contents of the
   * audio buffer to be used in the UI instead of
   * directly accessing the buffer.
   *
   * This should contain blocks of block_length
   * samples and should maintain at least 10
   * cycles' worth of buffers.
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
} Port;

static const cyaml_strval_t
port_flow_strings[] =
{
  { "unknown",       FLOW_UNKNOWN    },
  { "input",         FLOW_INPUT   },
  { "output",        FLOW_OUTPUT   },
};

static const cyaml_strval_t
port_owner_type_strings[] =
{
  { "backend",   PORT_OWNER_TYPE_BACKEND    },
  { "plugin",    PORT_OWNER_TYPE_PLUGIN   },
  { "track",     PORT_OWNER_TYPE_TRACK   },
  { "pre-fader", PORT_OWNER_TYPE_PREFADER   },
  { "fader",     PORT_OWNER_TYPE_FADER   },
  { "track processor",
    PORT_OWNER_TYPE_TRACK_PROCESSOR   },
  { "monitor fader",
    PORT_OWNER_TYPE_MONITOR_FADER },
  { "sample processor",
    PORT_OWNER_TYPE_SAMPLE_PROCESSOR },
};

static const cyaml_strval_t
port_type_strings[] =
{
  { "unknown",       TYPE_UNKNOWN    },
  { "control",       TYPE_CONTROL   },
  { "audio",         TYPE_AUDIO   },
  { "event",         TYPE_EVENT   },
  { "cv",            TYPE_CV   },
};

static const cyaml_strval_t
port_internal_type_strings[] =
{
  { "LV2 port",       INTERNAL_LV2_PORT    },
  { "JACK port",      INTERNAL_JACK_PORT   },
};

static const cyaml_schema_field_t
port_identifier_fields_schema[] =
{
  CYAML_FIELD_STRING_PTR (
    "label", CYAML_FLAG_POINTER,
    PortIdentifier, label,
     0, CYAML_UNLIMITED),
  CYAML_FIELD_ENUM (
    "owner_type", CYAML_FLAG_DEFAULT,
    PortIdentifier, owner_type,
    port_owner_type_strings,
    CYAML_ARRAY_LEN (port_owner_type_strings)),
  CYAML_FIELD_ENUM (
    "type", CYAML_FLAG_DEFAULT,
    PortIdentifier, type, port_type_strings,
    CYAML_ARRAY_LEN (port_type_strings)),
  CYAML_FIELD_ENUM (
    "flow", CYAML_FLAG_DEFAULT,
    PortIdentifier, flow, port_flow_strings,
    CYAML_ARRAY_LEN (port_flow_strings)),
  CYAML_FIELD_BITFIELD (
    "flags", CYAML_FLAG_DEFAULT,
    PortIdentifier, flags, flags_bitvals,
    CYAML_ARRAY_LEN (flags_bitvals)),
  CYAML_FIELD_INT (
    "track_pos", CYAML_FLAG_DEFAULT,
    PortIdentifier, track_pos),
  CYAML_FIELD_INT (
    "plugin_slot", CYAML_FLAG_DEFAULT,
    PortIdentifier, plugin_slot),
  CYAML_FIELD_INT (
    "port_index", CYAML_FLAG_DEFAULT,
    PortIdentifier, port_index),

  CYAML_FIELD_END,
};

static const cyaml_schema_value_t
port_identifier_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    PortIdentifier, port_identifier_fields_schema),
};

static const cyaml_schema_value_t
port_identifier_schema_default = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_DEFAULT,
    PortIdentifier, port_identifier_fields_schema),
};

static const cyaml_schema_field_t
port_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "identifier", CYAML_FLAG_DEFAULT,
    Port, identifier,
    port_identifier_fields_schema),
  CYAML_FIELD_INT (
    "exposed_to_backend", CYAML_FLAG_DEFAULT,
    Port, exposed_to_backend),
  CYAML_FIELD_SEQUENCE_COUNT (
    "src_ids", CYAML_FLAG_DEFAULT,
    Port, src_ids, num_srcs,
    &port_identifier_schema_default,
    0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "dest_ids", CYAML_FLAG_DEFAULT,
    Port, dest_ids, num_dests,
    &port_identifier_schema_default,
    0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "multipliers", CYAML_FLAG_DEFAULT,
    Port, multipliers, num_dests,
    &float_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "dest_locked", CYAML_FLAG_DEFAULT,
    Port, dest_locked, num_dests,
    &int_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "dest_enabled", CYAML_FLAG_DEFAULT,
    Port, dest_enabled, num_dests,
    &int_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_ENUM (
    "internal_type", CYAML_FLAG_DEFAULT,
    Port, internal_type, port_internal_type_strings,
    CYAML_ARRAY_LEN (port_internal_type_strings)),
  CYAML_FIELD_FLOAT (
    "control", CYAML_FLAG_DEFAULT,
    Port, control),
  CYAML_FIELD_INT (
    "vst_param_id", CYAML_FLAG_DEFAULT,
    Port, vst_param_id),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
port_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    Port, port_fields_schema),
};

/**
 * L & R port, for convenience.
 *
 * Must ONLY be created via stereo_ports_new()
 */
typedef struct StereoPorts
{
  Port       * l;
  Port       * r;
} StereoPorts;

static const cyaml_schema_field_t
  stereo_ports_fields_schema[] =
{
  CYAML_FIELD_MAPPING_PTR (
    "l", CYAML_FLAG_POINTER,
    StereoPorts, l,
    port_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "r", CYAML_FLAG_POINTER,
    StereoPorts, r,
    port_fields_schema),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
  stereo_ports_schema = {
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    StereoPorts, stereo_ports_fields_schema),
};

/**
 * This function finds the Ports corresponding to
 * the PortIdentifiers for srcs and dests.
 *
 * Should be called after the ports are loaded from
 * yml.
 */
void
port_init_loaded (Port * port);

Port *
port_find_from_identifier (
  PortIdentifier * id);

static inline void
port_identifier_copy (
  PortIdentifier * src,
  PortIdentifier * dest)
{
  if (dest->label)
    g_free (dest->label);
  dest->label = g_strdup (src->label);
  dest->owner_type = src->owner_type;
  dest->type = src->type;
  dest->flow = src->flow;
  dest->flags = src->flags;
  dest->plugin_slot = src->plugin_slot;
  dest->track_pos = src->track_pos;
  dest->port_index = src->port_index;
}

/**
 * Returns if the 2 PortIdentifier's are equal.
 */
int
port_identifier_is_equal (
  PortIdentifier * src,
  PortIdentifier * dest);

void
stereo_ports_init_loaded (StereoPorts * sp);

/**
 * Creates port.
 */
Port *
port_new_with_type (
  PortType     type,
  PortFlow     flow,
  const char * label);

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

/**
 * Creates blank stereo ports.
 */
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
 */
void
stereo_ports_connect (
  StereoPorts * src,
  StereoPorts * dest,
  int           locked);

#ifdef HAVE_JACK
/**
 * Receives MIDI data from the port's exposed
 * JACK port (if any) into the port.
 */
void
port_receive_midi_events_from_jack (
  Port *          port,
  const nframes_t start_frames,
  const nframes_t nframes);

/**
 * Receives audio data from the port's exposed
 * JACK port (if any) into the port.
 */
void
port_receive_audio_data_from_jack (
  Port *          port,
  const nframes_t start_frames,
  const nframes_t nframes);
#endif

/**
 * Copies a full designation of \p self in the
 * format "Track/Port" or "Track/Plugin/Port" in
 * \p buf.
 */
void
port_get_full_designation (
  const Port * self,
  char *       buf);

/**
 * Gathers all ports in the project and puts them
 * in the given array and size.
 */
void
port_get_all (
  Port ** ports,
  int *   size);

/**
 * Returns the index of the destination in the dest
 * array.
 */
static inline int
port_get_dest_index (
  Port * port,
  Port * dest)
{
  for (int i = 0; i < port->num_dests; i++)
    {
      if (port->dests[i] == dest)
        return i;
    }
  g_return_val_if_reached (-1);
}

/**
 * Set the multiplier for a destination by its
 * index in the dest array.
 */
static inline void
port_set_multiplier_by_index (
  Port * port,
  int    idx,
  float  val)
{
  port->multipliers[idx] = val;
}

/**
 * Get the multiplier for a destination by its
 * index in the dest array.
 */
static inline float
port_get_multiplier_by_index (
  Port * port,
  int    idx)
{
  return port->multipliers[idx];
}

/**
 * Returns the minimum possible value for this
 * port.
 *
 * Note that for Audio we should consider the
 * amp (0.0 and 2.0).
 */
float
port_get_minf (
  Port * self);

/**
 * Returns the maximum possible value for this
 * port.
 *
 * Note that for Audio we should consider the
 * amp (0.0 and 2.0).
 */
float
port_get_maxf (
  Port * self);

/**
 * Returns the zero value for the given port.
 *
 * Note that for Audio we should consider the
 * amp (0.0 and 2.0).
 */
float
port_get_zerof (
  Port * self);

/**
 * Deletes port, doing required cleanup and updating counters.
 */
void
port_free (Port * port);

/**
 * Sets whether to expose the port to the backend
 * and exposes it or removes it.
 *
 * It checks what the backend is using the engine's
 * audio backend or midi backend settings.
 */
void
port_set_expose_to_backend (
  Port * self,
  int    expose);

/**
 * Returns if the port is exposed to the backend.
 */
int
port_is_exposed_to_backend (
  const Port * self);

/**
 * Renames the port on the backend side.
 */
void
port_rename_backend (
  const Port * self);

#ifdef HAVE_JACK
/**
 * Sets whether to expose the port to JACk and
 * exposes it or removes it from JACK.
 */
void
port_set_expose_to_jack (
  Port * self,
  int    expose);

/**
 * Returns the JACK port attached to this port,
 * if any.
 */
jack_port_t *
port_get_internal_jack_port (
  Port * port);

#endif

#ifdef HAVE_ALSA

/**
 * Returns the Port matching the given ALSA
 * sequencer port ID.
 */
Port *
port_find_by_alsa_seq_id (
  const int id);

void
port_set_expose_to_alsa (
  Port * self,
  int    expose);

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
 */
void
port_set_control_value (
  Port *      self,
  const float val,
  const int   is_normalized,
  const int   forward_event);

/**
 * Gets the given control value from the
 * corresponding underlying structure in the Port.
 *
 * @param normalize Whether to get the value
 *   normalized or not.
 */
float
port_get_control_value (
  Port *      self,
  const int   normalize);

/**
 * Returns the RMS of the last n cycles for
 * audio ports.
 *
 * @param num_cycles Number of cycles to take into
 *   account, normally 1. If this is more than 1,
 *   the minimum of available cycles or given
 *   cycles is chosen.
 */
float
port_get_rms_db (
  Port * port,
  int    num_cycles);

/**
 * Connets src to dest.
 *
 * @param locked Lock the connection or not.
 */
int
port_connect (
  Port * src,
  Port * dest,
  const int locked);

/**
 * Disconnects src from dest.
 */
int
port_disconnect (Port * src, Port * dest);

/**
 * Disconnects dests of port.
 */
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
int
port_get_num_unlocked_srcs (Port * port);

/**
 * Returns the number of unlocked (user-editable)
 * destinations.
 */
int
port_get_num_unlocked_dests (Port * port);

/**
 * Apply given fader value to port.
 *
 * @param start_frame The start frame offset from
 *   0 in this cycle.
 * @param nframes The number of frames to process.
 */
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
void
port_sum_signal_from_inputs (
  Port *    port,
  const nframes_t start_frame,
  const nframes_t nframes,
  const int noroll);

/**
 * Sets the owner track & its ID.
 */
void
port_set_owner_track (
  Port *    port,
  Track *   track);

/**
 * Sets the owner track & its ID.
 */
void
port_set_owner_track_processor (
  Port *    port,
  Track *   track);

/**
 * Sets the owner sample processor.
 */
void
port_set_owner_sample_processor (
  Port *   port,
  SampleProcessor * sample_processor);

/**
 * Sets the owner fader & its ID.
 */
void
port_set_owner_fader (
  Port *    port,
  Fader *   fader);

/**
 * Sets the owner fader & its ID.
 */
void
port_set_owner_prefader (
  Port *                 port,
  PassthroughProcessor * fader);

/**
 * Sets the owner plugin & its ID.
 */
void
port_set_owner_plugin (
  Port *   port,
  Plugin * pl);

/**
 * Returns if the connection from \p src to \p
 * dest is locked or not.
 */
int
port_is_connection_locked (
  Port * src,
  Port * dest);

/**
 * Returns if the two ports are connected or not.
 */
int
ports_connected (
  Port * src, Port * dest);

/**
 * Returns whether the Port's can be connected (if
 * the connection will be valid and won't break the
 * acyclicity of the graph).
 */
int
ports_can_be_connected (
  const Port * src,
  const Port *dest);

/**
 * Disconnects all the given ports.
 */
static inline void
ports_disconnect (
  Port ** ports,
  int     num_ports,
  int     deleting)
{
  int i, j;
  Port * port;

  /* go through each port */
  for (i = 0; i < num_ports; i++)
    {
      port = ports[i];
      g_message ("Attempting to disconnect %s ("
                 "current srcs %d)",
                 port->identifier.label,
                 port->num_srcs);
      port->deleting = deleting;

      /* disconnect incoming ports */
      for (j = port->num_srcs - 1; j >= 0; j--)
        {
          port_disconnect (port->srcs[j], port);
        }
      /* disconnect outgoing ports */
      for (j = port->num_dests - 1; j >= 0; j--)
        {
          port_disconnect (
            port, port->dests[j]);
        }
      g_message ("%s num srcs %d dests %d",
                 port->identifier.label,
                 port->num_srcs,
                 port->num_dests);
    }
}

/**
 * Removes all the given ports from the project,
 * optionally freeing them.
 */
int
ports_remove (
  Port ** ports,
  int *   num_ports);

/**
 * Prints all connections.
 */
void
port_print_connections_all (void);

/**
 * Clears the port buffer.
 */
void
port_clear_buffer (Port * port);

/**
 * Disconnects all srcs and dests from port.
 */
int
port_disconnect_all (Port * port);

/**
 * Applies the pan to the given L/R ports.
 */
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
