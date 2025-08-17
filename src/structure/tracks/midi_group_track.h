// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/foldable_track.h"
#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
class MidiGroupTrack : public Track
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::structure::tracks::FoldableTrackMixin * foldableTrackMixin READ
      foldableTrackMixin CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  MidiGroupTrack (FinalTrackDependencies dependencies);

  friend void init_from (
    MidiGroupTrack        &obj,
    const MidiGroupTrack  &other,
    utils::ObjectCloneType clone_type);

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
  friend void           to_json (nlohmann::json &j, const MidiGroupTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
    j[kFoldableTrackMixinKey] = track.foldable_track_mixin_;
  }
  friend void from_json (const nlohmann::json &j, MidiGroupTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
    j.at (kFoldableTrackMixinKey).get_to (*track.foldable_track_mixin_);
  }

private:
  utils::QObjectUniquePtr<FoldableTrackMixin> foldable_track_mixin_;
};
}
