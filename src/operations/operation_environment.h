// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QUuid>

namespace zrythm::dsp
{
class Transport;
}

namespace zrythm::structure::tracks
{
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
    utils::IObjectRegistry             &object_registry,
    undo::UndoStack                    &undo_stack,
    structure::tracks::TrackCollection &track_collection,
    structure::tracks::TrackFactory    &track_factory,
    structure::tracks::TrackRouting    &track_routing,
    structure::tracks::SingletonTracks &singleton_tracks,
    dsp::Transport *                    transport = nullptr);

  utils::IObjectRegistry             &object_registry () const;
  undo::UndoStack                    &undo_stack () const;
  structure::tracks::TrackCollection &track_collection () const;
  structure::tracks::TrackFactory    &track_factory () const;
  structure::tracks::TrackRouting    &track_routing () const;
  structure::tracks::SingletonTracks &singleton_tracks () const;

  /**
   * The session transport, or null in hosts that run without one.
   */
  dsp::Transport * transport () const;

  /**
   * Returns the track with the given UUID if it is part of the project's
   * track collection, or null.
   */
  structure::tracks::Track * find_track (const QUuid &uuid) const;

  /**
   * Returns the next edit sequence number.
   *
   * Every applied Edit operation gets a number one higher than the previous
   * one; numbers are never reused within a session.
   */
  std::uint64_t next_revision ();

private:
  utils::IObjectRegistry             &object_registry_;
  undo::UndoStack                    &undo_stack_;
  structure::tracks::TrackCollection &track_collection_;
  structure::tracks::TrackFactory    &track_factory_;
  structure::tracks::TrackRouting    &track_routing_;
  structure::tracks::SingletonTracks &singleton_tracks_;
  dsp::Transport *                    transport_;
  std::uint64_t                       last_revision_ = 0;
};

} // namespace zrythm::operations
