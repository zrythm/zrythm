// SPDX-FileCopyrightText: © 2019-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/marker.h"
#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
class MarkerTrack
    : public Track,
      public arrangement::ArrangerObjectOwner<arrangement::Marker>
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    MarkerTrack,
    markers,
    arrangement::Marker)

public:
  using Marker = arrangement::Marker;

  MarkerTrack (FinalTrackDependencies dependencies);

  auto get_marker_at (size_t index) const
  {
    return get_children_view ()[index];
  }

  bool validate_marker_name (const utils::Utf8String &name)
  {
    /* valid if no other marker with the same name exists*/
    return !std::ranges::contains (
      get_children_view (), name,
      [] (const auto &marker) { return marker->name ()->get_name (); });
  }

  /**
   * Returns the start marker.
   */
  Marker * get_start_marker () const;

  /**
   * Returns the end marker.
   */
  Marker * get_end_marker () const;

  friend void init_from (
    MarkerTrack           &obj,
    const MarkerTrack     &other,
    utils::ObjectCloneType clone_type);

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
};

}
