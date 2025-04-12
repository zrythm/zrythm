// SPDX-FileCopyrightText: © 2019-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MARKER_TRACK_H__
#define __AUDIO_MARKER_TRACK_H__

#include "gui/dsp/arranger_object_span.h"
#include "gui/dsp/marker.h"
#include "gui/dsp/track.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

#define P_MARKER_TRACK (TRACKLIST->marker_track_)

class MarkerTrack final
    : public QAbstractListModel,
      public Track,
      public ICloneable<MarkerTrack>,
      public zrythm::utils::serialization::ISerializable<MarkerTrack>,
      public utils::InitializableObject
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (MarkerTrack)

  friend class InitializableObject;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (MarkerTrack)

public:
  using MarkerPtr = Marker *;

  enum MarkerRoles
  {
    MarkerPtrRole = Qt::UserRole + 1,
  };

  // ========================================================================
  // QML Interface
  // ========================================================================
  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  // ========================================================================

  void
  init_loaded (PluginRegistry &plugin_registry, PortRegistry &port_registry)
    override;

  /**
   * Inserts a marker to the track.
   *
   * This takes ownership of the marker.
   */
  MarkerPtr insert_marker (ArrangerObjectUuidReference marker_ref, int pos);

  /**
   * Appends a marker to the track.
   *
   * @see insert_marker.
   */
  MarkerPtr add_marker (auto marker_ref)
  {
    return insert_marker (marker_ref, markers_.size ());
  }

  /**
   * Removes all objects from the marker track.
   *
   * Mainly used in testing.
   */
  void clear_objects () override;

  /**
   * Removes a marker.
   */
  void remove_marker (const Marker::Uuid &marker_id);

  bool validate () const override;

  auto get_markers ()
  {
    return std::views::transform (markers_, [&] (auto &&marker_id) {
      return std::get<Marker *> (marker_id.get_object ());
    });
  }
  auto get_markers () const
  {
    return std::views::transform (markers_, [&] (auto &&marker_id) {
      return std::get<Marker *> (marker_id.get_object ());
    });
  }
  auto get_marker_at (size_t index) const { return get_markers ()[index]; }

  bool validate_marker_name (const std::string &name)
  {
    /* valid if no other marker with the same name exists*/
    return !std::ranges::contains (get_markers (), name, [] (const auto &marker) {
      return marker->get_name ();
    });
  }

  /**
   * Returns the start marker.
   */
  MarkerPtr get_start_marker () const;

  /**
   * Returns the end marker.
   */
  MarkerPtr get_end_marker () const;

  void init_after_cloning (const MarkerTrack &other, ObjectCloneType clone_type)
    override;

  void
  append_ports (std::vector<Port *> &ports, bool include_plugins) const final;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  bool initialize ();
  void set_playback_caches () override;

public:
  std::vector<ArrangerObjectUuidReference> markers_;

  /** Snapshots used during playback TODO unimplemented. */
  std::vector<std::unique_ptr<Marker>> marker_snapshots_;
};

/**
 * @}
 */

#endif
