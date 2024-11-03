// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_PORTS_H__
#define __AUDIO_PORTS_H__

#include "zrythm-config.h"

#include <string>
#include <vector>

#include "common/dsp/midi_event.h"
#include "common/dsp/port_connection.h"
#include "common/dsp/port_identifier.h"
#include "common/utils/ring_buffer.h"
#include "common/utils/types.h"

#if HAVE_RTMIDI
#  include <rtmidi_c.h>
#endif

#if HAVE_RTAUDIO
#  include <rtaudio_c.h>
#endif

namespace zrythm::plugins
{
class Plugin;
struct PluginGtkController;
}

class MidiEvents;
class Fader;
class Channel;
class AudioEngine;
class Track;
class TrackProcessor;
class ModulatorMacroProcessor;
class RtMidiDevice;
class RtAudioDevice;
class AutomationTrack;
class TruePeakDsp;
class ExtPort;
class AudioClip;
class PortConnectionsManager;
class ChannelSend;
class Transport;
struct EngineProcessTimeInfo;
enum class PanAlgorithm;
enum class PanLaw;

/**
 * @addtogroup dsp
 *
 * @{
 */

constexpr int PORT_MAGIC = 456861194;
#define IS_PORT(_p) (((Port *) (_p))->magic_ == PORT_MAGIC)
#define IS_PORT_AND_NONNULL(x) ((x) && IS_PORT (x))

constexpr int TIME_TO_RESET_PEAK = 4800000;

template <typename T>
concept FinalPortSubclass = std::derived_from<T, Port> && FinalClass<T>;

