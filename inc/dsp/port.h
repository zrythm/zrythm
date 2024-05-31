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

#include <string>
#include <vector>

#include "dsp/port_identifier.h"
#include "utils/types.h"

#include "juce/juce.h"
#include "zix/sem.h"

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
enum class PanAlgorithm;
enum class PanLaw;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define PORT_MAGIC 456861194
#define IS_PORT(_p) (((Port *) (_p))->magic_ == PORT_MAGIC)
#define IS_PORT_AND_NONNULL(x) ((x) && IS_PORT (x))

#define TIME_TO_RESET_PEAK 4800000

/**
 * Special ID for owner_pl, owner_ch, etc. to indicate that
 * the port is not owned.
 */
#define PORT_NOT_OWNED -1

class Port
{
public:
  /**
   * What the internal data is.
   */
  enum class InternalType
  {
    None,

    /** Pointer to Lv2Port. */
    Lv2Port,

    /** Pointer to jack_port_t. */
    JackPort,

    /** TODO */
    PulseAudioPort,

    /** Pointer to snd_seq_port_info_t. */
    AlsaSequencerPort,
  };

  /**
   * Scale point.
   */
  struct ScalePoint
  {
    float       val_;
    std::string label_;

    ScalePoint (float val, std::string label);
    ~ScalePoint () = default;

    bool operator< (const ScalePoint &other) { return val_ - other.val_ > 0.f; }
  };

  Port () = default;
  Port (std::string label);
  Port (PortType type, PortFlow flow, std::string label);
  Port (
    PortType                  type,
    PortFlow                  flow,
    std::string               label,
    PortIdentifier::OwnerType owner_type,
    void *                    owner);
  // Port (const Port &other) { *this = other; } /// copy ctor

  // make it uncopyable because some members are not copyable atm (like
  // rtmididevice)
  Port (const Port &other) = delete;
  // move ctor
  Port (Port &&other) noexcept = default; //: Port () { swap (*this, other); }

#if 0
  friend void swap (Port &a, Port &b)
  {
    using std::swap;

#  define _SWAP(x) swap (a.x, b.x)

    _SWAP (id_);
    _SWAP (exposed_to_backend_);
    _SWAP (buf_);
    _SWAP (midi_events_);
    _SWAP (srcs_);
    _SWAP (dests_);
    _SWAP (src_connections_);
    _SWAP (dest_connections_);
    _SWAP (internal_type_);
    _SWAP (minf_);
    _SWAP (maxf_);
    _SWAP (zerof_);
    _SWAP (deff_);
    _SWAP (carla_param_id_);
    _SWAP (data_);
    _SWAP (mme_connections_);
    _SWAP (mme_connections_sem_);
    _SWAP (last_midi_dequeue_);
    _SWAP (rtmidi_ins_);
    _SWAP (rtmidi_outs_);
    _SWAP (rtaudio_ins_);
    _SWAP (scale_points_);
    _SWAP (control_);
    _SWAP (unsnapped_control_);
    _SWAP (value_changed_from_reading_);
    _SWAP (last_change_);
    _SWAP (plugin_);
    _SWAP (transport_);
    _SWAP (channel_send_);
    _SWAP (engine_);
    _SWAP (fader_);
    _SWAP (track_);
    _SWAP (modulator_macro_processor_);
    _SWAP (tmp_plugin_);
    _SWAP (initialized_);
    _SWAP (base_value_);
    _SWAP (capture_latency_);
    _SWAP (playback_latency_);
    _SWAP (deleting_);
    _SWAP (write_ring_buffers_);
    _SWAP (has_midi_events_);
    _SWAP (last_midi_event_time_);
    _SWAP (audio_ring_);
    _SWAP (midi_ring_);
    _SWAP (peak_);
    _SWAP (peak_timestamp_);
    _SWAP (last_midi_status_);
    _SWAP (widget_);
    _SWAP (received_ui_event_);
    _SWAP (automating_);
    _SWAP (at_);
    _SWAP (midi_channel_);
    _SWAP (midi_cc_no_);
    _SWAP (ext_port_);
    _SWAP (magic_);
    _SWAP (last_buf_sz_);

#  undef _SWAP
  }
#endif

