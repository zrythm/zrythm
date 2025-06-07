// SPDX-FileCopyrightText: © 2021-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include "dsp/port_identifier.h"

#include <QtQmlIntegration>

namespace zrythm::dsp
{

/**
 * A connection between two ports.
 */
class PortConnection final : public QObject
{
  Q_OBJECT
  QML_ELEMENT

public:
  using PortUuid = dsp::PortIdentifier::PortUuid;

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

private:
  static constexpr std::string_view kSourceIdKey = "srcId";
  static constexpr std::string_view kDestIdKey = "destId";
  static constexpr std::string_view kMultiplierKey = "multiplier";
  static constexpr std::string_view kLockedKey = "locked";
  static constexpr std::string_view kEnabledKey = "enabled";
  // note: this is only needed for CV ports
  static constexpr std::string_view kBaseValueKey = "baseValue";
  friend void to_json (nlohmann::json &j, const PortConnection &port_connection)
  {
    j[kSourceIdKey] = port_connection.src_id_;
    j[kDestIdKey] = port_connection.dest_id_;
    j[kMultiplierKey] = port_connection.multiplier_;
    j[kLockedKey] = port_connection.locked_;
    j[kEnabledKey] = port_connection.enabled_;
    j[kBaseValueKey] = port_connection.base_value_;
  }
  friend void
  from_json (const nlohmann::json &j, PortConnection &port_connection)
  {
    j.at (kSourceIdKey).get_to (port_connection.src_id_);
    j.at (kDestIdKey).get_to (port_connection.dest_id_);
    j.at (kMultiplierKey).get_to (port_connection.multiplier_);
    j.at (kLockedKey).get_to (port_connection.locked_);
    j.at (kEnabledKey).get_to (port_connection.enabled_);
    j.at (kBaseValueKey).get_to (port_connection.base_value_);
  }

  friend void init_from (
    PortConnection        &obj,
    const PortConnection  &other,
    utils::ObjectCloneType clone_type);

  friend bool operator== (const PortConnection &lhs, const PortConnection &rhs)
  {
    return lhs.src_id_ == rhs.src_id_ && lhs.dest_id_ == rhs.dest_id_;
    //  && utils::math::floats_equal (lhs.multiplier_, rhs.multiplier_)
    //  && lhs.locked_ == rhs.locked_ && lhs.enabled_ == rhs.enabled_
    //  && utils::math::floats_equal (lhs.base_value_, rhs.base_value_);
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

  /** Used for CV -> control port connections. */
  float base_value_ = 0.0f;

  BOOST_DESCRIBE_CLASS (
    PortConnection,
    (),
    (src_id_, dest_id_, multiplier_, locked_, enabled_),
    (),
    ())
};

} // namespace zrythm::dsp
