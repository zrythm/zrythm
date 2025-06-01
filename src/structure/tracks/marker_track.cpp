// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/marker_track.h"
#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
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
      icon_name_ = u8"gnome-icon-library-flag-filled-symbolic";
      color_ = Color (QColor ("#7C009B"));
    }
}

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
}

auto
MarkerTrack::get_start_marker () const -> Marker *
{
  auto markers = get_children_view ();
  auto it = std::ranges::find_if (markers, [] (const auto &marker) {
    return marker->marker_type_ == Marker::Type::Start;
  });
  z_return_val_if_fail (it != markers.end (), nullptr);
  return *it;
}

auto
MarkerTrack::get_end_marker () const -> Marker *
{
  auto markers = get_children_view ();
  auto it = std::ranges::find_if (markers, [] (const auto &marker) {
    return marker->marker_type_ == Marker::Type::End;
  });
  z_return_val_if_fail (it != markers.end (), nullptr);
  return *it;
}
void
MarkerTrack::clear_objects ()
{
  std::vector<Marker::Uuid> markers_to_delete;
  for (auto * marker : get_children_view () | std::views::reverse)
    {
      if (marker->is_start () || marker->is_end ())
        continue;
      markers_to_delete.push_back (marker->get_uuid ());
    }
  for (const auto &id : markers_to_delete)
    {
      remove_object (id);
    }
}

void
MarkerTrack::set_playback_caches ()
{
  // TODO
#if 0
  marker_snapshots_.clear ();
  marker_snapshots_.reserve (markers_.size ());

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
  ArrangerObjectOwner::copy_members_from (other, clone_type);
  Track::copy_members_from (other, clone_type);
}

bool
MarkerTrack::validate () const
{
  if (!Track::validate_base ())
    {
      return false;
    }

  return true;
}

void
MarkerTrack::append_ports (std::vector<Port *> &ports, bool include_plugins) const
{
}
}