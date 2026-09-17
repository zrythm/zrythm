// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track_all.h"

#include <QUndoCommand>

namespace zrythm::commands
{
/**
 * @brief Command that sets (or replaces) a track's folder parent without
 * changing its position.
 */
class SetFolderParentCommand : public QUndoCommand
{
public:
  static constexpr auto CommandId = 1789181890;

  SetFolderParentCommand (
    structure::tracks::TrackCollection &collection,
    structure::tracks::Track::Uuid      child_id,
    structure::tracks::Track::Uuid      parent_id)
      : QUndoCommand (QObject::tr ("Set Folder Parent")),
        collection_ (collection), child_id_ (child_id), parent_id_ (parent_id)
  {
  }

  int id () const override { return CommandId; }

  void undo () override
  {
    if (original_parent_.has_value ())
      {
        collection_.set_folder_parent (child_id_, original_parent_.value ());
      }
    else
      {
        collection_.remove_folder_parent (child_id_);
      }
    notify_child_changed ();
  }

  void redo () override
  {
    original_parent_ = collection_.get_folder_parent (child_id_);
    collection_.set_folder_parent (child_id_, parent_id_);
    notify_child_changed ();
  }

private:
  /**
   * @brief Notifies views that the depths of the child and its
   * descendants changed (depth is derived from the ancestor chain).
   */
  void notify_child_changed ()
  {
    collection_.notify_track_data_changed (
      child_id_, { structure::tracks::TrackCollection::TrackDepthRole });
    for (const auto &desc_id : collection_.get_all_descendants (child_id_))
      {
        collection_.notify_track_data_changed (
          desc_id, { structure::tracks::TrackCollection::TrackDepthRole });
      }
  }

  structure::tracks::TrackCollection           &collection_;
  structure::tracks::Track::Uuid                child_id_;
  structure::tracks::Track::Uuid                parent_id_;
  std::optional<structure::tracks::Track::Uuid> original_parent_;
};

} // namespace zrythm::commands
