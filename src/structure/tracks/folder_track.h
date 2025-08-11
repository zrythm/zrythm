// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/channel_track.h"
#include "structure/tracks/foldable_track.h"

namespace zrythm::structure::tracks
{

/**
 * @brief A track that can contain other tracks.
 */
class FolderTrack final
    : public QObject,
      public FoldableTrack,
      // public ChannelTrack,
      public utils::InitializableObject<FolderTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (FolderTrack)

  friend class InitializableObject;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (FolderTrack)

public:
  bool currently_listened () const override
  {
    return is_status (MixerStatus::Listened);
  }

  bool currently_muted () const override
  {
    return is_status (MixerStatus::Muted);
  }

  bool get_implied_soloed () const override
  {
    return is_status (MixerStatus::ImpliedSoloed);
  }

  bool currently_soloed () const override
  {
    return is_status (MixerStatus::Soloed);
  }

  friend void init_from (
    FolderTrack           &obj,
    const FolderTrack     &other,
    utils::ObjectCloneType clone_type);

  void temporary_virtual_method_hack () const override { }

private:
  friend void to_json (nlohmann::json &j, const FolderTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
    to_json (j, static_cast<const FoldableTrack &> (track));
  }

  bool initialize ();
};

} // namespace zrythm::structure::tracks