class Port : public ISerializable<Port>
{
  Q_DISABLE_COPY_MOVE (Port)
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
  };

  ~Port () override { disconnect_all (); }

  static std::unique_ptr<Port> create_unique_from_type (PortType type);

  /**
   * This function finds the Ports corresponding to the PortIdentifiers for
   * srcs and dests.
   *
   * Should be called after the ports are deserialized from JSON.
   */
  template <typename T> void init_loaded (T * owner) { set_owner (owner); }

  /**
   * @brief Returns whether the port is in the active project.
   */
  bool is_in_active_project () const;

  template <typename T> void set_owner (T * owner);

  /**
   * Finds the Port corresponding to the identifier.
   *
   * @param id The PortIdentifier to use for searching.
   */
  template <typename T>
  static T * find_from_identifier (const PortIdentifier &id);

  static Port * find_from_identifier (const PortIdentifier &id);

  std::string get_label () const;

  inline bool is_control () const { return id_->is_control (); }
  inline bool is_audio () const { return id_->type_ == PortType::Audio; }
  inline bool is_cv () const { return id_->type_ == PortType::CV; }
  inline bool is_event () const { return id_->type_ == PortType::Event; }
  inline bool is_midi () const { return is_event (); }
  inline bool is_input () const { return id_->flow_ == PortFlow::Input; }
  inline bool is_output () const { return id_->flow_ == PortFlow::Output; }

  /**
   * @brief Allocates buffers used during DSP.
   *
   * To be called only where necessary to save RAM.
   */
  virtual void allocate_bufs () = 0;

  /**
   * Clears the port buffer.
   *
   * @note Only the Zrythm buffer is cleared. Use port_clear_external_buffer()
   * to clear backend buffers.
   */
  virtual void clear_buffer (AudioEngine &engine) = 0;

  /**
   * If MIDI port, returns if there are any events, if audio port, returns if
   * there is sound in the buffer.
   */
  virtual bool has_sound () const { return false; };

  /**
   * Gets a full designation of the port in the format "Track/Port" or
   * "Track/Plugin/Port".
   */
  std::string get_full_designation () const;

  void print_full_designation () const;

  Track * get_track (bool warn_if_fail) const;

  zrythm::plugins::Plugin * get_plugin (bool warn_if_fail) const;

  /**
   * To be called when the port's identifier changes to update corresponding
   * identifiers.
   *
   * @param prev_id Previous identifier to be used for searching.
   * @param track The track that owns this port.
   * @param update_automation_track Whether to update the identifier in the
   * corresponding automation track as well. This should be false when moving
   * a plugin.
   */
  void update_identifier (
    const PortIdentifier &prev_id,
    Track *               track,
    bool                  update_automation_track);

  /**
   * Disconnects all hardware inputs from the port.
   */
  void disconnect_hw_inputs ();

  /**
   * Sets whether to expose the port to the backend and exposes it or removes
   * it.
   *
   * It checks what the backend is using the engine's audio backend or midi
   * backend settings.
   */
  void set_expose_to_backend (AudioEngine &engine, bool expose);

  /**
   * Returns if the port is exposed to the backend.
   */
  inline bool is_exposed_to_backend () const
  {
    return internal_type_ == InternalType::JackPort
           || id_->owner_type_ == PortIdentifier::OwnerType::AudioEngine
           || exposed_to_backend_;
  }

  /**
   * Renames the port on the backend side.
   */
  void rename_backend ();

  /**
   * Returns the number of unlocked (user-editable) sources.
   */
  ATTR_NONNULL int get_num_unlocked_srcs () const;

  /**
   * Returns the number of unlocked (user-editable) destinations.
   */
  ATTR_NONNULL int get_num_unlocked_dests () const;

  /**
   * Updates the track name hash on a track port and all its
   * source/destination identifiers.
   *
   * @param track Owner track.
   * @param hash The new hash.
   */
  void update_track_name_hash (Track &track, unsigned int new_hash);

  /**
   * @brief Process the port for the given time range.
   *
   * @param noroll Whether to clear the port buffer in this range.
   */
  ATTR_HOT virtual void
  process (EngineProcessTimeInfo time_nfo, bool noroll) = 0;

  bool is_connected_to (const Port &dest) const;

  /**
   * Returns whether the Port's can be connected (if the connection will be
   * valid and won't break the acyclicity of the graph).
   */
  bool can_be_connected_to (const Port &dest) const;

  /**
   * Disconnects all the given ports.
   */
  ATTR_NONNULL static void
  disconnect_ports (std::vector<Port *> &ports, bool deleting);

  /**
   * Copies the metadata from a project port to the given port.
   *
   * Used when doing delete actions so that ports can be restored on undo.
   */
  virtual void copy_metadata_from_project (const Port &project_port) {};

  /**
   * Reverts the data on the corresponding project port for the given
   * non-project port.
   *
   * This restores src/dest connections and the control value.
   *
   * @param non_project Non-project port.
   */
  virtual void restore_from_non_project (const Port &non_project) {};

  /**
   * Clears the backend's port buffer.
   */
  ATTR_HOT void clear_external_buffer ();

  /**
   * Disconnects all srcs and dests from port.
   */
  void disconnect_all ();

  /**
   * Generates a hash for a given port.
   */
  uint32_t get_hash () const;

  /** Same as above, to be used as a callback. */
  static uint32_t get_hash (const void * ptr);

  bool     has_label () const { return !id_->label_.empty (); }
  PortType get_type () const { return id_->type_; }
  PortFlow get_flow () const { return id_->flow_; }

  /**
   * Connects to @p dest with default settings: { enabled: true,
   * multiplier: 1.0
   * }
   */
  void connect_to (PortConnectionsManager &mgr, Port &dest, bool locked);

  /**
   * @brief Removes the connection to @p dest.
   */
  void disconnect_from (PortConnectionsManager &mgr, Port &dest);

protected:
  Port ();

  Port (
    std::string label,
    PortType    type = (PortType) 0,
    PortFlow    flow = (PortFlow) 0,
    float       minf = 0.f,
    float       maxf = 1.f,
    float       zerof = 0.f);

  void copy_members_from (const Port &other);

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

private:
#if HAVE_JACK
  /**
   * Sums the inputs coming in from JACK, before the port is processed.
   */
  void sum_data_from_jack (nframes_t start_frame, nframes_t nframes);

  /**
   * Sends the port data to JACK, after the port is processed.
   */
  void send_data_to_jack (const nframes_t start_frame, const nframes_t nframes);

  /**
   * Sets whether to expose the port to JACK and exposes it or removes it from
   * JACK.
   */
  void expose_to_jack (bool expose);
#endif // HAVE_JACK

  int get_num_unlocked (bool sources) const;

