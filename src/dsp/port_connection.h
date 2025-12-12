// SPDX-FileCopyrightText: © 2021-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/port_fwd.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::dsp
{

/**
 * A connection between two ports.
 */
class PortConnection : public QObject
{
  Q_OBJECT
  QML_ELEMENT

public:
  PortConnection (QObject * parent = nullptr);

  PortConnection (
    const PortUuid &src,
    const PortUuid &dest,
    float           multiplier,
    bool            locked,
    bool            enabled,
    QObject *       parent = nullptr);

  void update (float multiplier, bool locked, bool enabled)
  {
    multiplier_ = multiplier;
    locked_ = locked;
    enabled_ = enabled;
  }

  void set_bipolar (bool bipolar) { bipolar_ = bipolar; }

  void set_channel_mapping (uint8_t source_channel, uint8_t destination_channel)
  {
    source_ch_to_destination_ch_mapping_ =
      std::make_pair (source_channel, destination_channel);
  }
  void unset_channel_mapping ()
  {
    source_ch_to_destination_ch_mapping_.reset ();
  }

private:
  static constexpr std::string_view kSourceIdKey = "srcId";
  static constexpr std::string_view kDestIdKey = "destId";
  static constexpr std::string_view kMultiplierKey = "multiplier";
  static constexpr std::string_view kLockedKey = "locked";
  static constexpr std::string_view kEnabledKey = "enabled";
  static constexpr std::string_view kBipolarKey = "bipolar";
  static constexpr std::string_view kSourceDestMappingKey =
    "sourceChannelToDestinationChannelMapping";
  friend void to_json (nlohmann::json &j, const PortConnection &port_connection)
  {
    j[kSourceIdKey] = port_connection.src_id_;
    j[kDestIdKey] = port_connection.dest_id_;
    j[kMultiplierKey] = port_connection.multiplier_;
    j[kLockedKey] = port_connection.locked_;
    j[kEnabledKey] = port_connection.enabled_;
    j[kBipolarKey] = port_connection.bipolar_;
    j[kSourceDestMappingKey] =
      port_connection.source_ch_to_destination_ch_mapping_;
  }
  friend void
  from_json (const nlohmann::json &j, PortConnection &port_connection)
  {
    j.at (kSourceIdKey).get_to (port_connection.src_id_);
    j.at (kDestIdKey).get_to (port_connection.dest_id_);
    j.at (kMultiplierKey).get_to (port_connection.multiplier_);
    j.at (kLockedKey).get_to (port_connection.locked_);
    j.at (kEnabledKey).get_to (port_connection.enabled_);
    j.at (kBipolarKey).get_to (port_connection.bipolar_);
    j.at (kSourceDestMappingKey)
      .get_to (port_connection.source_ch_to_destination_ch_mapping_);
  }

  friend void init_from (
    PortConnection        &obj,
    const PortConnection  &other,
    utils::ObjectCloneType clone_type);

  friend bool operator== (const PortConnection &lhs, const PortConnection &rhs)
  {
    return lhs.src_id_ == rhs.src_id_ && lhs.dest_id_ == rhs.dest_id_;
  }

public:
  PortUuid src_id_;
  PortUuid dest_id_;

  /**
   * Multiplier to apply, where applicable.
   *
   * Range: 0 to 1.
   * Default: 1.
   */
  float multiplier_ = 1.0f;

  /**
   * Whether the connection can be removed or the multiplier edited by the user.
   *
   * Ignored when connecting things internally and only used to deter the user
   * from breaking necessary connections.
   */
  bool locked_ = false;

  /**
   * Whether the connection is enabled.
   *
   * @note The user can disable port connections only if they are not locked.
   */
  bool enabled_ = true;

  /**
   * @brief Range type for CV->CV connections, where the destination port is
   * used for modulating parameters.
   *
   * If true, modulation will be applied in bipolar fashion, where the midpoint
   * is the base parameter value. Otherwise (and by default), modulation will be
   * added to the base parameter value.
   *
   * @note only used for connections matching the above description.
   */
  bool bipolar_{};

  /**
   * @brief Optional source channel to destination channel mapping.
   *
   * Used for audio ports. If this is nullopt for an audio port connection then
   * the destination port is expected to use a reasonable approach to merge the
   * source channels into the destination channels.
   */
  std::optional<std::pair<uint8_t, uint8_t>> source_ch_to_destination_ch_mapping_;

  BOOST_DESCRIBE_CLASS (
    PortConnection,
    (),
    (src_id_,
     dest_id_,
     multiplier_,
     locked_,
     enabled_,
     bipolar_,
     source_ch_to_destination_ch_mapping_),
    (),
    ())
};

} // namespace zrythm::dsp
