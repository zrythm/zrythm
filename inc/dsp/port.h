// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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

#include "dsp/port_identifier.h"
#include "utils/types.h"

#include "zix/sem.h"

#ifdef HAVE_JACK
#  include "weak_libjack.h"
#endif

#ifdef HAVE_RTMIDI
#  include <rtmidi_c.h>
#endif

#ifdef HAVE_RTAUDIO
#  include <rtaudio_c.h>
#endif

typedef struct Plugin                  Plugin;
typedef struct MidiEvents              MidiEvents;
typedef struct Fader                   Fader;
typedef struct ZixRingImpl             ZixRing;
typedef struct WindowsMmeDevice        WindowsMmeDevice;
typedef struct Channel                 Channel;
typedef struct AudioEngine             AudioEngine;
typedef struct Track                   Track;
typedef struct PortConnection          PortConnection;
typedef struct TrackProcessor          TrackProcessor;
typedef struct ModulatorMacroProcessor ModulatorMacroProcessor;
typedef struct RtMidiDevice            RtMidiDevice;
typedef struct RtAudioDevice           RtAudioDevice;
typedef struct AutomationTrack         AutomationTrack;
typedef struct TruePeakDsp             TruePeakDsp;
typedef struct ExtPort                 ExtPort;
typedef struct AudioClip               AudioClip;
typedef struct ChannelSend             ChannelSend;
typedef struct Transport               Transport;
typedef struct PluginGtkController     PluginGtkController;
typedef struct EngineProcessTimeInfo   EngineProcessTimeInfo;
typedef enum PanAlgorithm              PanAlgorithm;
typedef enum PanLaw                    PanLaw;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define PORT_SCHEMA_VERSION 1
#define STEREO_PORTS_SCHEMA_VERSION 1

#define PORT_MAGIC 456861194
#define IS_PORT(_p) (((Port *) (_p))->magic == PORT_MAGIC)
#define IS_PORT_AND_NONNULL(x) ((x) && IS_PORT (x))

#define TIME_TO_RESET_PEAK 4800000

/**
 * Special ID for owner_pl, owner_ch, etc. to indicate that
 * the port is not owned.
 */
#define PORT_NOT_OWNED -1