  Port &operator= (Port other) = delete;
#if 0
  Port &operator= (Port other)
  {
    swap (*this, other);
    return *this;
  }
#endif

  Port clone ()
  {
    Port self = Port ();

    self.id_ = this->id_;

#define COPY_MEMBER(x) self.x = this->x

    COPY_MEMBER (exposed_to_backend_);
    COPY_MEMBER (control_);
    COPY_MEMBER (minf_);
    COPY_MEMBER (maxf_);
    COPY_MEMBER (zerof_);
    COPY_MEMBER (deff_);
    COPY_MEMBER (carla_param_id_);

#undef COPY_MEMBER

    return self;
  }

  /** @warning This disconnects the port from various places. */
  ~Port ();

  /**
   * This function finds the Ports corresponding to the PortIdentifiers for
   * srcs and dests.
   *
   * Should be called after the ports are deserialized from JSON.
   */
  NONNULL void init_loaded (void * owner);

  /**
   * @brief Returns whether the port is in the active project.
   */
  bool is_in_active_project () const;

  void set_owner (PortIdentifier::OwnerType owner_type, void * owner);

  NONNULL static Port * find_from_identifier (const PortIdentifier * const id);

  const std::string &get_label () const { return this->id_.get_label (); }

  const char * get_label_as_c_str () const { return id_.get_label ().c_str (); }

  /**
   * @brief Allocates buffers used during DSP.
   *
   * To be called only where necessary to save RAM.
   */
  void allocate_bufs ();

  /**
   * @brief Frees buffers.
   *
   * To be used when removing ports from the project/graph.
   */
  void free_bufs ();

  /**
   * Clears the port buffer.
   *
   * @note Only the Zrythm buffer is cleared. Use port_clear_external_buffer()
   * to clear backend buffers.
   */
  void clear_buffer (AudioEngine &engine);

  /**
   * Function to get a port's value from its string symbol.
   *
   * Used when saving the LV2 state. This function MUST set size and type
   * appropriately.
   */
  static const void * get_value_from_symbol (
    const char * port_sym,
    void *       user_data,
    uint32_t *   size,
    uint32_t *   type);

#ifdef HAVE_JACK
  /**
   * Receives MIDI data from the port's exposed JACK port (if any) into the port.
   */
  void receive_midi_events_from_jack (
    const nframes_t start_frames,
    const nframes_t nframes);

  /**
   * Receives audio data from the port's exposed JACK port (if any) into the
   * port.
   */
  void receive_audio_data_from_jack (
    const nframes_t start_frames,
    const nframes_t nframes);
#endif

  /**
   * If MIDI port, returns if there are any events, if audio port, returns if
   * there is sound in the buffer.
   */
  bool has_sound () const;

  /**
   * Copies a full designation of the port in the format "Track/Port" or
   * "Track/Plugin/Port" in @p buf.
   */
  NONNULL void get_full_designation (char * buf) const;

  void print_full_designation () const;

  /**
   * Gathers all ports in the project and appends them in the given array.
   */
  static void get_all (GPtrArray * ports);

  Track * get_track (bool warn_if_fail) const;

  Plugin * get_plugin (bool warn_if_fail) const;

  /**
   * To be called when the port's identifier changes to update corresponding
   * identifiers.
   *
   * @param prev_id Previous identifier to be used for searching.
   * @param track The track that owns this port.
   * @param update_automation_track Whether to update the identifier in the
   * corresponding automation track as well. This should be false when moving a
   * plugin.
   */
  void update_identifier (
    const PortIdentifier * prev_id,
    Track *                track,
    bool                   update_automation_track);

#ifdef HAVE_RTMIDI
  /**
   * Dequeue the midi events from the ring buffers into \ref RtMidiDevice.events.
   */
  void prepare_rtmidi_events ();

