// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/dsp/port.h"
#include "utils/icloneable.h"

#define MIDI_MAPPINGS (PROJECT->midi_mappings_)

namespace zrythm::engine::session
{
/**
 * A mapping from a MIDI CC value to a destination ControlPort.
 */
class MidiMapping final : public QObject
{
  Q_OBJECT
  QML_ELEMENT

public:
  using PortIdentifier = zrythm::dsp::PortIdentifier;

  MidiMapping (QObject * parent = nullptr);

public:
  friend void init_from (
    MidiMapping           &obj,
    const MidiMapping     &other,
    utils::ObjectCloneType clone_type);

  void set_enabled (bool enabled) { enabled_.store (enabled); }

  void apply (std::array<midi_byte_t, 3> buf);

private:
  static constexpr auto kKeyKey = "key"sv;
  static constexpr auto kDeviceIdKey = "deviceIdentifier"sv;
  static constexpr auto kDestIdKey = "destId"sv;
  static constexpr auto kEnabledKey = "enabled"sv;
  friend void           to_json (nlohmann::json &j, const MidiMapping &mapping)
  {
    j = nlohmann::json{
      { kKeyKey,      mapping.key_             },
      { kDeviceIdKey, mapping.device_id_       },
      { kDestIdKey,   mapping.dest_id_         },
      { kEnabledKey,  mapping.enabled_.load () },
    };
  }
  friend void from_json (const nlohmann::json &j, MidiMapping &mapping)
  {
    j.at (kKeyKey).get_to (mapping.key_);
    j.at (kDeviceIdKey).get_to (mapping.device_id_);
    j.at (kDestIdKey).get_to (mapping.dest_id_);
    mapping.enabled_.store (j.at (kEnabledKey).get<bool> ());
  }

public:
  /** Raw MIDI signal. */
  std::array<midi_byte_t, 3> key_ = {};

  /**
   * @brief The device that this connection will be mapped for.
   *
   * If nullopt, all devices will be considered.
   */
  std::optional<utils::Utf8String> device_id_;

  /** Destination. */
  std::optional<PortIdentifier::PortUuid> dest_id_;

  /**
   * Destination pointer, for convenience.
   *
   * @note This pointer is not owned by this instance.
   */
  Port * dest_ = nullptr;

  /** Whether this binding is enabled. */
  /* TODO: check if really should be atomic */
  std::atomic<bool> enabled_ = false;
};

/**
 * All MIDI mappings in Zrythm.
 */
class MidiMappings final
{
public:
  void init_loaded ();

  /**
   * Binds the CC represented by the given raw buffer (must be size 3) to the
   * given Port.
   *
   * @param idx Index to insert at.
   * @param buf The buffer used for matching at [0] and [1].
   * @param device_id Device ID, if custom mapping.
   */
  void bind_at (
    std::array<midi_byte_t, 3>       buf,
    std::optional<utils::Utf8String> device_id,
    Port                            &dest_port,
    int                              idx,
    bool                             fire_events);

  /**
   * Unbinds the given binding.
   *
   * @note This must be called inside a port operation lock, such as inside an
   * undoable action.
   */
  void unbind (int idx, bool fire_events);

  void bind_device (
    std::array<midi_byte_t, 3>       buf,
    std::optional<utils::Utf8String> device_id,
    Port                            &dest_port,
    bool                             fire_events)
  {
    bind_at (
      buf, device_id, dest_port, static_cast<int> (mappings_.size ()),
      fire_events);
  }

  void
  bind_track (std::array<midi_byte_t, 3> buf, Port &dest_port, bool fire_events)
  {
    bind_at (
      buf, std::nullopt, dest_port, static_cast<int> (mappings_.size ()),
      fire_events);
  }

  int get_mapping_index (const MidiMapping &mapping) const;

  /**
   * Applies the events to the appropriate mapping.
   *
   * This is used only for TrackProcessor.cc_mappings.
   *
   * @note Must only be called while transport is recording.
   */
  void apply_from_cc_events (dsp::MidiEventVector &events);

  /**
   * Applies the given buffer to the matching ports.
   */
  void apply (const midi_byte_t * buf);

  /**
   * Get MIDI mappings for the given port.
   *
   * @param arr Optional array to fill with the mappings.
   *
   * @return The number of results.
   */
  int
  get_for_port (const Port &dest_port, std::vector<MidiMapping *> * arr) const;

  friend void init_from (
    MidiMappings          &obj,
    const MidiMappings    &other,
    utils::ObjectCloneType clone_type)

  {
    utils::clone_unique_ptr_container (obj.mappings_, other.mappings_);
  }

private:
  static constexpr auto kMappingsKey = "mappings"sv;
  friend void to_json (nlohmann::json &j, const MidiMappings &mappings)
  {
    j[kMappingsKey] = mappings.mappings_;
  }
  friend void from_json (const nlohmann::json &j, MidiMappings &mappings);

public:
  std::vector<std::unique_ptr<MidiMapping>> mappings_;
};

}
