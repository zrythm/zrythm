// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/chord_region.h"
#include "gui/dsp/chord_track.h"
#include "gui/dsp/track.h"
#include "utils/rt_thread_id.h"

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
      icon_name_ = "gnome-icon-library-library-music-symbolic";
    }
  automation_tracklist_->setParent (this);
}

void
ChordTrack::init_after_cloning (
  const ChordTrack &other,
  ObjectCloneType   clone_type)
{
  Track::copy_members_from (other, clone_type);
  AutomatableTrack::copy_members_from (other, clone_type);
  ProcessableTrack::copy_members_from (other, clone_type);
  RecordableTrack::copy_members_from (other, clone_type);
  ChannelTrack::copy_members_from (other, clone_type);
  ArrangerObjectOwner<ChordRegion>::copy_members_from (other, clone_type);
  ArrangerObjectOwner<ScaleObject>::copy_members_from (other, clone_type);
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
  for (auto * region : ArrangerObjectOwner<ChordRegion>::get_children_view ())
    {
      region->init_loaded ();
    }
  for (auto * scale : ArrangerObjectOwner<ScaleObject>::get_children_view ())
    {
      scale->init_loaded ();
    }
}

ScaleObject *
ChordTrack::get_scale_at (size_t index) const
{
  return ArrangerObjectOwner<ScaleObject>::get_children_view ()[index];
}

ScaleObject *
ChordTrack::get_scale_at_pos (const Position pos) const
{
  auto view = std::ranges::reverse_view (
    ArrangerObjectOwner<ScaleObject>::get_children_view ());
  auto it = std::ranges::find_if (view, [&pos] (const auto &scale) {
    return *scale->pos_ <= pos;
  });

  return it != view.end () ? (*it) : nullptr;
}

ChordObject *
ChordTrack::get_chord_at_pos (const Position pos) const
{
  auto region_var =
    ArrangerObjectUuidReferenceSpan{
      ArrangerObjectOwner<ChordRegion>::get_children_vector ()
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
      return co->pos_->frames_ <= local_frames;
    });

  return it != chord_objects_view.end () ? (*it) : nullptr;
}

bool
ChordTrack::validate () const
{
  if (
    !Track::validate_base () || !ChannelTrack::validate_base ()
    || !AutomatableTrack::validate_base ())
    return false;

  bool valid = std::ranges::all_of (
    ArrangerObjectOwner<ChordRegion>::get_children_view (),
    [&] (const auto * region) {
      return region->validate (Track::is_in_active_project (), 0);
    });
  z_return_val_if_fail (valid, false);

#if 0
  for (size_t i = 0; i < scales_.size (); i++)
    {
      auto &m = scales_[i];
      z_return_val_if_fail (
        m->index_in_chord_track_ == static_cast<int> (i), false);
    }
#endif

  return true;
}