  /**
   * Sums the inputs coming in from RtMidi before the port is processed.
   */
  void
  sum_data_from_rtmidi (const nframes_t start_frame, const nframes_t nframes);
#endif

#ifdef HAVE_RTAUDIO
  /**
   * Dequeue the audio data from the ring buffers into @ref RtAudioDevice.buf.
   */
  void prepare_rtaudio_data ();

  /**
   * Sums the inputs coming in from RtAudio before the port is processed.
   */
  void
  sum_data_from_rtaudio (const nframes_t start_frame, const nframes_t nframes);
#endif

  /**
   * Disconnects all hardware inputs from the port.
   */
  void disconnect_hw_inputs ();

  /**
   * Sets whether to expose the port to the backend and exposes it or removes it.
   *
   * It checks what the backend is using the engine's audio backend or midi
   * backend settings.
   */
  void set_expose_to_backend (bool expose);

  /**
   * Returns if the port is exposed to the backend.
   */
  bool is_exposed_to_backend () const
  {
    return internal_type_ == InternalType::JackPort
           || internal_type_ == InternalType::AlsaSequencerPort
           || id_.owner_type_ == PortIdentifier::OwnerType::PORT_OWNER_TYPE_AUDIO_ENGINE
           || exposed_to_backend_;
  }

  /**
   * Renames the port on the backend side.
   */
  void rename_backend ();

#ifdef HAVE_ALSA

  /**
   * Returns the Port matching the given ALSA sequencer port ID.
   */
  static Port * find_by_alsa_seq_id (const int id);
#endif

  /**
   * Sets the given control value to the corresponding underlying structure in
   the Port.
   *
   * Note: this is only for setting the base values (eg when automating via an
   automation lane). For CV automations this should not be used.
   *
   * @param is_normalized Whether the given value is normalized between 0 and 1.
   * @param forward_event Whether to forward a port   control change event to
   the plugin UI. Only
   *   applicable for plugin control ports. If the control is being changed
   manually or from within Zrythm, this should be true to notify the plugin of
   the change.
   */
  void set_control_value (
    const float val,
    const bool  is_normalized,
    const bool  forward_event);

  /**
   * Gets the given control value from the corresponding underlying structure in
   * the Port.
   *
   * @param normalize Whether to get the value normalized or not.
   */
  HOT float get_control_value (const bool normalize) const;

  /**
   * Returns the number of unlocked (user-editable) sources.
   */
  NONNULL int get_num_unlocked_srcs () const;

  /**
   * Returns the number of unlocked (user-editable) destinations.
   */
  NONNULL int get_num_unlocked_dests () const;

  /**
   * Updates the track name hash on a track port and all its source/destination
   * identifiers.
   *
   * @param track Owner track.
   * @param hash The new hash.
   */
  void update_track_name_hash (Track * track, unsigned int new_hash);

  /**
   * Apply given fader value to port.
   *
   * @param start_frame The start frame offset from 0 in this cycle.
   * @param nframes The number of frames to process.
   */
  void apply_fader (float amp, nframes_t start_frame, const nframes_t nframes);

  /**
   * First sets port buf to 0, then sums the given port signal from its inputs.
   *
   * @param noroll Clear the port buffer in this range.
   */
  HOT void process (const EngineProcessTimeInfo time_nfo, const bool noroll);

  bool is_connected_to (const Port * dest) const;

  /**
   * Returns whether the Port's can be connected (if the connection will be
   * valid and won't break the acyclicity of the graph).
   */
  bool can_be_connected_to (const Port * dest) const;

  /**
   * Disconnects all the given ports.
   */
  NONNULL static void
  disconnect_ports (Port ** ports, int num_ports, bool deleting);

  /**
   * Copies the metadata from a project port to the given port.
   *
   * Used when doing delete actions so that ports can be restored on undo.
   */
  NONNULL void copy_metadata_from_project (Port * project_port);

  /**
   * Copies the port values from @ref other.
   *
   * @param other
   */
  void copy_values (const Port &other);

  /**
   * Reverts the data on the corresponding project port for the given
   * non-project port.
   *
   * This restores src/dest connections and the control value.
   *
   * @param non_project Non-project port.
   */
  void restore_from_non_project (Port &non_project);

