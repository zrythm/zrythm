// SPDX-FileCopyrightText: © 2019-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_span.h"
#include "structure/arrangement/marker.h"
#include "structure/tracks/track.h"

#define P_MARKER_TRACK (TRACKLIST->marker_track_)

namespace zrythm::structure::tracks
{
class MarkerTrack final
    : public QObject,
      public Track,
      public arrangement::ArrangerObjectOwner<arrangement::Marker>,
      public ICloneable<MarkerTrack>,
      public utils::InitializableObject<MarkerTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (MarkerTrack)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    MarkerTrack,
    markers,
    arrangement::Marker)

  friend class InitializableObject;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (MarkerTrack)

public:
  using Marker = arrangement::Marker;

public:
  void
  init_loaded (PluginRegistry &plugin_registry, PortRegistry &port_registry)
    override;

  /**
   * Removes all objects from the marker track.
   *
   * Mainly used in testing.
   */
  void clear_objects () override;

  auto get_marker_at (size_t index) const
  {
    return get_children_view ()[index];
  }

  bool validate_marker_name (const utils::Utf8String &name)
  {
    /* valid if no other marker with the same name exists*/
    return !std::ranges::contains (
      get_children_view (), name,
      [] (const auto &marker) { return marker->get_name (); });
  }

  /**
   * Returns the start marker.
   */
  Marker * get_start_marker () const;

  /**
   * Returns the end marker.
   */
  Marker * get_end_marker () const;

  void init_after_cloning (const MarkerTrack &other, ObjectCloneType clone_type)
    override;

  void
  append_ports (std::vector<Port *> &ports, bool include_plugins) const final;

  Location get_location (const Marker &) const override
  {
    return { .track_id_ = get_uuid () };
  }

  std::string get_field_name_for_serialization (const Marker *) const override
  {
    return "markers";
  }

private:
  friend void to_json (nlohmann::json &j, const MarkerTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
    to_json (j, static_cast<const ArrangerObjectOwner &> (track));
  }
  friend void from_json (const nlohmann::json &j, MarkerTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
    from_json (j, static_cast<ArrangerObjectOwner &> (track));
  }

private:
  bool initialize ();
  void set_playback_caches () override;
};

}
