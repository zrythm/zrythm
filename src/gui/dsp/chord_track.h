// SPDX-FileCopyrightText: © 2018-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef GUI_DSP_CHORD_TRACK_H
#define GUI_DSP_CHORD_TRACK_H

#include "gui/dsp/arranger_object_owner.h"
#include "gui/dsp/channel_track.h"
#include "gui/dsp/chord_region.h"
#include "gui/dsp/recordable_track.h"
#include "gui/dsp/scale_object.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

#define P_CHORD_TRACK (TRACKLIST->chord_track_)

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
      public ArrangerObjectOwner<ChordRegion>,
      public ArrangerObjectOwner<ScaleObject>,
      public ICloneable<ChordTrack>,
      public zrythm::utils::serialization::ISerializable<ChordTrack>,
      public utils::InitializableObject
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (ChordTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (ChordTrack)
  DEFINE_CHANNEL_TRACK_QML_PROPERTIES (ChordTrack)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (ChordTrack, regions, ChordRegion)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    ChordTrack,
    scaleObjects,
    ScaleObject)
  DEFINE_RECORDABLE_TRACK_QML_PROPERTIES (ChordTrack)

  friend class InitializableObject;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (ChordTrack)

public:
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

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  bool initialize ();
  void set_playback_caches () override;
};

/**
 * @}
 */

#endif