  /**
   * Clears the backend's port buffer.
   */
  HOT OPTIMIZE_O3 void clear_external_buffer ();

  /**
   * Disconnects all srcs and dests from port.
   */
  int disconnect_all ();

  /**
   * Applies the pan to the given port.
   *
   * @param start_frame The start frame offset from 0 in this cycle.
   * @param nframes The number of frames to process.
   */
  void apply_pan (
    float           pan,
    PanLaw          pan_law,
    PanAlgorithm    pan_algo,
    nframes_t       start_frame,
    const nframes_t nframes);

  /**
   * Generates a hash for a given port.
   */
  uint32_t get_hash () const;

  /** Same as above, to be used as a callback. */
  static uint32_t get_hash (const void * ptr);

  bool     has_label () const { return !id_.label_.empty (); }
  PortType get_type () const { return id_.type_; }
  PortFlow get_flow () const { return id_.flow_; }

  /**
   * Connects to @p dest with default settings: { enabled: true, multiplier: 1.0
   * }
   */
  void connect_to (Port &dest, bool locked);

  /**
   * @brief Removes the connection to @p dest.
   */
  void disconnect_from (Port &dest);

public:
  PortIdentifier id_ = PortIdentifier ();

  /**
   * Flag to indicate that this port is exposed to the backend.
   */
  bool exposed_to_backend_ = false;

  /**
   * Buffer to be reallocated every time the buffer size changes.
   *
   * The buffer size is AUDIO_ENGINE->block_length.
   */
  std::vector<float> buf_;

  /**
   * Contains raw MIDI data (MIDI ports only)
   */
  MidiEvents * midi_events_ = nullptr;

  /** Caches filled when recalculating the graph. */
  std::vector<Port *> srcs_;

  /** Caches filled when recalculating the graph. */
  std::vector<Port *> dests_;

  /** Caches filled when recalculating the graph. */
  std::vector<const PortConnection *> src_connections_;
  std::vector<const PortConnection *> dest_connections_;

  /**
   * Indicates whether data or lv2_port should be used.
   */
  InternalType internal_type_ = ENUM_INT_TO_VALUE (InternalType, 0);

  /**
   * Minimum, maximum and zero values for this port.
   *
   * Note that for audio, this is the amp (0 - 2) and not the actual values.
   */
  float minf_ = 0.f;
  float maxf_ = 0.f;

  /**
   * The zero position of the port.
   *
   * For example, in balance controls, this will be the middle. In audio ports,
   * this will be 0 amp (silence), etc.
   */
  float zerof_ = 0.f;

  /** Default value, only used for controls. */
  float deff_ = 0.f;

  /** Index of the control parameter (for Carla plugin ports). */
  int carla_param_id_ = 0;

  /**
   * Pointer to arbitrary data.
   *
   * Use internal_type_ to check what data it is.
   *
   * FIXME just add the various data structs here and remove this ambiguity.
   */
  void * data_ = nullptr;

#ifdef _WIN32
  /**
   * Connections to WindowsMmeDevices.
   *
   * These must be pointers to \ref AudioEngine.mme_in_devs or \ref
   * AudioEngine.mme_out_devs and must not be allocated or free'd.
   */
  std::vector<WindowsMmeDevice *> mme_connections_;
#else
  std::vector<void *> mme_connections_;
#endif

  /** Semaphore for changing the connections atomically. */
  ZixSem mme_connections_sem_ = {};

  /**
   * Last time the port finished dequeueing MIDI events.
   *
   * Used for some backends only.
   */
  gint64 last_midi_dequeue_ = {};

#ifdef HAVE_RTMIDI
  /**
   * RtMidi pointers for input ports.
   *
   * Each RtMidi port represents a device, and this Port can be connected to
   * multiple devices.
   */
  std::vector<RtMidiDevice *> rtmidi_ins_;

