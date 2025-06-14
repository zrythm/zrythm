// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include "dsp/graph_node.h"
#include "dsp/port_connection.h"
#include "dsp/port_identifier.h"
#include "utils/audio.h"
#include "utils/ring_buffer.h"
#include "utils/types.h"
#include "utils/uuid_identifiable_object.h"

namespace zrythm::dsp
{
class PortConnectionsManager;
}

template <typename T>
concept FinalPortSubclass = std::derived_from<T, Port> && FinalClass<T>;

class PortRange
{
public:
  PortRange () = default;

  PortRange (float min, float max, float zero = 0.f)
      : minf_ (min), maxf_ (max), zerof_ (zero)
  {
  }

  float clamp_to_range (float val) const
  {
    return std::clamp (val, minf_, maxf_);
  }

private:
  static constexpr std::string_view kMinKey = "minf";
  static constexpr std::string_view kMaxKey = "maxf";
  static constexpr std::string_view kZeroKey = "zerof";
  friend void to_json (nlohmann::json &j, const PortRange &p)
  {
    j[kMinKey] = p.minf_;
    j[kMaxKey] = p.maxf_;
    j[kZeroKey] = p.zerof_;
  }
  friend void from_json (const nlohmann::json &j, PortRange &p)
  {
    j.at (kMinKey).get_to (p.minf_);
    j.at (kMaxKey).get_to (p.maxf_);
    j.at (kZeroKey).get_to (p.zerof_);
  }

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
  using TrackUuid = dsp::PortIdentifier::TrackUuid;
  using PluginUuid = dsp::PortIdentifier::PluginUuid;
  using PortUuid = dsp::PortIdentifier::PortUuid;

  virtual ~IPortOwner () = default;

  /**
   * @brief Function that will be called by the Port to update the identifier's
   * relevant members based on this port owner.
   *
   * @param id The identifier to update.
   */
  virtual void
  set_port_metadata_from_owner (dsp::PortIdentifier &id, PortRange &range)
    const = 0;

  virtual utils::Utf8String
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
  virtual void on_control_change_event (
    const PortUuid            &port_uuid,
    const dsp::PortIdentifier &id,
    float                      val) { };

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
class Port
    : public virtual dsp::graph::IProcessable,
      public utils::UuidIdentifiableObject<Port>
{
  Z_DISABLE_COPY_MOVE (Port)
public:
  using PortIdentifier = dsp::PortIdentifier;
  using PortType = dsp::PortType;
  using PortFlow = dsp::PortFlow;
  using PortConnection = dsp::PortConnection;

  ~Port () override = default;

  /**
   * This function finds the Ports corresponding to the PortIdentifiers for
   * srcs and dests.
   *
   * Should be called after the ports are deserialized from JSON.
   */
  void init_loaded (IPortOwner &owner) { set_owner (owner); }

  void set_owner (IPortOwner &owner);

  utils::Utf8String get_label () const;

  bool is_control () const { return id_->is_control (); }
  bool is_audio () const { return id_->type_ == PortType::Audio; }
  bool is_cv () const { return id_->type_ == PortType::CV; }
  bool is_event () const { return id_->type_ == PortType::Event; }
  bool is_midi () const { return is_event (); }
  bool is_input () const { return id_->flow_ == PortFlow::Input; }
  bool is_output () const { return id_->flow_ == PortFlow::Output; }

  utils::Utf8String get_node_name () const override
  {
    return get_full_designation ();
  }

  /**
   * @brief Ports have no latency.
   */
  nframes_t get_single_playback_latency () const override { return 0; }

  /**
   * Clears the port buffer.
   */
  virtual void clear_buffer (std::size_t block_length) = 0;

  /**
   * If MIDI port, returns if there are any events, if audio port, returns if
   * there is sound in the buffer.
   */
  virtual bool has_sound () const { return false; };

  /**
   * Gets a full designation of the port in the format "Track/Port" or
   * "Track/Plugin/Port".
   */
  utils::Utf8String get_full_designation () const
  {
    return (owner_ != nullptr)
             ? owner_->get_full_designation_for_port (*id_)
             : get_label ();
  }

  void print_full_designation () const;

  /**
   * Updates the owner track identifier.
   */
  void change_track (IPortOwner::TrackUuid new_track_id);

  /**
   * Copies the metadata from a project port to the given port.
   *
   * Used when doing delete actions so that ports can be restored on undo.
   */
  virtual void copy_metadata_from_project (const Port &project_port) { };

  /**
   * Reverts the data on the corresponding project port for the given
   * non-project port.
   *
   * This restores src/dest connections and the control value.
   *
   * @param non_project Non-project port.
   */
  virtual void restore_from_non_project (const Port &non_project) { };

  /**
   * Generates a hash for a given port.
   */
  size_t get_hash () const;

  bool     has_label () const { return !id_->label_.empty (); }
  PortType get_type () const { return id_->type_; }
  PortFlow get_flow () const { return id_->flow_; }

protected:
  Port ();

  Port (
    utils::Utf8String label,
    PortType          type = {},
    PortFlow          flow = {},
    float             minf = 0.f,
    float             maxf = 1.f,
    float             zerof = 0.f);

  friend void
  init_from (Port &obj, const Port &other, utils::ObjectCloneType clone_type);

private:
  static constexpr std::string_view kIdKey = "id";
  friend void to_json (nlohmann::json &j, const Port &p) { j[kIdKey] = p.id_; }
  friend void from_json (const nlohmann::json &j, Port &p)
  {
    j.at (kIdKey).get_to (p.id_);
  }

public:
  /**
   * @brief Owned pointer.
   */
  std::unique_ptr<PortIdentifier> id_;

public:
  IPortOwner * owner_{};
  PortRange    range_;
};

class RingBufferOwningPortMixin
{
public:
  virtual ~RingBufferOwningPortMixin () = default;

