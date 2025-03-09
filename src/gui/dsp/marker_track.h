// SPDX-FileCopyrightText: © 2019-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MARKER_TRACK_H__
#define __AUDIO_MARKER_TRACK_H__

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
   * @brief Adds the start/end markers.
   */
  void add_default_markers (int ticks_per_bar, double frames_per_tick);

  /**
   * Inserts a marker to the track.
   *
   * This takes ownership of the marker.
   */
  MarkerPtr insert_marker (MarkerPtr marker, int pos);

  /**
   * Appends a marker to the track.
   *
   * @see insert_marker.
   */
  MarkerPtr add_marker (MarkerPtr marker)
  {
    return insert_marker (marker, markers_.size ());
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
  MarkerPtr remove_marker (Marker &marker, bool free_marker, bool fire_events);

  bool validate () const override;

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
  std::vector<MarkerPtr> markers_;

  /** Snapshots used during playback TODO unimplemented. */
  std::vector<std::unique_ptr<Marker>> marker_snapshots_;
};

/**
 * @}
 */

#endif
