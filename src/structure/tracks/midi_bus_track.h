// SPDX-FileCopyrightText: © 2018-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/channel_track.h"

namespace zrythm::structure::tracks
{
/**
 * @class MidiBusTrack
 * @brief A track that processes MIDI data.
 *
 * The MidiBusTrack class is a concrete implementation of the ProcessableTrack,
 * ChannelTrack, and AutomatableTrack interfaces. It represents a track that
 * processes MIDI data.
 */
class MidiBusTrack final
    : public QObject,
      public ChannelTrack,
      public ICloneable<MidiBusTrack>,
      public utils::InitializableObject<MidiBusTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (MidiBusTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (MidiBusTrack)
  DEFINE_CHANNEL_TRACK_QML_PROPERTIES (MidiBusTrack)

  friend class InitializableObject;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (MidiBusTrack)

public:
  void
  init_after_cloning (const MidiBusTrack &other, ObjectCloneType clone_type)
    override;

  void
  init_loaded (PluginRegistry &plugin_registry, PortRegistry &port_registry)
    override;

  bool validate () const override;

  void
  append_ports (std::vector<Port *> &ports, bool include_plugins) const final;

private:
  friend void to_json (nlohmann::json &j, const MidiBusTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
    to_json (j, static_cast<const ProcessableTrack &> (track));
    to_json (j, static_cast<const AutomatableTrack &> (track));
    to_json (j, static_cast<const ChannelTrack &> (track));
  }
  friend void from_json (const nlohmann::json &j, MidiBusTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
    from_json (j, static_cast<ProcessableTrack &> (track));
    from_json (j, static_cast<AutomatableTrack &> (track));
    from_json (j, static_cast<ChannelTrack &> (track));
  }

  bool initialize ();
};
}