public:
  /**
   * @brief Owned pointer.
   */
  PortIdentifier * id_ = nullptr;

  /**
   * Flag to indicate that this port is exposed to the backend.
   */
  bool exposed_to_backend_ = false;

  /** Caches filled when recalculating the graph. */
  std::vector<Port *> srcs_;

  /** Caches filled when recalculating the graph. */
  std::vector<Port *> dests_;

  /** Caches filled when recalculating the graph.
   *
   * The pointers are owned by this instance.
   */
  std::vector<std::unique_ptr<PortConnection>> src_connections_;
  std::vector<std::unique_ptr<PortConnection>> dest_connections_;

  /**
   * Indicates whether data or lv2_port should be used.
   */
  InternalType internal_type_ = ENUM_INT_TO_VALUE (InternalType, 0);

  /**
   * Minimum, maximum and zero values for this port.
   *
   * Note that for audio, this is the amplitude (0 - 2) and not the actual
   * values.
   */
  float minf_ = 0.f;
  float maxf_ = 1.f;

  /**
   * The zero position of the port.
   *
   * For example, in balance controls, this will be the middle. In audio
   * ports, this will be 0 amp (silence), etc.
   */
  float zerof_ = 0.f;

  /**
   * Pointer to arbitrary data.
   *
   * Use internal_type_ to check what data it is.
   *
   * FIXME just add the various data structs here and remove this ambiguity.
   */
  void * data_ = nullptr;

  /** Pointer to owner plugin, if any. */
  zrythm::plugins::Plugin * plugin_ = nullptr;

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

  /** used when loading projects FIXME needed? */
  int initialized_ = 0;

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
   * Buffer to be reallocated every time the buffer size changes.
   *
   * The buffer size is AUDIO_ENGINE->block_length_.
   *
   * Used only by CV and Audio ports.
   */
  std::vector<float> buf_;

  /**
   * Ring buffer for saving the contents of the audio buffer to be used in the
   * UI instead of directly accessing the buffer.
   *
   * This should contain blocks of block_length samples and should maintain at
   * least 10 cycles' worth of buffers.
   *
   * This is also used for CV.
   *
   * FIXME should be moved to a class inherited by AudioPort and CVPort.
   */
  std::unique_ptr<RingBuffer<float>> audio_ring_;

  /** Pointer to ExtPort, if hw. */
  ExtPort * ext_port_ = nullptr;

  /** Magic number to identify that this is a Port. */
  int magic_ = PORT_MAGIC;

  /** Last allocated buffer size (used for audio ports). */
  size_t last_buf_sz_ = 0;
};

class MidiPort;
class AudioPort;
class CVPort;
class ControlPort;
using PortVariant = std::variant<MidiPort, AudioPort, CVPort, ControlPort>;
using PortPtrVariant = to_pointer_variant<PortVariant>;

class HardwareProcessor;
class RecordableTrack;
class TempoTrack;
extern template MidiPort *
Port::find_from_identifier<MidiPort> (const PortIdentifier &);
extern template AudioPort *
Port::find_from_identifier (const PortIdentifier &);
extern template CVPort *
Port::find_from_identifier (const PortIdentifier &);
extern template ControlPort *
Port::find_from_identifier (const PortIdentifier &);
extern template void
Port::set_owner<zrythm::plugins::Plugin> (zrythm::plugins::Plugin *);
extern template void
Port::set_owner<Transport> (Transport *);
extern template void
Port::set_owner<ChannelSend> (ChannelSend *);
extern template void
Port::set_owner<AudioEngine> (AudioEngine *);
extern template void
Port::set_owner<Fader> (Fader *);
extern template void
Port::set_owner<Track> (Track *);
extern template void
Port::set_owner<ModulatorMacroProcessor> (ModulatorMacroProcessor *);
extern template void
Port::set_owner<ExtPort> (ExtPort *);
extern template void
Port::set_owner<Channel> (Channel *);
extern template void
Port::set_owner<TrackProcessor> (TrackProcessor *);
extern template void
Port::set_owner<HardwareProcessor> (HardwareProcessor *);
extern template void
Port::set_owner<TempoTrack> (TempoTrack *);
extern template void
Port::set_owner<RecordableTrack> (RecordableTrack *);

/**
 * @}
 */

#endif
