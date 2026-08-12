// SPDX-FileCopyrightText: © 2021-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_bus_channel_routing.h"
#include "dsp/port_fwd.h"

#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::project
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
    const dsp::PortUuid &src,
    const dsp::PortUuid &dest,
    float                multiplier,
    bool                 locked,
    bool                 enabled,
    QObject *            parent = nullptr);

  void update (float multiplier, bool locked, bool enabled)
  {
    multiplier_ = multiplier;
    locked_ = locked;
    enabled_ = enabled;
  }

  void set_bipolar (bool bipolar) { bipolar_ = bipolar; }

  /**
   * @brief Overrides the channel routing with an explicit set of routes.
   *
   * @throw std::invalid_argument if two routes describe the same source and
   * destination channel pair.
   */
  void
  set_audio_bus_channel_routing (std::vector<dsp::AudioBusChannelRoute> routes)
  {
    audio_bus_channel_routing_ =
      dsp::AudioBusChannelRouting{ std::move (routes) };
  }

  /** Reverts to routing derived from the two ports' speaker arrangements. */
  void reset_audio_bus_channel_routing ()
  {
    audio_bus_channel_routing_.reset_to_derived ();
  }

private:
  static constexpr std::string_view kSourceIdKey = "srcId";
  static constexpr std::string_view kDestIdKey = "destId";
  static constexpr std::string_view kMultiplierKey = "multiplier";
  static constexpr std::string_view kLockedKey = "locked";
  static constexpr std::string_view kEnabledKey = "enabled";
  static constexpr std::string_view kBipolarKey = "bipolar";
  static constexpr std::string_view kAudioBusChannelRoutingKey =
    "audioBusChannelRouting";
  friend void
  to_json (nlohmann::json &j, const PortConnection &port_connection);
  friend void
  from_json (const nlohmann::json &j, PortConnection &port_connection);

  friend void init_from (
    PortConnection        &obj,
    const PortConnection  &other,
    utils::ObjectCloneType clone_type);

  friend bool operator== (const PortConnection &lhs, const PortConnection &rhs)
  {
    return lhs.src_id_ == rhs.src_id_ && lhs.dest_id_ == rhs.dest_id_;
  }

public:
  dsp::PortUuid src_id_;
  dsp::PortUuid dest_id_;

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
   * @brief How the source port's channels feed the destination port's
   * channels.
   *
   * Used for audio ports. Derived from the two ports' speaker arrangements
   * unless the user has edited this connection's channel matrix.
   */
  dsp::AudioBusChannelRouting audio_bus_channel_routing_;

  BOOST_DESCRIBE_CLASS (
    PortConnection,
    (),
    (src_id_,
     dest_id_,
     multiplier_,
     locked_,
     enabled_,
     bipolar_,
     audio_bus_channel_routing_),
    (),
    ())
};

} // namespace zrythm::structure::project
