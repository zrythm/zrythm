// SPDX-FileCopyrightText: © 2018-2019, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/folder_track.h"
#include "structure/tracks/track_all.h"

namespace zrythm::structure::tracks
{
FolderTrack::FolderTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::Folder,
        PortType::Unknown,
        PortType::Unknown,
        {},
        dependencies.to_base_dependencies ()),
      foldable_track_mixin_ (
        utils::make_qobject_unique<
          FoldableTrackMixin> (dependencies.track_registry_, this))
{
  color_ = Color (QColor ("#865E3C"));
  icon_name_ = u8"fluentui-folder-regular";
}

void
from_json (const nlohmann::json &j, FolderTrack &track)
{
  from_json (j, static_cast<Track &> (track));
  j.at (FolderTrack::kFoldableTrackMixinKey)
    .get_to (*track.foldable_track_mixin_);
}

void
init_from (
  FolderTrack           &obj,
  const FolderTrack     &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
}
}
