// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/foldable_track.h"
#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{

/**
 * @brief A track that can contain other tracks.
 */
class FolderTrack : public Track
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::structure::tracks::FoldableTrackMixin * foldableTrackMixin READ
      foldableTrackMixin CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  FolderTrack (FinalTrackDependencies dependencies);

  // ========================================================================
  // QML Interface
  // ========================================================================

  FoldableTrackMixin * foldableTrackMixin () const
  {
    return foldable_track_mixin_.get ();
  }

  // ========================================================================

private:
  static constexpr auto kFoldableTrackMixinKey = "foldableTrackMixin"sv;
  friend void           to_json (nlohmann::json &j, const FolderTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
    j.at (kFoldableTrackMixinKey).get_to (*track.foldable_track_mixin_);
  }
  friend void from_json (const nlohmann::json &j, FolderTrack &track);

  friend void init_from (
    FolderTrack           &obj,
    const FolderTrack     &other,
    utils::ObjectCloneType clone_type);

private:
  utils::QObjectUniquePtr<FoldableTrackMixin> foldable_track_mixin_;
};

} // namespace zrythm::structure::tracks
