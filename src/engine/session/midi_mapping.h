// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/parameter.h"
#include "utils/icloneable.h"

#define MIDI_MAPPINGS (PROJECT->midi_mappings_)

namespace zrythm::engine::session
{
/**
 * A mapping from a MIDI CC value to a destination parameter.
 */
class MidiMapping : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  MidiMapping (
    dsp::ProcessorParameterRegistry &param_registry,
    QObject *                        parent = nullptr);
  Z_DISABLE_COPY_MOVE (MidiMapping)

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
    j[kKeyKey] = mapping.key_;
    j[kDeviceIdKey] = mapping.device_id_;
    if (mapping.dest_id_)
      {
        j[kDestIdKey] = *mapping.dest_id_;
      }
    j[kEnabledKey] = mapping.enabled_.load ();
  }
  friend void from_json (const nlohmann::json &j, MidiMapping &mapping)
  {
    j.at (kKeyKey).get_to (mapping.key_);
    j.at (kDeviceIdKey).get_to (mapping.device_id_);
    if (j.contains (kDestIdKey))
      {
        mapping.dest_id_.emplace (mapping.param_registry_);
        j.at (kDestIdKey).get_to (*mapping.dest_id_);
      }
    mapping.enabled_.store (j.at (kEnabledKey).get<bool> ());
  }

public:
  dsp::ProcessorParameterRegistry &param_registry_;

  /** Raw MIDI signal. */
  std::array<midi_byte_t, 3> key_ = {};

  /**
   * @brief The device that this connection will be mapped for.
   *
   * If nullopt, all devices will be considered.
   */
  std::optional<utils::Utf8String> device_id_;

  /** Destination. */
  std::optional<dsp::ProcessorParameterUuidReference> dest_id_;

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
  MidiMappings (dsp::ProcessorParameterRegistry &param_registry);

  /**
   * Binds the CC represented by the given raw buffer (must be size 3) to the
   * given Port.
   *
   * @param idx Index to insert at.
   * @param buf The buffer used for matching at [0] and [1].
   * @param device_id Device ID, if custom mapping.
   */
  void bind_at (
    std::array<midi_byte_t, 3>           buf,
    std::optional<utils::Utf8String>     device_id,
    dsp::ProcessorParameterUuidReference dest_port,
    int                                  idx,
    bool                                 fire_events);

  /**
   * Unbinds the given binding.
   *
   * @note This must be called inside a port operation lock, such as inside an
   * undoable action.
   */
  void unbind (int idx, bool fire_events);

  void bind_device (
    std::array<midi_byte_t, 3>           buf,
    std::optional<utils::Utf8String>     device_id,
    dsp::ProcessorParameterUuidReference dest_port,
    bool                                 fire_events)
  {
    bind_at (
      buf, device_id, dest_port, static_cast<int> (mappings_.size ()),
      fire_events);
  }

  void bind_track (
    std::array<midi_byte_t, 3>           buf,
    dsp::ProcessorParameterUuidReference dest_port,
    bool                                 fire_events)
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
  void apply_from_cc_events (const dsp::MidiEventVector &events);

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
  int get_for_port (
    const dsp::ProcessorParameter &dest_port,
    std::vector<MidiMapping *> *   arr) const;

  friend void init_from (
    MidiMappings          &obj,
    const MidiMappings    &other,
    utils::ObjectCloneType clone_type)
  {
    // TODO
    // utils::clone_unique_ptr_container (obj.mappings_, other.mappings_);
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

private:
  dsp::ProcessorParameterRegistry &param_registry_;
};

}
