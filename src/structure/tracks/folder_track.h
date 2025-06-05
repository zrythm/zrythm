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
      public ICloneable<FolderTrack>,
      public utils::InitializableObject<FolderTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (FolderTrack)

  friend class InitializableObject;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (FolderTrack)

public:
  bool get_listened () const override
  {
    return is_status (MixerStatus::Listened);
  }

  bool get_muted () const override { return is_status (MixerStatus::Muted); }

  bool get_implied_soloed () const override
  {
    return is_status (MixerStatus::ImpliedSoloed);
  }

  bool get_soloed () const override { return is_status (MixerStatus::Soloed); }
  void
  init_loaded (PluginRegistry &plugin_registry, PortRegistry &port_registry)
    override;

  void init_after_cloning (const FolderTrack &other, ObjectCloneType clone_type)
    override;
  void
  append_ports (std::vector<Port *> &ports, bool include_plugins) const final;

private:
  friend void to_json (nlohmann::json &j, const FolderTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
    to_json (j, static_cast<const FoldableTrack &> (track));
  }

  bool initialize ();
};

} // namespace zrythm::structure::tracks
