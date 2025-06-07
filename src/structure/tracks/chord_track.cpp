// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/arrangement/chord_region.h"
#include "structure/tracks/chord_track.h"
#include "structure/tracks/track.h"
#include "utils/rt_thread_id.h"

namespace zrythm::structure::tracks
{
ChordTrack::ChordTrack (
  TrackRegistry          &track_registry,
  PluginRegistry         &plugin_registry,
  PortRegistry           &port_registry,
  ArrangerObjectRegistry &obj_registry,
  bool                    new_identity)
    : Track (
        Track::Type::Chord,
        PortType::Event,
        PortType::Event,
        plugin_registry,
        port_registry,
        obj_registry),
      AutomatableTrack (port_registry, new_identity),
      ProcessableTrack (port_registry, new_identity),
      RecordableTrack (port_registry, new_identity),
      ChannelTrack (track_registry, plugin_registry, port_registry, new_identity)
{
  if (new_identity)
    {
      color_ = Color (QColor ("#1C8FFB"));
      icon_name_ = u8"gnome-icon-library-library-music-symbolic";
    }
  automation_tracklist_->setParent (this);
}

void
init_from (
  ChordTrack            &obj,
  const ChordTrack      &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
  init_from (
    static_cast<AutomatableTrack &> (obj),
    static_cast<const AutomatableTrack &> (other), clone_type);
  init_from (
    static_cast<ProcessableTrack &> (obj),
    static_cast<const ProcessableTrack &> (other), clone_type);
  init_from (
    static_cast<RecordableTrack &> (obj),
    static_cast<const RecordableTrack &> (other), clone_type);
  init_from (
    static_cast<ChannelTrack &> (obj),
    static_cast<const ChannelTrack &> (other), clone_type);
  init_from (
    static_cast<arrangement::ArrangerObjectOwner<arrangement::ChordRegion> &> (
      obj),
    static_cast<const arrangement::ArrangerObjectOwner<
      arrangement::ChordRegion> &> (other),
    clone_type);
  init_from (
    static_cast<arrangement::ArrangerObjectOwner<arrangement::ScaleObject> &> (
      obj),
    static_cast<const arrangement::ArrangerObjectOwner<
      arrangement::ScaleObject> &> (other),
    clone_type);
}

void
ChordTrack::append_ports (std::vector<Port *> &ports, bool include_plugins) const
{
  ChannelTrack::append_member_ports (ports, include_plugins);
  ProcessableTrack::append_member_ports (ports, include_plugins);
  RecordableTrack::append_member_ports (ports, include_plugins);
}

bool
ChordTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();
  init_recordable_track ([] () {
    return ZRYTHM_HAVE_UI
           && zrythm::gui::SettingsManager::get_instance ()->get_trackAutoArm ();
  });

  return true;
}

void
ChordTrack::clear_objects ()
{
  ArrangerObjectOwner<ChordRegion>::clear_objects ();
  ArrangerObjectOwner<ScaleObject>::clear_objects ();
}

void
ChordTrack::set_playback_caches ()
{
// FIXME: TODO
#if 0
  region_snapshots_.clear ();
  region_snapshots_.reserve (region_list_->regions_.size ());
  foreach_region ([&] (auto &region) {
    z_return_if_fail (region.track_id_ == get_uuid ());
    region_snapshots_.push_back (region.clone_unique ());
  });

  scale_snapshots_.clear ();
  scale_snapshots_.reserve (scales_.size ());
  for (const auto &scale : get_scales_view ())
    {
      scale_snapshots_.push_back (scale->clone_unique ());
    }
#endif
}

void
ChordTrack::init_loaded (
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry)
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded (plugin_registry, port_registry);
  AutomatableTrack::init_loaded (plugin_registry, port_registry);
  ProcessableTrack::init_loaded (plugin_registry, port_registry);
  RecordableTrack::init_loaded (plugin_registry, port_registry);
}

auto
ChordTrack::get_scale_at (size_t index) const -> ScaleObject *
{
  return ArrangerObjectOwner<ScaleObject>::get_children_view ()[index];
}

auto
ChordTrack::get_scale_at_pos (const Position pos) const -> ScaleObject *
{
  auto view = std::ranges::reverse_view (
    ArrangerObjectOwner<ScaleObject>::get_children_view ());
  auto it = std::ranges::find_if (view, [&pos] (const auto &scale) {
    return scale->get_position () <= pos;
  });

  return it != view.end () ? (*it) : nullptr;
}

auto
ChordTrack::get_chord_at_pos (const Position pos) const -> ChordObject *
{
  auto region_var =
    arrangement::ArrangerObjectSpan{
      arrangement::ArrangerObjectOwner<
        arrangement::ChordRegion>::get_children_vector ()
    }
      .get_bounded_object_at_pos (pos, false);
  if (!region_var)
    {
      return nullptr;
    }
  const auto * region = std::get<ChordRegion *> (*region_var);

  const auto local_frames = region->timeline_frames_to_local (pos.frames_, true);

  auto chord_objects_view = region->get_children_view () | std::views::reverse;
  auto it =
    std::ranges::find_if (chord_objects_view, [local_frames] (const auto &co) {
      return co->get_position ().frames_ <= local_frames;
    });

  return it != chord_objects_view.end () ? (*it) : nullptr;
}

}
