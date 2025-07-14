// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/automatable_track.h"
#include "structure/tracks/foldable_track.h"
#include "structure/tracks/group_target_track.h"

namespace zrythm::structure::tracks
{
/**
 * An audio group track that can be folded and is a target for other tracks.
 * It is also an automatable track, meaning it can have automation data.
 */
class AudioGroupTrack final
    : public QObject,
      public FoldableTrack,
      public GroupTargetTrack,
      public utils::InitializableObject<AudioGroupTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (AudioGroupTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (AudioGroupTrack)

  friend class InitializableObject;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (AudioGroupTrack)

public:
  friend void init_from (
    AudioGroupTrack       &obj,
    const AudioGroupTrack &other,
    utils::ObjectCloneType clone_type);

  void init_loaded (
    PluginRegistry                  &plugin_registry,
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry) override;

  void temporary_virtual_method_hack () const override { }

private:
  friend void to_json (nlohmann::json &j, const AudioGroupTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
    to_json (j, static_cast<const ProcessableTrack &> (track));
    to_json (j, static_cast<const AutomatableTrack &> (track));
    to_json (j, static_cast<const ChannelTrack &> (track));
    to_json (j, static_cast<const GroupTargetTrack &> (track));
    to_json (j, static_cast<const FoldableTrack &> (track));
  }
  friend void from_json (const nlohmann::json &j, AudioGroupTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
    from_json (j, static_cast<ProcessableTrack &> (track));
    from_json (j, static_cast<AutomatableTrack &> (track));
    from_json (j, static_cast<ChannelTrack &> (track));
    from_json (j, static_cast<GroupTargetTrack &> (track));
    from_json (j, static_cast<FoldableTrack &> (track));
  }

  bool initialize ();
};
}
