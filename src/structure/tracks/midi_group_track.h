// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/foldable_track.h"
#include "structure/tracks/group_target_track.h"

namespace zrythm::structure::tracks
{
class MidiGroupTrack final
    : public QObject,
      public FoldableTrack,
      public GroupTargetTrack,
      public utils::InitializableObject<MidiGroupTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (MidiGroupTrack)
  DEFINE_PROCESSABLE_TRACK_QML_PROPERTIES (MidiGroupTrack)
  DEFINE_CHANNEL_TRACK_QML_PROPERTIES (MidiGroupTrack)

  friend class InitializableObject;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (MidiGroupTrack)

public:
  friend void init_from (
    MidiGroupTrack        &obj,
    const MidiGroupTrack  &other,
    utils::ObjectCloneType clone_type);

  void init_loaded (
    PluginRegistry                  &plugin_registry,
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry) override;

  void temporary_virtual_method_hack () const override { }

private:
  friend void to_json (nlohmann::json &j, const MidiGroupTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
    to_json (j, static_cast<const ProcessableTrack &> (track));
    to_json (j, static_cast<const ChannelTrack &> (track));
    to_json (j, static_cast<const GroupTargetTrack &> (track));
    to_json (j, static_cast<const FoldableTrack &> (track));
  }
  friend void from_json (const nlohmann::json &j, MidiGroupTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
    from_json (j, static_cast<ProcessableTrack &> (track));
    from_json (j, static_cast<ChannelTrack &> (track));
    from_json (j, static_cast<GroupTargetTrack &> (track));
    from_json (j, static_cast<FoldableTrack &> (track));
  }

  bool initialize ();
};
}
