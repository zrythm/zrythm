// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/track.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"
#include "structure/tracks/tracklist.h"
#include "undo/undo_stack.h"
#include "utils/object_registry.h"

#include "operations/operation_environment.h"

namespace zrythm::operations
{

OperationEnvironment::OperationEnvironment (
  utils::IObjectRegistry             &object_registry,
  undo::UndoStack                    &undo_stack,
  structure::tracks::TrackCollection &track_collection,
  structure::tracks::TrackFactory    &track_factory,
  structure::tracks::TrackRouting    &track_routing,
  structure::tracks::SingletonTracks &singleton_tracks,
  dsp::Transport *                    transport)
    : object_registry_ (object_registry), undo_stack_ (undo_stack),
      track_collection_ (track_collection), track_factory_ (track_factory),
      track_routing_ (track_routing), singleton_tracks_ (singleton_tracks),
      transport_ (transport)
{
}

utils::IObjectRegistry &
OperationEnvironment::object_registry () const
{
  return object_registry_;
}

undo::UndoStack &
OperationEnvironment::undo_stack () const
{
  return undo_stack_;
}

structure::tracks::TrackCollection &
OperationEnvironment::track_collection () const
{
  return track_collection_;
}

structure::tracks::TrackFactory &
OperationEnvironment::track_factory () const
{
  return track_factory_;
}

structure::tracks::TrackRouting &
OperationEnvironment::track_routing () const
{
  return track_routing_;
}

structure::tracks::SingletonTracks &
OperationEnvironment::singleton_tracks () const
{
  return singleton_tracks_;
}

dsp::Transport *
OperationEnvironment::transport () const
{
  return transport_;
}

structure::tracks::Track *
OperationEnvironment::find_track (const QUuid &uuid) const
{
  const auto track_id = structure::tracks::Track::Uuid (uuid);
  if (!track_collection_.contains (track_id))
    {
      return nullptr;
    }
  return track_collection_.track_ref_at_id (track_id).get ();
}

std::uint64_t
OperationEnvironment::next_revision ()
{
  return ++last_revision_;
}

} // namespace zrythm::operations
