// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/marker_track.h"
#include "common/dsp/track.h"
#include "common/utils/rt_thread_id.h"
#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/timeline_selections.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

MarkerTrack::MarkerTrack (int track_pos)
    : Track (
        Track::Type::Marker,
        _ ("Markers"),
        track_pos,
        PortType::Unknown,
        PortType::Unknown)
{
  main_height_ = TRACK_DEF_HEIGHT / 2;
  icon_name_ = "gnome-icon-library-flag-outline-thick-symbolic";
  color_ = Color ("#813D9C");
}

void
MarkerTrack::add_default_markers ()
{
  /* add start and end markers */
  auto     marker_name = fmt::format ("[{}]", _ ("start"));
  auto     marker = std::make_shared<Marker> (marker_name);
  Position pos;
  pos.set_to_bar (1);
  marker->pos_setter (&pos);
  marker->marker_type_ = Marker::Type::Start;
  add_marker (marker);

  marker_name = fmt::format ("[{}]", _ ("end"));
  marker = std::make_shared<Marker> (marker_name);
  pos.set_to_bar (129);
  marker->pos_setter (&pos);
  marker->marker_type_ = Marker::Type::End;
  add_marker (marker);
}

bool
MarkerTrack::initialize ()
{
  return true;
}

void
MarkerTrack::init_loaded ()
{
  for (auto &marker : markers_)
    {
      marker->init_loaded ();
    }
}

MarkerTrack::MarkerPtr
MarkerTrack::get_start_marker () const
{
  auto it =
    std::find_if (markers_.begin (), markers_.end (), [] (const auto &marker) {
      return marker->marker_type_ == Marker::Type::Start;
    });
  z_return_val_if_fail (it != markers_.end (), nullptr);
  return *it;
}

MarkerTrack::MarkerPtr
MarkerTrack::get_end_marker () const
{
  auto it =
    std::find_if (markers_.begin (), markers_.end (), [] (const auto &marker) {
      return marker->marker_type_ == Marker::Type::End;
    });
  z_return_val_if_fail (it != markers_.end (), nullptr);
  return *it;
}

MarkerTrack::MarkerPtr
MarkerTrack::insert_marker (std::shared_ptr<Marker> marker, int pos)
{
  marker->set_track_name_hash (get_name_hash ());
  markers_.insert (markers_.begin () + pos, marker);

  for (size_t i = pos; i < markers_.size (); ++i)
    {
      auto m = markers_[i];
      m->set_marker_track_index (i);
    }

  z_return_val_if_fail (validate (), nullptr);

  EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CREATED, marker.get ());

  return marker;
}

void
MarkerTrack::clear_objects ()
{
  for (auto &marker : markers_ | std::views::reverse)
    {
      if (marker->is_start () || marker->is_end ())
        continue;
      remove_marker (*marker, true);
    }
}

void
MarkerTrack::set_playback_caches ()
{
  marker_snapshots_.clear ();
  marker_snapshots_.reserve (markers_.size ());
  for (const auto &marker : markers_)
    {
      marker_snapshots_.push_back (marker->clone_unique ());
    }
}

void
MarkerTrack::init_after_cloning (const MarkerTrack &other)
{
  clone_unique_ptr_container (markers_, other.markers_);
  Track::copy_members_from (other);
}

bool
MarkerTrack::validate () const
{
  if (!Track::validate_base ())
    {
      return false;
    }

  for (size_t i = 0; i < markers_.size (); ++i)
    {
      auto m = markers_[i];
      z_return_val_if_fail (m->marker_track_index_ == (int) i, false);
    }
  return true;
}

MarkerTrack::MarkerPtr
MarkerTrack::remove_marker (Marker &marker, bool fire_events)
{
  /* deselect */
  TL_SELECTIONS->remove_object (marker);

  auto it =
    std::find_if (markers_.begin (), markers_.end (), [&] (const auto &m) {
      return m.get () == &marker;
    });
  z_return_val_if_fail (it != markers_.end (), nullptr);
  auto ret = *it;
  it = markers_.erase (it);

  for (
    size_t i = std::distance (markers_.begin (), it); i < markers_.size (); ++i)
    {
      auto m = markers_[i];
      m->set_marker_track_index (i);
    }

  EVENTS_PUSH (
    EventType::ET_ARRANGER_OBJECT_REMOVED, ArrangerObject::Type::Marker);

  return ret;
}
