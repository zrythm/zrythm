// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_region.h"
#include "dsp/chord_track.h"
#include "dsp/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/rt_thread_id.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

ChordTrack::ChordTrack () : ChordTrack (0) { }

ChordTrack::ChordTrack (int pos)
    : Track (Track::Type::Chord, _ ("Chords"), pos, PortType::Event, PortType::Event)
{
  color_ = Color ("#1C71D8");
  icon_name_ = "minuet-chords";
}

bool
ChordTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();

  return true;
}

void
ChordTrack::set_playback_caches ()
{
  region_snapshots_.clear ();
  region_snapshots_.reserve (regions_.size ());
  for (const auto &region : regions_)
    {
      region_snapshots_.push_back (region->clone_unique ());
    }

  scale_snapshots_.clear ();
  scale_snapshots_.reserve (scales_.size ());
  for (const auto &scale : scales_)
    {
      scale_snapshots_.push_back (scale->clone_unique ());
    }
}

void
ChordTrack::init_loaded ()
{
  for (auto &scale : scales_)
    {
      scale->init_loaded ();
    }
  for (auto &chord_region : regions_)
    {
      chord_region->track_name_hash_ =
        dynamic_cast<Track &> (*this).get_name_hash ();
      chord_region->init_loaded ();
    }
}

std::shared_ptr<ScaleObject>
ChordTrack::insert_scale (std::shared_ptr<ScaleObject> scale, int idx)
{
  assert (idx >= 0);
  scales_.insert (scales_.begin () + idx, std::move (scale));
  for (size_t i = 0; i < scales_.size (); i++)
    {
      auto &s = scales_[i];
      s->set_index_in_chord_track (i);
    }
  auto &ret = scales_[idx];

  EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CREATED, ret.get ());

  return ret;
}

ScaleObject *
ChordTrack::get_scale_at_pos (const Position pos) const
{
  auto it = std::ranges::find_if (
    scales_ | std::views::reverse,
    [&pos] (const auto &scale) { return scale->pos_ <= pos; });

  return it != scales_.rend () ? (*it).get () : nullptr;
}

ChordObject *
ChordTrack::get_chord_at_pos (const Position pos) const
{
  auto region = get_region_at_pos (pos, false);
  if (!region)
    {
      return nullptr;
    }

  auto local_frames = (signed_frame_t) region->timeline_frames_to_local (
    pos.frames_, F_NORMALIZE);

  auto it = std::ranges::find_if (
    region->chord_objects_ | std::views::reverse,
    [local_frames] (const auto &co) {
      return co->pos_.frames_ <= local_frames;
    });

  return it != region->chord_objects_.rend () ? (*it).get () : nullptr;
}

void
ChordTrack::remove_scale (ScaleObject &scale)
{
  // Deselect the scale
  scale.select (false, false, false);

  // Find and remove the scale from the vector
  auto it =
    std::find_if (scales_.begin (), scales_.end (), [&scale] (const auto &s) {
      return s.get () == &scale;
    });
  z_return_if_fail (it != scales_.end ());

  int pos = std::distance (scales_.begin (), it);
  scales_.erase (it);

  scale.index_in_chord_track_ = -1;

  // Update indices of remaining scales
  for (size_t i = pos; i < scales_.size (); i++)
    {
      scales_[i]->set_index_in_chord_track (static_cast<int> (i));
    }

  EVENTS_PUSH (
    EventType::ET_ARRANGER_OBJECT_REMOVED, ArrangerObject::Type::ScaleObject);
}

bool
ChordTrack::validate () const
{
  if (!ChannelTrack::validate ())
    return false;

  for (const auto &region : regions_)
    {
      z_return_val_if_fail (
        region->validate (Track::is_in_active_project (), 0), false);
    }

  for (size_t i = 0; i < scales_.size (); i++)
    {
      auto &m = scales_[i];
      z_return_val_if_fail (
        m->index_in_chord_track_ == static_cast<int> (i), false);
    }

  return true;
}