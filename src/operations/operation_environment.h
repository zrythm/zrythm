// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>

#include <QUuid>

namespace zrythm::dsp
{
class Port;
class Transport;
}

namespace zrythm::structure::tracks
{
class ChannelSend;
class ChannelSendRouting;
class Track;
class TrackCollection;
class TrackFactory;
class TrackRouting;
class SingletonTracks;
} // namespace zrythm::structure::tracks

namespace zrythm::undo
{
class UndoStack;
}

namespace zrythm::utils
{
class IObjectRegistry;
}

namespace zrythm::operations
{

/**
 * Answers whether a channel send's output may be wired to a
 * destination port in the processing graph.
 *
 * Returns false when the wiring is invalid or would close a signal
 * loop.
 */
using SendConnectivityQuery = std::function<
  bool (const structure::tracks::ChannelSend &, const dsp::Port &)>;

/**
 * The session-scoped services operations act on.
 *
 * One environment exists per open project. It is the only way operations
 * reach project state: models, the undo stack, factories and services hang
 * off it, and every mutation still goes through the undo stack. It performs
 * no locking: operations are dispatched on one thread.
 */
class OperationEnvironment
{
public:
  OperationEnvironment (
    utils::IObjectRegistry                &object_registry,
    undo::UndoStack                       &undo_stack,
    structure::tracks::TrackCollection    &track_collection,
    structure::tracks::TrackFactory       &track_factory,
    structure::tracks::TrackRouting       &track_routing,
    structure::tracks::ChannelSendRouting &channel_send_routing,
    structure::tracks::SingletonTracks    &singleton_tracks,
    dsp::Transport *                       transport = nullptr,
    SendConnectivityQuery                  send_connectivity_query = {});

  utils::IObjectRegistry                &object_registry () const;
  undo::UndoStack                       &undo_stack () const;
  structure::tracks::TrackCollection    &track_collection () const;
  structure::tracks::TrackFactory       &track_factory () const;
  structure::tracks::TrackRouting       &track_routing () const;
  structure::tracks::ChannelSendRouting &channel_send_routing () const;
  structure::tracks::SingletonTracks    &singleton_tracks () const;

  /**
   * The session transport, or null in hosts that run without one.
   */
  dsp::Transport * transport () const;

  /**
   * Returns whether @p send's output may be wired to @p destination in
   * the processing graph.
   *
   * Sessions without a connectivity query accept every wiring; the
   * static eligibility checks in ChannelSendRouting still apply.
   */
  bool can_connect_send_destination (
    const structure::tracks::ChannelSend &send,
    const dsp::Port                      &destination) const;

  /**
   * Returns the track with the given UUID if it is part of the project's
   * track collection, or null.
   */
  structure::tracks::Track * find_track (const QUuid &uuid) const;

private:
  utils::IObjectRegistry                &object_registry_;
  undo::UndoStack                       &undo_stack_;
  structure::tracks::TrackCollection    &track_collection_;
  structure::tracks::TrackFactory       &track_factory_;
  structure::tracks::TrackRouting       &track_routing_;
  structure::tracks::ChannelSendRouting &channel_send_routing_;
  structure::tracks::SingletonTracks    &singleton_tracks_;
  dsp::Transport *                       transport_;
  SendConnectivityQuery                  send_connectivity_query_;
};

} // namespace zrythm::operations
