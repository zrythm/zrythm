// SPDX-FileCopyrightText: © 2018-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CHORD_TRACK_H__
#define __AUDIO_CHORD_TRACK_H__

#include <memory>
#include <ranges>
#include <vector>

#include "gui/dsp/channel_track.h"
#include "gui/dsp/chord_region.h"
#include "gui/dsp/recordable_track.h"
#include "gui/dsp/region_owner.h"
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
    : public QAbstractListModel,
      public RecordableTrack,
      public ChannelTrack,
      public RegionOwner<ChordRegion>,
      public ICloneable<ChordTrack>,
      public zrythm::utils::serialization::ISerializable<ChordTrack>,
      public utils::InitializableObject
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (ChordTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (ChordTrack)
  DEFINE_CHANNEL_TRACK_QML_PROPERTIES (ChordTrack)
  DEFINE_REGION_OWNER_QML_PROPERTIES (ChordTrack)

  friend class InitializableObject;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (ChordTrack)

public:
  using ScaleObjectPtr = ScaleObject *;

  // =========================================================================
  // QML Interface
  // =========================================================================

  enum Role
  {
    ScaleObjectPtrRole = Qt::UserRole + 1,
  };

  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  // =========================================================================

  void
  init_loaded (PluginRegistry &plugin_registry, PortRegistry &port_registry)
    override;

  bool is_in_active_project () const override
  {
    return Track::is_in_active_project ();
  }

  bool is_auditioner () const override { return Track::is_auditioner (); }

  /**
   * Inserts a scale object to the track.
   *
   * This takes ownership of the scale object.
   */
  void insert_scale (ArrangerObjectUuidReference scale_ref, int index);

  /**
   * Adds a scale to the track.
   */
  void add_scale (auto scale_ref)
  {
    return insert_scale (scale_ref, scales_.size ());
  }

  /**
   * Removes a scale from the chord Track.
   */
  void remove_scale (const ArrangerObject::Uuid &scale_id);

  auto get_scales_view ()
  {
    return scales_ | std::views::transform ([&] (const auto &id) {
             return std::get<ScaleObject *> (id.get_object ());
           });
  }

  auto get_scales_view () const
  {
    return scales_ | std::views::transform ([&] (const auto &id) {
             return std::get<ScaleObject *> (id.get_object ());
           });
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

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  bool initialize ();
  void set_playback_caches () override;

public:
  ArrangerObjectRegistry &object_registry_;

  /**
   * @note These must always be sorted by Position.
   */
  std::vector<ArrangerObjectUuidReference> scales_;

  /** Snapshots used during playback TODO unimplemented. */
  std::vector<std::unique_ptr<ScaleObject>> scale_snapshots_;
};

/**
 * @}
 */

#endif
