// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "structure/tracks/track_collection.h"

#include <QUndoCommand>

namespace zrythm::commands
{
class AddEmptyTrackCommand : public QUndoCommand
{
public:
  static constexpr auto CommandId = 1823189;

  AddEmptyTrackCommand (
    structure::tracks::TrackCollection   &collection,
    structure::tracks::TrackUuidReference track_ref)
      : QUndoCommand (QObject::tr ("Add Track")), collection_ (collection),
        track_ref_ (std::move (track_ref))
  {
  }

  int id () const override { return CommandId; }

  void undo () override { collection_.remove_track (track_ref_.id ()); }
  void redo () override { collection_.add_track (track_ref_); }

private:
  structure::tracks::TrackCollection   &collection_;
  structure::tracks::TrackUuidReference track_ref_;
};

} // namespace zrythm::commands
