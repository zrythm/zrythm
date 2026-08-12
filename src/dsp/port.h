// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <optional>
#include <stdexcept>
#include <string_view>

#include "dsp/graph_node.h"
#include "dsp/port_fwd.h"
#include "utils/typed_uuid_reference.h"
#include "utils/utf8_string.h"
#include "utils/uuid_identifiable_object.h"

#include <QtQmlIntegration/qqmlintegration.h>

#include <fmt/format.h>

using namespace std::string_view_literals;

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
class Port : public utils::UuidIdentifiableObject<Port>, public dsp::graph::IProcessable
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")
  Q_PROPERTY (bool detached READ detached NOTIFY detachedChanged)
  Q_PROPERTY (QString label READ label NOTIFY labelChanged)

  Q_DISABLE_COPY_MOVE (Port)
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

  /**
   * @brief Sets the label, emitting @ref labelChanged if it differs from the
   * current one.
   */
  void set_label (utils::Utf8String new_label);

  auto get_symbol () const { return sym_; }
  void set_symbol (const utils::Utf8String &sym) { sym_ = sym; }

  // ========================================================================
  // IProcessable Interface
  // ========================================================================

  utils::Utf8String get_node_name () const override
  {
    return get_full_designation ();
  }

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

  /**
   * @brief Sets the detached state.
   *
   * Owning processors derive their attached port subset from this flag on
   * read, so this is the single source of truth for the detached state. The
   * signal exists for property bindings and other observers.
   *
   * Must only be called on the main thread with the engine paused (e.g.
   * during bus configuration reconciliation), never concurrently with
   * processing.
   */
  void set_detached (bool new_detached) [[clang::blocking]];

  // ========================================================================
  // QML Interface
  // ========================================================================

  /**
   * @brief Whether this port is detached from processing.
   *
   * Detached ports stay in their owner's port list but are excluded from the
   * graph and from processing, while keeping their identity (UUID),
   * connections and serialization. Re-attaching the same port object revives
   * its connections.
   *
   * Can only be changed via @ref set_detached() on the main thread with the
   * engine paused, never concurrently with processing.
   */
  bool          detached () const { return detached_; }
  Q_SIGNAL void detachedChanged (bool detached);

  QString       label () const { return label_.to_qstring (); }
  Q_SIGNAL void labelChanged (const QString &label);

protected:
  Port (
    utils::Utf8String label,
    PortType          type = {},
    PortFlow          flow = {},
    QObject *         parent = nullptr);

  friend void
  init_from (Port &obj, const Port &other, utils::ObjectCloneType clone_type);

private:
  static constexpr auto kFlowId = "flow"sv;
  static constexpr auto kLabelId = "label"sv;
  static constexpr auto kSymbolId = "symbol"sv;
  static constexpr auto kPortGroupId = "portGroup"sv;
  static constexpr auto kDetachedId = "detached"sv;
  friend void           to_json (nlohmann::json &j, const Port &p);
  friend void           from_json (const nlohmann::json &j, Port &p);

private:
  FullDesignationProvider full_designation_provider_ =
    [this] (const Port &port) { return get_label (); };

  /** Data type (e.g. AUDIO). */
  PortType type_{};
  /** Flow (IN/OUT). */
  PortFlow flow_{};

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

  /** Whether this port is detached from processing (see detached()). */
  bool detached_{};

  BOOST_DESCRIBE_CLASS (
    Port,
    (utils::UuidIdentifiableObject<Port>),
    (),
    (),
    (label_, sym_, port_group_, detached_))
};

template <typename PortT> class PortConnectionsCacheMixin
{
  using ElementType = std::pair<const PortT *, graph::ConnectionConfig>;

public:
  virtual ~PortConnectionsCacheMixin () = default;

  auto &port_sources () const { return port_sources_; }

  /**
   * @brief Rebuilds the cache of upstream sources and their connection
   * configurations from the given (source port, configuration) pairs.
   *
   * A nullopt configuration means the edge carries no user connection and
   * gets the default unity/derived configuration.
   */
  void set_port_sources (
    this auto                                          &self,
    utils::RangeOf<graph::PortSourceConfig<PortT>> auto source_ports)
    [[clang::blocking]]
  {
    self.port_sources_.clear ();
    if (self.flow () != PortFlow::Input)
      {
        throw std::runtime_error (
          fmt::format (
            "Destination port '{}' must be an input port (is {})",
            self.get_full_designation (),
            self.flow () == PortFlow::Output ? "Output" : "Unknown"));
      }
    for (const auto &[source_port, config] : source_ports)
      {
        if (source_port->flow () != PortFlow::Output)
          {
            throw std::runtime_error (
              fmt::format (
                "Source port '{}' must be an output port (is {}), destination '{}'",
                source_port->get_full_designation (),
                source_port->flow () == PortFlow::Input ? "Input" : "Unknown",
                self.get_full_designation ()));
          }
        self.port_sources_.push_back (
          std::make_pair (
            source_port, config.value_or (graph::ConnectionConfig{})));
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

using PortUuidReference = utils::TypedUuidReference<Port>;

} // namespace zrythm::dsp