  /** RtMidi pointers for output ports. */
  std::vector<RtMidiDevice *> rtmidi_outs_;
#else
  std::vector<void *> rtmidi_ins_;
  std::vector<void *> rtmidi_outs_;
#endif

#ifdef HAVE_RTAUDIO
  /**
   * RtAudio pointers for input ports.
   *
   * Each port can have multiple RtAudio devices.
   */
  std::vector<RtAudioDevice *> rtaudio_ins_;
#else
  std::vector<void *> rtaudio_ins_;
#endif

  /** Scale points. */
  std::vector<ScalePoint> scale_points_;

  /**
   * @brief The control value if control port, otherwise 0.0f.
   *
   * FIXME for fader, this should be the fader_val (0.0 to 1.0) and not the
   * amplitude.
   *
   * This value will be snapped (eg, if integer or toggle).
   */
  float control_ = 0.f;

  /** Unsnapped value, used by widgets. */
  float unsnapped_control_ = 0.f;

  /** Flag that the value of the port changed from reading automation. */
  bool value_changed_from_reading_ = false;

  /**
   * Last timestamp the control changed.
   *
   * This is used when recording automation in "touch" mode.
   */
  gint64 last_change_ = 0;

  /** Pointer to owner plugin, if any. */
  Plugin * plugin_ = nullptr;

  /** Pointer to owner transport, if any. */
  Transport * transport_ = nullptr;

  /** Pointer to owner channel send, if any. */
  ChannelSend * channel_send_ = nullptr;

  /** Pointer to owner engine, if any. */
  AudioEngine * engine_ = nullptr;

  /** Pointer to owner fader, if any. */
  Fader * fader_ = nullptr;

  /**
   * Pointer to owner track, if any.
   *
   * Also used for channel and track processor ports.
   */
  Track * track_ = nullptr;

  /** Pointer to owner modulator macro processor, if any. */
  ModulatorMacroProcessor * modulator_macro_processor_ = nullptr;

  /* ====== flags to indicate port owner ====== */

  /**
   * Temporary plugin pointer (used when the plugin doesn't exist yet in its
   * supposed slot).
   * FIXME delete
   */
  mutable Plugin * tmp_plugin_ = nullptr;

  /** used when loading projects FIXME needed? */
  int initialized_ = 0;

  /**
   * For control ports, when a modulator is attached to the port the previous
   * value will be saved here.
   *
   * Automation in AutomationTrack's will overwrite this value.
   */
  float base_value_ = 0.f;

  /**
   * Capture latency.
   *
   * See page 116 of "The Ardour DAW - Latency Compensation and
   * Anywhere-to-Anywhere Signal Routing Systems".
   */
  long capture_latency_ = 0;

  /**
   * Playback latency.
   *
   * See page 116 of "The Ardour DAW - Latency Compensation and
   * Anywhere-to-Anywhere Signal Routing Systems".
   */
  long playback_latency_ = 0;

  /** Port undergoing deletion. */
  bool deleting_ = false;

  /**
   * Flag to indicate if the ring buffers below should be filled or not.
   *
   * If a UI element that needs them becomes mapped
   * (visible), this should be set to true, and when unmapped
   * (invisible) it should be set to false.
   */
  bool write_ring_buffers_ = false;

  /**
   * @brief Whether the port has midi events not yet processed by the UI.
   *
   * @note This is an int because atomic operations are called on it.
   */
  int has_midi_events_ = 0;

  /** Used by the UI to detect when unprocessed MIDI events exist. */
  gint64 last_midi_event_time_ = 0;

  /**
   * Ring buffer for saving the contents of the audio buffer to be used in the
   * UI instead of directly accessing the buffer.
   *
   * This should contain blocks of block_length samples and should maintain at
   * least 10 cycles' worth of buffers.
   *
   * This is also used for CV.
   */
  ZixRing * audio_ring_ = nullptr;

  /**
   * @brief Ring buffer for saving MIDI events to be used in the UI instead of
   * directly accessing the events.
   *
   * This should keep pushing MidiEvent's whenever they occur and the reader
   * should empty it after checking if there are any events.
   *
   * Currently there is only 1 reader for each port so this wont be a problem
   * for now, but we should have one ring for each reader.
   */
  ZixRing * midi_ring_ = nullptr;