#define port_is_owner_active(self, _owner_type, owner) \
  ((self->id.owner_type == _owner_type) && (self->owner != NULL) \
   && owner##_is_in_active_project (self->owner))

#define port_is_in_active_project(self) \
  (port_is_owner_active (self, PORT_OWNER_TYPE_AUDIO_ENGINE, engine) \
   || port_is_owner_active (self, PORT_OWNER_TYPE_PLUGIN, plugin) \
   || port_is_owner_active (self, PORT_OWNER_TYPE_TRACK, track) \
   || port_is_owner_active (self, PORT_OWNER_TYPE_CHANNEL, track) \
   || port_is_owner_active (self, PORT_OWNER_TYPE_FADER, fader) \
   || port_is_owner_active (self, PORT_OWNER_TYPE_CHANNEL_SEND, channel_send) \
   || port_is_owner_active (self, PORT_OWNER_TYPE_TRACK_PROCESSOR, track) \
   || port_is_owner_active ( \
     self, PORT_OWNER_TYPE_MODULATOR_MACRO_PROCESSOR, modulator_macro_processor) \
   || port_is_owner_active (self, PORT_OWNER_TYPE_HW, ext_port))

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

PURE int
port_scale_point_cmp (const void * _a, const void * _b);

NONNULL PortScalePoint *
port_scale_point_new (const float val, const char * label);

NONNULL void
port_scale_point_free (PortScalePoint * self);

/**
 * Must ONLY be created via port_new()
 */
typedef struct Port
{
  int schema_version;

  PortIdentifier id;

  /**
   * Flag to indicate that this port is exposed
   * to the backend.
   */
  int exposed_to_backend;

  /**
   * Buffer to be reallocated every time the buffer
   * size changes.
   *
   * The buffer size is AUDIO_ENGINE->block_length.
   */
  float * buf;

  /**
   * Contains raw MIDI data (MIDI ports only)
   */
  MidiEvents * midi_events;

  /** Caches filled when recalculating the
   * graph. */
  struct Port ** srcs;
  int            num_srcs;
  size_t         srcs_size;

  /** Caches filled when recalculating the
   * graph. */
  struct Port ** dests;
  int            num_dests;
  size_t         dests_size;

  /** Caches filled when recalculating the
   * graph. */
  const PortConnection ** src_connections;
  int                     num_src_connections;
  size_t                  src_connections_size;

  /** Caches filled when recalculating the
   * graph. */
  const PortConnection ** dest_connections;
  int                     num_dest_connections;
  size_t                  dest_connections_size;

  /**
   * Indicates whether data or lv2_port should be
   * used.
   */
  PortInternalType internal_type;

  /**
   * Minimum, maximum and zero values for this
   * port.
   *
   * Note that for audio, this is the amp (0 - 2)
   * and not the actual values.
   */
  float minf;
  float maxf;

  /**
   * The zero position of the port.
   *
   * For example, in balance controls, this will
   * be the middle. In audio ports, this will be
   * 0 amp (silence), etc.
   */
  float zerof;

  /** Default value, only used for controls. */
  float deff;

  /** Index of the control parameter (for Carla
   * plugin ports). */
  int carla_param_id;

  /**
   * Pointer to arbitrary data.
   *
   * Use internal_type to check what data it is.
   *
   * FIXME just add the various data structs here
   * and remove this ambiguity.
   */
  void * data;

#ifdef _WOE32
  /**
   * Connections to WindowsMmeDevices.
   *
   * These must be pointers to \ref
   * AudioEngine.mme_in_devs or \ref
   * AudioEngine.mme_out_devs and must not be
   * allocated or free'd.
   */
  WindowsMmeDevice * mme_connections[40];
  int                num_mme_connections;
#else
  void * mme_connections[40];
  int    num_mme_connections;
#endif

  /** Semaphore for changing the connections
   * atomically. */
  ZixSem mme_connections_sem;

  /**
   * Last time the port finished dequeueing
   * MIDI events.
   *
   * Used for some backends only.
   */
  gint64 last_midi_dequeue;

#ifdef HAVE_RTMIDI
  /**
   * RtMidi pointers for input ports.
   *
   * Each RtMidi port represents a device, and this
   * Port can be connected to multiple devices.
   */
  RtMidiDevice * rtmidi_ins[128];
  int            num_rtmidi_ins;

  /** RtMidi pointers for output ports. */
  RtMidiDevice * rtmidi_outs[128];
  int            num_rtmidi_outs;
#else
  void * rtmidi_ins[128];
  int    num_rtmidi_ins;
  void * rtmidi_outs[128];
  int    num_rtmidi_outs;
#endif

#ifdef HAVE_RTAUDIO
  /**
   * RtAudio pointers for input ports.
   *
   * Each port can have multiple RtAudio devices.
   */
  RtAudioDevice * rtaudio_ins[128];
  int             num_rtaudio_ins;
#else
  void * rtaudio_ins[128];
  int    num_rtaudio_ins;
#endif

  /** Scale points. */
  PortScalePoint ** scale_points;
  int               num_scale_points;

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
  float control;

  /** Unsnapped value, used by widgets. */
  float unsnapped_control;

  /** Flag that the value of the port changed from
   * reading automation. */
  bool value_changed_from_reading;

  /**
   * Last timestamp the control changed.
   *
   * This is used when recording automation in "touch" mode.
   */
  gint64 last_change;

  /** Pointer to owner plugin, if any. */
  Plugin * plugin;

  /** Pointer to owner transport, if any. */
  Transport * transport;

  /** Pointer to owner channel send, if any. */
  ChannelSend * channel_send;

  /** Pointer to owner engine, if any. */
  AudioEngine * engine;

  /** Pointer to owner fader, if any. */
  Fader * fader;

  /**
   * Pointer to owner track, if any.
   *
   * Also used for channel and track processor
   * ports.
   */
  Track * track;

  /** Pointer to owner modulator macro processor,
   * if any. */
  ModulatorMacroProcessor * modulator_macro_processor;

  /* ====== flags to indicate port owner ====== */

  /**
   * Temporary plugin pointer (used when the
   * plugin doesn't exist yet in its supposed slot).
   * FIXME delete
   */
  Plugin * tmp_plugin;

  /** used when loading projects FIXME needed? */
  int initialized;

  /**
   * For control ports, when a modulator is attached to the port the previous
   * value will be saved here.
   *
   * Automation in AutomationTrack's will overwrite this value.
   */
  float base_value;

  /**
   * Capture latency.
   *
   * See page 116 of "The Ardour DAW - Latency Compensation and
   * Anywhere-to-Anywhere Signal Routing Systems".
   */
  long capture_latency;

  /**
   * Playback latency.
   *
   * See page 116 of "The Ardour DAW - Latency Compensation and
   * Anywhere-to-Anywhere Signal Routing Systems".
   */
  long playback_latency;

  /** Port undergoing deletion. */
  int deleting;

  /**
   * Flag to indicate if the ring buffers below should be
   * filled or not.
   *
   * If a UI element that needs them becomes mapped
   * (visible), this should be set to true, and when unmapped
   * (invisible) it should be set to false.
   */
  bool write_ring_buffers;

  /** Whether the port has midi events not yet processed by the UI. */
  int has_midi_events;

  /** Used by the UI to detect when unprocessed MIDI events
   * exist. */
  gint64 last_midi_event_time;

  /**
   * Ring buffer for saving the contents of the audio buffer
   * to be used in the UI instead of directly accessing the
   * buffer.
   *
   * This should contain blocks of block_length samples and
   * should maintain at least 10 cycles' worth of buffers.
   *
   * This is also used for CV.
   */
  ZixRing * audio_ring;

  /**
   * Ring buffer for saving MIDI events to be
   * used in the UI instead of directly accessing
   * the events.
   *
   * This should keep pushing MidiEvent's whenever
   * they occur and the reader should empty it
   * after checking if there are any events.
   *
   * Currently there is only 1 reader for each port
   * so this wont be a problem for now, but we
   * should have one ring for each reader.
   */
  ZixRing * midi_ring;

  /** Max amplitude during processing, if audio
   * (fabsf). */
  float peak;

  /** Last time \ref Port.max_amp was set. */
  gint64 peak_timestamp;

  /**
   * Last known MIDI status byte received.
   *
   * Used for running status (see
   * http://midi.teragonaudio.com/tech/midispec/run.htm).
   *
   * Not needed for JACK.
   */
  midi_byte_t last_midi_status;

  /**
   * Control widget, if applicable.
   *
   * Only used for generic UIs.
   */
  PluginGtkController * widget;

  /**
   * Whether the port received a UI event from
   * the plugin UI in this cycle.
   *
   * This is used to avoid re-sending that event
   * to the plugin.
   *
   * @note for control ports only.
   */
  bool received_ui_event;

  /** Whether this value was set via automation. */
  bool automating;

  /**
   * Automation track this port is attached to.
   *
   * To be set at runtime only (not serialized).
   */
  AutomationTrack * at;

  /*
   * Next 2 objects are MIDI CC info, if MIDI CC in track processor.
   *
   * Used as a cache.
   */

  /** MIDI channel, starting from 1. */
  midi_byte_t midi_channel;

  /** MIDI CC number, if not pitchbend/poly key/channel pressure. */
  midi_byte_t midi_cc_no;

  /** Pointer to ExtPort, if hw. */
  ExtPort * ext_port;

  /** Magic number to identify that this is a Port. */
  int magic;

  /** Last allocated buffer size (used for audio
   * ports). */
  size_t last_buf_sz;
} Port;

/**
 * L & R port, for convenience.
 *
 * Must ONLY be created via stereo_ports_new()
 */
typedef struct StereoPorts
{
  int    schema_version;
  Port * l;
  Port * r;
} StereoPorts;

/**
 * This function finds the Ports corresponding to
 * the PortIdentifiers for srcs and dests.
 *
 * Should be called after the ports are loaded from
 * yml.
 */
NONNULL void
port_init_loaded (Port * self, void * owner);

void
port_set_owner (Port * self, PortOwnerType owner_type, void * owner);

NONNULL Port *
port_find_from_identifier (const PortIdentifier * const id);

NONNULL static inline void
stereo_ports_init_loaded (StereoPorts * sp, void * owner)
{
  port_init_loaded (sp->l, owner);
  port_init_loaded (sp->r, owner);
}

NONNULL_ARGS (1)
static inline void stereo_ports_set_owner (
  StereoPorts * sp,
  PortOwnerType owner_type,
  void *        owner)
{
  port_set_owner (sp->l, owner_type, owner);
  port_set_owner (sp->r, owner_type, owner);
}

/**
 * Creates port.
 */
WARN_UNUSED_RESULT NONNULL Port *
port_new_with_type (PortType type, PortFlow flow, const char * label);

WARN_UNUSED_RESULT NONNULL Port *
port_new_with_type_and_owner (
  PortType      type,
  PortFlow      flow,
  const char *  label,
  PortOwnerType owner_type,
  void *        owner);

/**
 * Allocates buffers used during DSP.
 *
 * To be called only where necessary to save
 * RAM.
 */
NONNULL void
port_allocate_bufs (Port * self);

/**
 * Frees buffers.
 *
 * To be used when removing ports from the
 * project/graph.
 */
NONNULL void
port_free_bufs (Port * self);

/**
 * Creates blank stereo ports.
 */
NONNULL StereoPorts *
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
  const char *  symbol,
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
NONNULL void
stereo_ports_connect (StereoPorts * src, StereoPorts * dest, int locked);

NONNULL void
stereo_ports_disconnect (StereoPorts * self);

StereoPorts *
stereo_ports_clone (const StereoPorts * src);

NONNULL void
stereo_ports_free (StereoPorts * self);

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
  void *       user_data,
  uint32_t *   size,
  uint32_t *   type);

