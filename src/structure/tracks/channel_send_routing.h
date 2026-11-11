// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>

#include "dsp/audio_port.h"
#include "dsp/port.h"
#include "structure/tracks/channel_send.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_collection.h"

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::structure::tracks
{

/**
 * @brief Query and validation service for channel send destinations.
 *
 * Answers which destinations a channel's send may point at: tracks that
 * accept the channel's signal type, and sidechain input ports of plugins
 * on other tracks.
 *
 * The service owns no data; it reads the track collection it is
 * constructed with. It performs static eligibility checks only: whether
 * a proposed send would close a signal loop is answered by the
 * processing graph (ProjectGraphBuilder::can_ports_be_connected()), the
 * single source of truth for connectivity.
 */
class ChannelSendRouting : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")
public:
  ChannelSendRouting (TrackCollection &collection, QObject * parent = nullptr);
  Q_DISABLE_COPY_MOVE (ChannelSendRouting)
  ~ChannelSendRouting () override;

  // ============================================================================
  // QML Interface
  // ============================================================================

  /**
   * @brief Lists the statically valid send destinations for @p source.
   *
   * Each entry is a map with a "key" (unique id string used to match the
   * current destination), a "display" label, and either a "track" (Track*)
   * or a "port" (AudioPort* representing a plugin's sidechain input).
   *
   * Entries that would close a signal loop are not filtered out; the
   * routing is validated against the processing graph when the user
   * picks a target.
   */
  Q_INVOKABLE QVariantList sendTargetsFor (Track * source) const;

  /**
   * @brief Returns the key of @p send's current destination (see
   * sendTargetsFor()), or an empty string when the send has none.
   */
  Q_INVOKABLE QString sendDestinationKey (const ChannelSend * send) const;

  // ============================================================================

  /**
   * @brief Returns whether @p source's channel may send to @p destination
   * by signal type and track kind.
   */
  bool can_send_to_track (const Track &source, const Track &destination) const;

  /**
   * @brief Returns whether @p source's channel may send to the sidechain
   * input @p destination by signal type and ownership.
   */
  bool can_send_to_port (const Track &source, const dsp::AudioPort &destination)
    const;

  /**
   * @brief Returns the input port a send into @p destination feeds.
   *
   * This is the same port direct track routing connects to.
   */
  std::optional<dsp::PortUuidReference>
  destination_port_for_track (const Track &destination) const;

  /**
   * @brief The track a send destination port feeds signal into.
   *
   * plugin is null when the destination is a track's own input port.
   */
  struct SendDestinationOwner
  {
    Track *           track{};
    plugins::Plugin * plugin{};
  };

  /**
   * @brief Finds the track (and plugin, if any) that owns @p port_id as
   * an input.
   */
  std::optional<SendDestinationOwner>
  resolve_port_owner (const dsp::PortUuid &port_id) const;

  /**
   * @brief Resolves the owner of @p send's destination port.
   */
  std::optional<SendDestinationOwner>
  resolve_send_destination (const ChannelSend &send) const;

private:
  TrackCollection &collection_;
};

} // namespace zrythm::structure::tracks
