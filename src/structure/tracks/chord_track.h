// SPDX-FileCopyrightText: © 2018-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/chord_region.h"
#include "structure/arrangement/scale_object.h"
#include "structure/tracks/channel_track.h"
#include "structure/tracks/recordable_track.h"

#define P_CHORD_TRACK (TRACKLIST->chord_track_)

namespace zrythm::structure::tracks
{
/**
 * The ChordTrack class is responsible for managing the chord and scale
 * information in the project. It inherits from the RecordableTrack and
 * ChannelTrack classes, which provide basic track functionality.
 *
 * The ChordTrack class provides methods for inserting, adding, and removing
 * chord regions and scales from the track. It also provides methods for
 * retrieving the current chord and scale at a given position in the timeline.
 *
 * The chord regions and scales are stored in sorted vectors, and the class
 * provides methods for clearing all objects from the track, which is mainly
 * used for testing.
 *
 * This is an abstract list model for ScaleObject's. For chord regions, @see
 * RegionOwner.
 */
class ChordTrack final
    : public QObject,
      public RecordableTrack,
      public ChannelTrack,
      public arrangement::ArrangerObjectOwner<arrangement::ChordRegion>,
      public arrangement::ArrangerObjectOwner<arrangement::ScaleObject>,
      public ICloneable<ChordTrack>,
      public utils::InitializableObject<ChordTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (ChordTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (ChordTrack)
  DEFINE_CHANNEL_TRACK_QML_PROPERTIES (ChordTrack)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    ChordTrack,
    regions,
    arrangement::ChordRegion)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    ChordTrack,
    scaleObjects,
    arrangement::ScaleObject)
  DEFINE_RECORDABLE_TRACK_QML_PROPERTIES (ChordTrack)

  friend class InitializableObject;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (ChordTrack)

public:
  using ScaleObject = arrangement::ScaleObject;
  using ChordRegion = arrangement::ChordRegion;
  using ChordObject = arrangement::ChordObject;
  using ScaleObjectPtr = ScaleObject *;

  void
  init_loaded (PluginRegistry &plugin_registry, PortRegistry &port_registry)
    override;

  bool is_in_active_project () const override
  {
    return Track::is_in_active_project ();
  }

  ScaleObject * get_scale_at (size_t index) const;

/**
 * Returns the current chord.
 */
#define get_chord_at_playhead() get_chord_at_pos (PLAYHEAD)

  /**
   * Returns the ChordObject at the given Position
   * in the TimelineArranger.
   */
  ChordObject * get_chord_at_pos (Position pos) const;

  /**
   * Returns the ScaleObject at the given Position
   * in the TimelineArranger.
   */
  ScaleObject * get_scale_at_pos (Position pos) const;

  bool validate () const override;

  void clear_objects () override;

  void get_regions_in_range (
    std::vector<Region *> &regions,
    const Position *       p1,
    const Position *       p2) override
  {
    std::ranges::for_each (regions, [&] (auto &region) {
      add_region_if_in_range (p1, p2, regions, region);
    });
  }

  void init_after_cloning (const ChordTrack &other, ObjectCloneType clone_type)
    override;

  void
  append_ports (std::vector<Port *> &ports, bool include_plugins) const final;

  ArrangerObjectOwner<ChordRegion>::Location
  get_location (const ChordRegion &) const override
  {
    return { .track_id_ = get_uuid () };
  }
  ArrangerObjectOwner<ScaleObject>::Location
  get_location (const ScaleObject &) const override
  {
    return { .track_id_ = get_uuid () };
  }

  std::string
  get_field_name_for_serialization (const ChordRegion *) const override
  {
    return "regions";
  }
  std::string
  get_field_name_for_serialization (const ScaleObject *) const override
  {
    return "scales";
  }

private:
  friend void to_json (nlohmann::json &j, const ChordTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
    to_json (j, static_cast<const ProcessableTrack &> (track));
    to_json (j, static_cast<const AutomatableTrack &> (track));
    to_json (j, static_cast<const ChannelTrack &> (track));
    to_json (j, static_cast<const RecordableTrack &> (track));
    to_json (j, static_cast<const ArrangerObjectOwner<ChordRegion> &> (track));
    to_json (j, static_cast<const ArrangerObjectOwner<ScaleObject> &> (track));
  }
  friend void from_json (const nlohmann::json &j, ChordTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
    from_json (j, static_cast<ProcessableTrack &> (track));
    from_json (j, static_cast<AutomatableTrack &> (track));
    from_json (j, static_cast<ChannelTrack &> (track));
    from_json (j, static_cast<RecordableTrack &> (track));
    from_json (j, static_cast<ArrangerObjectOwner<ChordRegion> &> (track));
    from_json (j, static_cast<ArrangerObjectOwner<ScaleObject> &> (track));
  }

  bool initialize ();
  void set_playback_caches () override;
};

} // namespace zrythm::structure::tracks