#ifdef HAVE_JACK
/**
 * Receives MIDI data from the port's exposed
 * JACK port (if any) into the port.
 */
NONNULL void
port_receive_midi_events_from_jack (
  Port *          self,
  const nframes_t start_frames,
  const nframes_t nframes);

/**
 * Receives audio data from the port's exposed
 * JACK port (if any) into the port.
 */
NONNULL void
port_receive_audio_data_from_jack (
  Port *          self,
  const nframes_t start_frames,
  const nframes_t nframes);
#endif

/**
 * If MIDI port, returns if there are any events,
 * if audio port, returns if there is sound in the
 * buffer.
 */
NONNULL bool
port_has_sound (Port * self);

/**
 * Copies a full designation of \p self in the
 * format "Track/Port" or "Track/Plugin/Port" in
 * \p buf.
 */
NONNULL void
port_get_full_designation (Port * const self, char * buf);

NONNULL void
port_print_full_designation (Port * const self);

/**
 * Gathers all ports in the project and appends them
 * in the given array.
 */
void
port_get_all (GPtrArray * ports);

NONNULL Track *
port_get_track (const Port * const self, int warn_if_fail);

NONNULL Plugin *
port_get_plugin (Port * const self, const bool warn_if_fail);

/**
 * To be called when the port's identifier changes
 * to update corresponding identifiers.
 *
 * @param prev_id Previous identifier to be used
 *   for searching.
 * @param track The track that owns this port.
 * @param update_automation_track Whether to update
 *   the identifier in the corresponding automation
 *   track as well. This should be false when
 *   moving a plugin.
 */
