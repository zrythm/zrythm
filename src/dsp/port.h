// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph_node.h"
#include "dsp/port_connection.h"
#include "dsp/port_fwd.h"
#include "utils/types.h"
#include "utils/uuid_identifiable_object.h"

namespace zrythm::dsp
{

/**
 * @brief A base class for ports used for connecting processors in the DSP
 * graph.
 *
 * Ports can be of different types (audio, MIDI, CV) and can be inputs or
 * outputs. They are used to connect different components of the audio
 * processing graph, such as tracks, plugins, and the audio engine.
 *
 * Ports are owned by various processors in the audio processing graph, such
 * as tracks, plugins, etc., and ports themselves are part of the processing
 * graph.
 */
class Port
    : public dsp::graph::IProcessable,
      public utils::UuidIdentifiableObject<Port>
{
  Z_DISABLE_COPY_MOVE (Port)
public:
  using FullDesignationProvider =
    std::function<utils::Utf8String (const Port &port)>;

  ~Port () override;

  void set_full_designation_provider (FullDesignationProvider provider)
  {
    full_designation_provider_ = std::move (provider);
  }

  /**
   * @brief Convenience helper for providers that contain a
   * get_full_designation_for_port() method.
   */
  void set_full_designation_provider (const auto * owner)
  {
    full_designation_provider_ = [owner] (const Port &port) {
      return owner->get_full_designation_for_port (port);
    };
  }

  bool is_input () const { return flow_ == PortFlow::Input; }
  bool is_output () const { return flow_ == PortFlow::Output; }

  bool is_midi () const { return type_ == PortType::Midi; }
  bool is_cv () const { return type_ == PortType::CV; }
  bool is_audio () const { return type_ == PortType::Audio; }

  utils::Utf8String get_label () const { return label_; }

  auto get_symbol () const { return sym_; }
  void set_symbol (const utils::Utf8String &sym) { sym_ = sym; }

  // ========================================================================
  // IProcessable Interface
  // ========================================================================

  utils::Utf8String get_node_name () const override
  {
    return get_full_designation ();
  }

  /**
   * @brief Ports have no latency.
   */
  nframes_t get_single_playback_latency () const override { return 0; }

  // ========================================================================

  /**
   * Clears the port buffer.
   */
  virtual void clear_buffer (std::size_t offset, std::size_t nframes) = 0;

  /**
   * Gets a full designation of the port in the format "Track/Port" or
   * "Track/Plugin/Port".
   */
  utils::Utf8String get_full_designation () const
  {
    return full_designation_provider_ (*this);
  }

  bool     has_label () const { return !label_.empty (); }
  PortType type () const { return type_; }
  PortFlow flow () const { return flow_; }

protected:
  Port (utils::Utf8String label, PortType type = {}, PortFlow flow = {});

  friend void
  init_from (Port &obj, const Port &other, utils::ObjectCloneType clone_type);

private:
  static constexpr auto kTypeId = "type"sv;
  static constexpr auto kFlowId = "flow"sv;
  static constexpr auto kLabelId = "label"sv;
  static constexpr auto kSymbolId = "symbol"sv;
  static constexpr auto kPortGroupId = "portGroup"sv;
  friend void           to_json (nlohmann::json &j, const Port &p)
  {
    j[kTypeId] = p.type_;
    j[kFlowId] = p.flow_;
    j[kLabelId] = p.label_;
    j[kSymbolId] = p.sym_;
    j[kPortGroupId] = p.port_group_;
  }
  friend void from_json (const nlohmann::json &j, Port &p)
  {
    j.at (kTypeId).get_to (p.type_);
    j.at (kFlowId).get_to (p.flow_);
    j.at (kLabelId).get_to (p.label_);
    j.at (kSymbolId).get_to (p.sym_);
    j.at (kPortGroupId).get_to (p.port_group_);
  }

private:
  FullDesignationProvider full_designation_provider_ =
    [this] (const Port &port) { return get_label (); };

  /** Data type (e.g. AUDIO). */
  PortType type_{ PortType::Unknown };
  /** Flow (IN/OUT). */
  PortFlow flow_{ PortFlow::Unknown };

  /** Human readable label (may be translated). */
  utils::Utf8String label_;

  /**
   * @brief Symbol (like a variable name, untranslated).
   *
   * Not necessarily unique.
   */
  utils::Utf8String sym_;

  /** Port group this port is part of (only applicable for LV2 plugin ports). */
  std::optional<utils::Utf8String> port_group_;

  BOOST_DESCRIBE_CLASS (
    Port,
    (utils::UuidIdentifiableObject<Port>),
    (),
    (),
    (label_, sym_, port_group_))
};

class RingBufferOwningPortMixin
{
public:
  virtual ~RingBufferOwningPortMixin ();

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

  /**
   * @brief RAII helper for managing ring buffer reader count.
   *
   * Increments the count on construction and decrements on destruction.
   */
  class RingBufferReader
  {
  public:
    explicit RingBufferReader (RingBufferOwningPortMixin &owner)
        : owner_ (owner)
    {
      owner_.num_ring_buffer_readers_++;
    }

    ~RingBufferReader () { owner_.num_ring_buffer_readers_--; }

    // Delete copy and move operations
    Z_DISABLE_COPY_MOVE (RingBufferReader)

  private:
    RingBufferOwningPortMixin &owner_;
  };
};

template <typename PortT> class PortConnectionsCacheMixin
{
  using ElementType =
    std::pair<const PortT *, std::unique_ptr<dsp::PortConnection>>;

public:
  virtual ~PortConnectionsCacheMixin () = default;

  auto &port_sources () const { return port_sources_; }

  void set_port_sources (this auto &self, RangeOf<PortT *> auto source_ports)
    [[clang::blocking]]
  {
    self.port_sources_.clear ();
    for (const auto &source_port : source_ports)
      {
        self.port_sources_.push_back (
          std::make_pair (
            source_port,
            std::make_unique<dsp::PortConnection> (
              source_port->get_uuid (), self.get_uuid (), 1.f, true, true)));
      }
  }

private:
  /**
   * @brief Caches filled when recalculating the graph.
   *
   * Used during processing.
   */
  std::vector<ElementType> port_sources_;
  // std::vector<ElementType> port_destinations_;
};

using PortRegistry = utils::OwningObjectRegistry<PortPtrVariant, Port>;
using PortRegistryRef = std::reference_wrapper<PortRegistry>;
using PortUuidReference = utils::UuidReference<PortRegistry>;

} // namespace zrythm::dsp

void
from_json (const nlohmann::json &j, dsp::PortRegistry &registry);
