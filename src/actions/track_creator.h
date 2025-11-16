// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "commands/add_track_command.h"
#include "structure/tracks/track_all.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"
#include "structure/tracks/tracklist.h"
#include "undo/undo_stack.h"

namespace zrythm::actions
{
class TrackCreator : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("One instance per project")

public:
  explicit TrackCreator (
    undo::UndoStack                    &undo_stack,
    structure::tracks::TrackFactory    &track_factory,
    structure::tracks::TrackCollection &track_collection,
    structure::tracks::TrackRouting    &track_routing,
    structure::tracks::SingletonTracks &singleton_tracks,
    QObject *                           parent = nullptr)
      : QObject (parent), track_factory_ (track_factory),
        track_collection_ (track_collection), track_routing_ (track_routing),
        singleton_tracks_ (singleton_tracks), undo_stack_ (undo_stack)
  {
  }

  Q_INVOKABLE QVariant
  addEmptyTrackFromType (structure::tracks::Track::Type trackType) const;

private:
  /**
   * Returns a unique name for a new track based on the given name.
   */
  utils::Utf8String get_unique_name_for_track (
    const structure::tracks::Track::Uuid &track_to_skip,
    const utils::Utf8String              &name) const;

private:
  structure::tracks::TrackFactory    &track_factory_;
  structure::tracks::TrackCollection &track_collection_;
  structure::tracks::TrackRouting    &track_routing_;
  structure::tracks::SingletonTracks &singleton_tracks_;
  undo::UndoStack                    &undo_stack_;
};
} // namespace zrythm::actions