void
port_update_identifier (
  Port *                 self,
  const PortIdentifier * prev_id,
  Track *                track,
  bool                   update_automation_track);

#ifdef HAVE_RTMIDI
/**
 * Dequeue the midi events from the ring
 * buffers into \ref RtMidiDevice.events.
 */
NONNULL void
port_prepare_rtmidi_events (Port * self);

/**
 * Sums the inputs coming in from RtMidi
 * before the port is processed.
 */
NONNULL void
port_sum_data_from_rtmidi (
  Port *          self,
  const nframes_t start_frame,
  const nframes_t nframes);
#endif

#ifdef HAVE_RTAUDIO
/**
 * Dequeue the audio data from the ring
 * buffers into \ref RtAudioDevice.buf.
 */
NONNULL void
port_prepare_rtaudio_data (Port * self);

/**
 * Sums the inputs coming in from RtAudio
 * before the port is processed.
 */
NONNULL void
port_sum_data_from_rtaudio (
  Port *          self,
  const nframes_t start_frame,
  const nframes_t nframes);
#endif

/**
 * Disconnects all hardware inputs from the port.
 */
NONNULL void
port_disconnect_hw_inputs (Port * self);

/**
 * Sets whether to expose the port to the backend
 * and exposes it or removes it.
 *
 * It checks what the backend is using the engine's
 * audio backend or midi backend settings.
 */
NONNULL void
port_set_expose_to_backend (Port * self, int expose);

/**
 * Returns if the port is exposed to the backend.
 */
NONNULL PURE static inline bool
port_is_exposed_to_backend (const Port * self)
{
  return self->internal_type == INTERNAL_JACK_PORT
         || self->internal_type == INTERNAL_ALSA_SEQ_PORT
         || self->id.owner_type == PORT_OWNER_TYPE_AUDIO_ENGINE
         || self->exposed_to_backend;
}

/**
 * Renames the port on the backend side.
 */
NONNULL void
port_rename_backend (Port * self);

#ifdef HAVE_ALSA

/**
 * Returns the Port matching the given ALSA
 * sequencer port ID.
 */
Port *
port_find_by_alsa_seq_id (const int id);
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
NONNULL void
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
NONNULL HOT float
port_get_control_value (Port * self, const bool normalize);

