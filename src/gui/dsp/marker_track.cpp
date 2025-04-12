// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/marker_track.h"
#include "gui/dsp/track.h"
#include "utils/rt_thread_id.h"

MarkerTrack::MarkerTrack (
  TrackRegistry          &track_registry,
  PluginRegistry         &plugin_registry,
  PortRegistry           &port_registry,
  ArrangerObjectRegistry &obj_registry,
  bool                    new_identity)
    : Track (
        Track::Type::Marker,
        PortType::Unknown,
        PortType::Unknown,
        plugin_registry,
        port_registry,
        obj_registry)
{
  if (new_identity)
    {
      main_height_ = DEF_HEIGHT / 2;
      icon_name_ = "gnome-icon-library-flag-filled-symbolic";
      color_ = Color (QColor ("#7C009B"));
    }
}

// ========================================================================
// QML Interface
// ========================================================================
QHash<int, QByteArray>
MarkerTrack::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[MarkerPtrRole] = "marker";
  return roles;
}

int
MarkerTrack::rowCount (const QModelIndex &parent) const
{
  return static_cast<int> (markers_.size ());
}

QVariant
MarkerTrack::data (const QModelIndex &index, int role) const
{
  if (!index.isValid ())
    return {};

  auto * marker = get_marker_at (index.row ());

  switch (role)
    {
    case MarkerPtrRole:
      return QVariant::fromValue (marker);
    default:
      return {};
    }
}

// ========================================================================

bool
MarkerTrack::initialize ()
{
  return true;
}

void
MarkerTrack::init_loaded (
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry)
{
  for (auto * marker : get_markers ())
    {
      marker->init_loaded ();
    }
}

MarkerTrack::MarkerPtr
MarkerTrack::get_start_marker () const
{
  auto markers = get_markers ();
  auto it = std::ranges::find_if (markers, [] (const auto &marker) {
    return marker->marker_type_ == Marker::Type::Start;
  });
  z_return_val_if_fail (it != markers.end (), nullptr);
  return *it;
}

MarkerTrack::MarkerPtr
MarkerTrack::get_end_marker () const
{
  auto markers = get_markers ();
  auto it = std::ranges::find_if (markers, [] (const auto &marker) {
    return marker->marker_type_ == Marker::Type::End;
  });
  z_return_val_if_fail (it != markers.end (), nullptr);
  return *it;
}

MarkerTrack::MarkerPtr
MarkerTrack::insert_marker (ArrangerObjectUuidReference marker_ref, int pos)
{
  auto * marker = std::get<Marker *> (marker_ref.get_object ());
  marker->set_track_id (get_uuid ());
  markers_.insert (markers_.begin () + pos, marker_ref);
  marker->setParent (this);

  for (size_t i = pos; i < markers_.size (); ++i)
    {
      auto * m = get_markers ()[i];
      m->set_marker_track_index (i);
    }

  z_return_val_if_fail (validate (), nullptr);

  // EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CREATED, marker.get ());

  return marker;
}

void
MarkerTrack::clear_objects ()
{
  std::vector<Marker::Uuid> markers_to_delete;
  for (auto * marker : get_markers () | std::views::reverse)
    {
      if (marker->is_start () || marker->is_end ())
        continue;
      markers_to_delete.push_back (marker->get_uuid ());
    }
  for (const auto &id : markers_to_delete)
    {
      remove_marker (id);
    }
}

void
MarkerTrack::set_playback_caches ()
{
  marker_snapshots_.clear ();
  marker_snapshots_.reserve (markers_.size ());
// TODO
#if 0
  for (const auto &marker : markers_)
    {
      marker_snapshots_.push_back (marker->clone_unique ());
    }
#endif
}

void
MarkerTrack::init_after_cloning (
  const MarkerTrack &other,
  ObjectCloneType    clone_type)
{
  markers_.reserve (other.markers_.size ());
// TODO
#if 0
  for (auto &marker : markers_)
    {
      auto * clone = marker->clone_raw_ptr ();
      clone->setParent (this);
      markers_.push_back (clone);
    }
#endif
  Track::copy_members_from (other, clone_type);
}

bool
MarkerTrack::validate () const
{
  if (!Track::validate_base ())
    {
      return false;
    }

  for (const auto &[index, m] : std::views::enumerate (get_markers ()))
    {
      z_return_val_if_fail (m->marker_track_index_ == index, false);
    }
  return true;
}

void
MarkerTrack::append_ports (std::vector<Port *> &ports, bool include_plugins) const
{
}

void
MarkerTrack::remove_marker (const Marker::Uuid &marker_id)
{
  auto it =
    std::ranges::find (markers_, marker_id, &ArrangerObjectUuidReference::id);
  assert (it != markers_.end ());
  it = markers_.erase (it);

  for (
    size_t i = std::distance (markers_.begin (), it); i < markers_.size (); ++i)
    {
      auto * m = get_marker_at (i);
      m->set_marker_track_index (i);
    }
}
