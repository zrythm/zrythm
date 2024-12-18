// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef ZRYTHM_GUI_DSP_PORT_H
#define ZRYTHM_GUI_DSP_PORT_H

#include "zrythm-config.h"

#include "dsp/port_identifier.h"
#include "gui/dsp/port_backend.h"
#include "gui/dsp/port_connection.h"
#include "utils/ring_buffer.h"
#include "utils/types.h"

class AudioEngine;
class Track;
struct EngineProcessTimeInfo;

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

class PortRange : public utils::serialization::ISerializable<PortRange>
{
public:
  PortRange () = default;

  PortRange (float min, float max, float zero = 0.f)
      : minf_ (min), maxf_ (max), zerof_ (zero)
  {
  }

  float clamp_to_range (float val) { return std::clamp (val, minf_, maxf_); }

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
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
};

class IPortOwner
{
public:
  virtual ~IPortOwner () = default;

  virtual bool is_in_active_project () const = 0;

  /**
   * @brief Function that will be called by the Port to update the identifier's
   * relevant members based on this port owner.
   *
   * @param id The identifier to update.
   */
  virtual void
  set_port_metadata_from_owner (dsp::PortIdentifier &id, PortRange &range)
    const = 0;

  virtual std::string
  get_full_designation_for_port (const dsp::PortIdentifier &id) const
  {
    return id.get_label ();
  };

  /**
   * @brief Will be called when a control port's value changes.
   *
   * @param val The real value of the control port.
   *
   * @attention This may be called from the audio thread so it must not block.
   */
  virtual void
  on_control_change_event (const dsp::PortIdentifier &id, float val) {};

  /**
   * @brief Called during processing if the MIDI port contains new MIDI events.
   */
  virtual void on_midi_activity (const dsp::PortIdentifier &id) { }

  /**
   * @brief Whether during processing, the port should sum the data from its
   * backend buffers coming in.
   *
   * If this is a TrackProcessor input port for a recordable track, this should
   * only return true when currently armed recording. Otherwise, we always
   * consider incoming external data.
   */
  virtual bool should_sum_data_from_backend () const { return true; }

  /**
   * @brief Whether the port should add its data to the master output when
   * bouncing.
   *
   * When bouncing a track directly to master (e.g., when bouncing the track on
   * its own without parents), the buffer should be added to the master track
   * output.
   *
   * This is only utilized for stereo output audio ports.
   */
  virtual bool should_bounce_to_master (utils::audio::BounceStep step) const
  {
    return false;
  }

  /**
   * @brief Returns whether MIDI events on this channel on an input port should
   * be processed (not ignored).
   *
   * This is used to implement MIDI channel filtering in tracks that have piano
   * rolls based on the enabled MIDI channels.
   */
  virtual bool are_events_on_midi_channel_approved (midi_byte_t channel) const
  {
    return true;
  }
};

/**
 * @brief The Port class represents a port in the audio processing graph.
 *
 * Ports can be of different types (audio, MIDI, CV, control) and can be
 * inputs or outputs. They are used to connect different components of the
 * audio processing graph, such as tracks, plugins, and the audio engine.
 *
 * Ports are owned by various components in the audio processing graph, such
 * as tracks, plugins, and the audio engine. The `set_owner()` method is used
 * to set the owner of the port.
 */
class Port : public utils::serialization::ISerializable<Port>, public dsp::IProcessable
{
  Q_DISABLE_COPY_MOVE (Port)
public:
  using PortIdentifier = dsp::PortIdentifier;
  using PortType = dsp::PortType;
  using PortFlow = dsp::PortFlow;

  ~Port () override { disconnect_all (); }

  static std::unique_ptr<Port> create_unique_from_type (PortType type);

  /**
   * This function finds the Ports corresponding to the PortIdentifiers for
   * srcs and dests.
   *
   * Should be called after the ports are deserialized from JSON.
   */
  void init_loaded (IPortOwner &owner) { set_owner (owner); }

  /**
   * @brief Returns whether the port is in the active project.
   */
  bool is_in_active_project () const
  {
    return owner_ && owner_->is_in_active_project ();
  }

  void set_owner (IPortOwner &owner);

  std::string get_label () const;

  bool is_control () const { return id_->is_control (); }
  bool is_audio () const { return id_->type_ == PortType::Audio; }
  bool is_cv () const { return id_->type_ == PortType::CV; }
  bool is_event () const { return id_->type_ == PortType::Event; }
  bool is_midi () const { return is_event (); }
  bool is_input () const { return id_->flow_ == PortFlow::Input; }
  bool is_output () const { return id_->flow_ == PortFlow::Output; }

  /**
   * @brief Allocates buffers used during DSP.
   *
   * To be called only where necessary to save RAM.
   */
  virtual void allocate_bufs () = 0;

  std::string get_node_name () const override
  {
    return get_full_designation ();
  }

  nframes_t get_single_playback_latency () const override { return 0; }

  ATTR_HOT void process_block (EngineProcessTimeInfo time_nfo) override;

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
  std::string get_full_designation () const
  {
    return (owner_ != nullptr)
             ? owner_->get_full_designation_for_port (*id_)
             : get_label ();
  }

  void print_full_designation () const;

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
  bool is_exposed_to_backend () const;

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
  ATTR_HOT void clear_external_buffer () override;

  bool needs_external_buffer_clear_on_early_return () const override;

  /**
   * Disconnects all srcs and dests from port.
   */
  void disconnect_all ();

  /**
   * Generates a hash for a given port.
   */
  uint32_t get_hash () const;

  bool     has_label () const { return !id_->label_.empty (); }
  PortType get_type () const { return id_->type_; }
  PortFlow get_flow () const { return id_->flow_; }

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

  int get_num_unlocked (bool sources) const;

public:
  /**
   * @brief Owned pointer.
   */
  std::unique_ptr<PortIdentifier> id_;

protected:
  /**
   * Flag to indicate that this port is exposed to the backend.
   *
   * @note This is used for serialization only.
   */
  bool exposed_to_backend_ = false;

public:
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

  PortRange range_;

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

  /** Magic number to identify that this is a Port. */
  int magic_ = PORT_MAGIC;

  /** Last allocated buffer size (used for audio ports). */
  size_t last_buf_sz_ = 0;

  /**
   * @brief Backend functionality.
   */
  std::unique_ptr<PortBackend> backend_;

  IPortOwner * owner_{};
};

class MidiPort;
class AudioPort;
class CVPort;
class ControlPort;
using PortVariant = std::variant<MidiPort, AudioPort, CVPort, ControlPort>;
using PortPtrVariant = to_pointer_variant<PortVariant>;

/**
 * @}
 */

#endif