/**
 * Connects @ref a and @ref b with default
 * settings:
 * - enabled
 * - multiplier 1.0
 */
#define port_connect(a, b, locked) \
  port_connections_manager_ensure_connect ( \
    PORT_CONNECTIONS_MGR, &((a)->id), &((b)->id), 1.f, locked, true)

/**
 * Removes the connection between the given ports.
 */
#define port_disconnect(a, b) \
  port_connections_manager_ensure_disconnect ( \
    PORT_CONNECTIONS_MGR, &((a)->id), &((b)->id))

/**
 * Returns the number of unlocked (user-editable)
 * sources.
 */
NONNULL int
port_get_num_unlocked_srcs (const Port * self);

/**
 * Returns the number of unlocked (user-editable)
 * destinations.
 */
NONNULL int
port_get_num_unlocked_dests (const Port * self);

/**
 * Updates the track name hash on a track port and
 * all its source/destination identifiers.
 *
 * @param track Owner track.
 * @param hash The new hash.
 */
void
port_update_track_name_hash (Port * self, Track * track, unsigned int new_hash);

/**
 * Apply given fader value to port.
 *
 * @param start_frame The start frame offset from
 *   0 in this cycle.
 * @param nframes The number of frames to process.
 */
NONNULL static inline void
port_apply_fader (
  Port *          port,
  float           amp,
  nframes_t       start_frame,
  const nframes_t nframes)
{
  nframes_t end = start_frame + nframes;
  while (start_frame < end)
    {
      port->buf[start_frame++] *= amp;
    }
}

/**
 * First sets port buf to 0, then sums the given port signal from its inputs.
 *
 * @param noroll Clear the port buffer in this range.
 */
HOT NONNULL void
port_process (
  Port *                      port,
  const EngineProcessTimeInfo time_nfo,
  const bool                  noroll);

#define ports_connected(a, b) \
  (port_connections_manager_find_connection ( \
     PORT_CONNECTIONS_MGR, &(a)->id, &(b)->id) \
   != NULL)

/**
 * Returns whether the Port's can be connected (if
 * the connection will be valid and won't break the
 * acyclicity of the graph).
 */
NONNULL bool
ports_can_be_connected (const Port * src, const Port * dest);

/**
 * Disconnects all the given ports.
 */
NONNULL void
ports_disconnect (Port ** ports, int num_ports, int deleting);

/**
 * Copies the metadata from a project port to
 * the given port.
 *
 * Used when doing delete actions so that ports
 * can be restored on undo.
 */
NONNULL void
port_copy_metadata_from_project (Port * self, Port * project_port);

/**
 * Copies the port values from @ref other to @ref
 * self.
 *
 * @param self
 * @param other
 */
NONNULL void
port_copy_values (Port * self, const Port * other);

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
NONNULL void
port_restore_from_non_project (Port * self, Port * non_project);

/**
 * Clears the port buffer.
 *
 * @note Only the Zrythm buffer is cleared. Use
 * port_clear_external_buffer() to clear backend buffers.
 */
#define port_clear_buffer(_port) \
  { \
    if (_port->id.type == TYPE_AUDIO || _port->id.type == TYPE_CV) \
      { \
        if (_port->buf) \
          { \
            dsp_fill ( \
              _port->buf, DENORMAL_PREVENTION_VAL, AUDIO_ENGINE->block_length); \
          } \
      } \
    else if (_port->id.type == TYPE_EVENT) \
      { \
        if (_port->midi_events) \
          _port->midi_events->num_events = 0; \
      } \
  }

/**
 * Clears the backend's port buffer.
 */
HOT NONNULL OPTIMIZE_O3 void
port_clear_external_buffer (Port * port);

/**
 * Disconnects all srcs and dests from port.
 */
NONNULL int
port_disconnect_all (Port * port);

/**
 * Applies the pan to the given L/R ports.
 */
NONNULL void
port_apply_pan_stereo (
  Port *       l,
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
NONNULL void
port_apply_pan (
  Port *          port,
  float           pan,
  PanLaw          pan_law,
  PanAlgorithm    pan_algo,
  nframes_t       start_frame,
  const nframes_t nframes);

/**
 * Generates a hash for a given port.
 */
NONNULL uint32_t
port_get_hash (const void * ptr);

/**
 * To be used during serialization.
 */
Port *
port_clone (const Port * src);

/**
 * Deletes port, doing required cleanup and updating counters.
 */
NONNULL void
port_free (Port * port);

/**
 * @}
 */

#endif