  /**
   * @brief Number of entities that want ring buffers to be written.
   *
   * This is used to avoid writing ring buffers when they are not needed by any
   * entity. If a UI element needs ring buffers filled, this should be
   * incremented, and then decremented when no longer needed.
   *
   * @note Significant ports like master output will write to their ring buffers
   * anyway.
   */
  std::atomic<int> num_ring_buffer_readers_{ 0 };
};

template <typename PortT> class PortConnectionsCacheMixin
{
  using ElementType =
    std::pair<const PortT *, std::unique_ptr<dsp::PortConnection>>;

public:
  virtual ~PortConnectionsCacheMixin () = default;
  /**
   * @brief Caches filled when recalculating the graph.
   *
   * Used during processing.
   */
  std::vector<ElementType> port_sources_;
  // std::vector<ElementType> port_destinations_;
};

class AudioAndCVPortMixin
    : public virtual dsp::graph::IProcessable,
      public RingBufferOwningPortMixin
{
public:
  ~AudioAndCVPortMixin () override = default;

  void
  prepare_for_processing (sample_rate_t sample_rate, nframes_t max_block_length)
    final
  {
    size_t max = std::max (max_block_length, 1u);
    buf_.resize (max);

    // 8 cycles
    audio_ring_ = std::make_unique<RingBuffer<float>> (max * 8);
  }

  void release_resources () final
  {
    buf_.clear ();
    audio_ring_.reset ();
  }

public:
  /**
   * Audio-like data buffer.
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
   */
  std::unique_ptr<RingBuffer<float>> audio_ring_;
};

class MidiPort;
class AudioPort;
class CVPort;
class ControlPort;
using PortVariant = std::variant<MidiPort, AudioPort, CVPort, ControlPort>;
using PortPtrVariant = to_pointer_variant<PortVariant>;
using PortRefVariant = to_reference_variant<PortVariant>;

using PortRegistry = utils::OwningObjectRegistry<PortPtrVariant, Port>;
using PortRegistryRef = std::reference_wrapper<PortRegistry>;
using PortUuidReference = utils::UuidReference<PortRegistry>;

void
from_json (const nlohmann::json &j, PortRegistry &registry);