  /** Max amplitude during processing, if audio (fabsf). */
  float peak_ = 0.f;

  /** Last time \ref Port.max_amp was set. */
  gint64 peak_timestamp_ = 0;

  /**
   * Last known MIDI status byte received.
   *
   * Used for running status (see
   * http://midi.teragonaudio.com/tech/midispec/run.htm).
   *
   * Not needed for JACK.
   */
  midi_byte_t last_midi_status_ = 0;

  /**
   * Control widget, if applicable.
   *
   * Only used for generic UIs.
   */
  PluginGtkController * widget_ = 0;

  /**
   * Whether the port received a UI event from the plugin UI in this cycle.
   *
   * This is used to avoid re-sending that event to the plugin.
   *
   * @note for control ports only.
   */
  bool received_ui_event_ = 0;

  /** Whether this value was set via automation. */
  bool automating_ = 0;

  /**
   * @brief Automation track this port is attached to.
   *
   * To be set at runtime only (not serialized).
   */
  AutomationTrack * at_ = nullptr;

  /*
   * Next 2 objects are MIDI CC info, if MIDI CC in track processor.
   *
   * Used as a cache.
   */

  /** MIDI channel, starting from 1. */
  midi_byte_t midi_channel_ = 0;

  /** MIDI CC number, if not pitchbend/poly key/channel pressure. */
  midi_byte_t midi_cc_no_ = 0;

  /** Pointer to ExtPort, if hw. */
  ExtPort * ext_port_ = nullptr;

  /** Magic number to identify that this is a Port. */
  int magic_ = PORT_MAGIC;

  /** Last allocated buffer size (used for audio ports). */
  size_t last_buf_sz_ = 0;

private:
  /**
   * Sums the inputs coming in from dummy, before the port is processed.
   */
  void
  sum_data_from_dummy (const nframes_t start_frame, const nframes_t nframes);
};

/**
 * L & R port, for convenience.
 */
class StereoPorts
{
public:
  StereoPorts () = default;
  // StereoPorts (const Port &l, const Port &r);
  StereoPorts (Port &&l, Port &&r);

  /**
   * Creates stereo ports for generic use.
   *
   * @param input Whether input ports.
   * @param owner Pointer to the owner. The type is determined by owner_type.
   */
  StereoPorts (
    bool                      input,
    std::string               name,
    std::string               symbol,
    PortIdentifier::OwnerType owner_type,
    void *                    owner);

  StereoPorts clone ()
  {
    StereoPorts self =
      StereoPorts (std::move (l_.clone ()), std::move (r_.clone ()));
    return self;
  }

  NONNULL void init_loaded (void * owner)
  {
    l_.init_loaded (owner);
    r_.init_loaded (owner);
  }

  void set_owner (PortIdentifier::OwnerType owner_type, void * owner)
  {
    l_.set_owner (owner_type, owner);
    r_.set_owner (owner_type, owner);
  }

  void set_expose_to_backend (bool expose)
  {
    l_.set_expose_to_backend (expose);
    r_.set_expose_to_backend (expose);
  }

  void disconnect_hw_inputs ()
  {
    l_.disconnect_hw_inputs ();
    r_.disconnect_hw_inputs ();
  }

  void clear_buffer (AudioEngine &engine)
  {
    l_.clear_buffer (engine);
    r_.clear_buffer (engine);
  }

  void allocate_bufs ()
  {
    l_.allocate_bufs ();
    r_.allocate_bufs ();
  }

  /**
   * Connects to the given port using Port::connect().
   *
   * @param dest Destination port.
   * @param locked Lock the connection.
   */
  NONNULL void connect_to (StereoPorts &dest, bool locked);

  void disconnect ();

  void set_write_ring_buffers (bool on)
  {
    l_.write_ring_buffers_ = on;
    r_.write_ring_buffers_ = on;
  }

  Port       &get_l () { return l_; }
  Port       &get_r () { return r_; }
  const Port &get_l () const { return l_; }
  const Port &get_r () const { return r_; }

private:
  Port l_; ///< Left port
  Port r_; ///< Right port
};

/**
 * @}
 */

#endif
