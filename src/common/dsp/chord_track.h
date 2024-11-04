// SPDX-FileCopyrightText: © 2018-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CHORD_TRACK_H__
#define __AUDIO_CHORD_TRACK_H__

#include <memory>
#include <ranges>
#include <vector>

#include "common/dsp/channel_track.h"
#include "common/dsp/chord_region.h"
#include "common/dsp/recordable_track.h"
#include "common/dsp/region_owner.h"
#include "common/dsp/scale_object.h"

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
 */
class ChordTrack final
    : public QObject,
      public RecordableTrack,
      public ChannelTrack,
      public RegionOwnerImpl<ChordRegion>,
      public ICloneable<ChordTrack>,
      public ISerializable<ChordTrack>,
      public InitializableObjectFactory<ChordTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (ChordTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (ChordTrack)
  DEFINE_CHANNEL_TRACK_QML_PROPERTIES (ChordTrack)

  friend class InitializableObjectFactory<ChordTrack>;

public:
  void init_loaded () override;

  bool is_in_active_project () const override
  {
    return Track::is_in_active_project ();
  }

  bool is_auditioner () const override { return Track::is_auditioner (); }

  /**
   * Inserts a scale to the track.
   */
  std::shared_ptr<ScaleObject>
  insert_scale (std::shared_ptr<ScaleObject> scale, int index);

  /**
   * Adds a scale to the track.
   */
  std::shared_ptr<ScaleObject> add_scale (std::shared_ptr<ScaleObject> scale)
  {
    return insert_scale (std::move (scale), scales_.size ());
  }

  /**
   * Removes a scale from the chord Track.
   */
  void remove_scale (ScaleObject &scale);

/**
 * Returns the current chord.
 */
#define get_chord_at_playhead() get_chord_at_pos (PLAYHEAD)

  /**
   * Returns the ChordObject at the given Position
   * in the TimelineArranger.
   */
  ChordObject * get_chord_at_pos (const Position pos) const;

/**
 * Returns the current scale.
 */
#define get_scale_at_playhead() get_scale_at_pos (PLAYHEAD)

  /**
   * Returns the ScaleObject at the given Position
   * in the TimelineArranger.
   */
  ScaleObject * get_scale_at_pos (const Position pos) const;

  bool validate () const override;

  void clear_objects () override;

  void get_regions_in_range (
    std::vector<Region *> &regions,
    const Position *       p1,
    const Position *       p2) override
  {
    for (auto &region : regions_)
      {
        add_region_if_in_range (p1, p2, regions, region.get ());
      }
  }

  void init_after_cloning (const ChordTrack &other) override
  {
    Track::copy_members_from (other);
    AutomatableTrack::copy_members_from (other);
    ProcessableTrack::copy_members_from (other);
    RecordableTrack::copy_members_from (other);
    ChannelTrack::copy_members_from (other);
    RegionOwnerImpl::copy_members_from (other);
    clone_unique_ptr_container (scales_, other.scales_);
  }

  void
  append_ports (std::vector<Port *> &ports, bool include_plugins) const final;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  ChordTrack (int track_pos = 0);

  bool initialize () override;
  void set_playback_caches () override;

public:
  /**
   * @note These must always be sorted by Position.
   */
  std::vector<std::shared_ptr<ScaleObject>> scales_;

  /** Snapshots used during playback TODO unimplemented. */
  std::vector<std::unique_ptr<ScaleObject>> scale_snapshots_;
};

/**
 * @}
 */

#endif
