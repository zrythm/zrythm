// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/chord_region.h"
#include "common/dsp/chord_track.h"
#include "common/dsp/track.h"
#include "common/utils/flags.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

ChordTrack::ChordTrack (int pos)
    : Track (
        Track::Type::Chord,
        QObject::tr ("Chords").toStdString (),
        pos,
        PortType::Event,
        PortType::Event)
{
  color_ = Color (QColor ("#1C8FFB"));
  icon_name_ = "gnome-icon-library-library-music-symbolic";
  automation_tracklist_->setParent (this);
  RegionOwnerImpl::parent_base_qproperties (*this);
}

// =========================================================================
// QML Interface
// =========================================================================

QHash<int, QByteArray>
ChordTrack::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[ScaleObjectPtrRole] = "scaleObject";
  return roles;
}

int
ChordTrack::rowCount (const QModelIndex &parent) const
{
  return static_cast<int> (scales_.size ());
}

QVariant
ChordTrack::data (const QModelIndex &index, int role) const
{
  if (!index.isValid ())
    return {};

  auto * scale = scales_.at (index.row ());

  switch (role)
    {
    case ScaleObjectPtrRole:
      return QVariant::fromValue (scale);
    default:
      return {};
    }
}

// =========================================================================

void
ChordTrack::init_after_cloning (const ChordTrack &other)
{
  Track::copy_members_from (other);
  AutomatableTrack::copy_members_from (other);
  ProcessableTrack::copy_members_from (other);
  RecordableTrack::copy_members_from (other);
  ChannelTrack::copy_members_from (other);
  RegionOwnerImpl::copy_members_from (other);
  scales_.reserve (other.scales_.size ());
  for (const auto &scale : other.scales_)
    {
      scales_.push_back (scale->clone_raw_ptr ());
      scales_.back ()->setParent (this);
    }
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

  return true;
}

void
ChordTrack::clear_objects ()
{
  beginResetModel ();
  clear_regions ();
  if (is_in_active_project ())
    {
      for (auto &scale : std::ranges::reverse_view (scales_))
        {
          remove_scale (*scale, true);
        }
    }
  else
    {
      scales_.clear ();
    }
  scale_snapshots_.clear ();
  endResetModel ();
}

void
ChordTrack::set_playback_caches ()
{
  region_snapshots_.clear ();
  region_snapshots_.reserve (region_list_->regions_.size ());
  const auto name_hash = get_name_hash ();
  foreach_region ([&] (auto &region) {
    z_return_if_fail (region.track_name_hash_ == name_hash);
    region_snapshots_.push_back (region.clone_unique ());
  });

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
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded ();
  AutomatableTrack::init_loaded ();
  ProcessableTrack::init_loaded ();
  RecordableTrack::init_loaded ();
  for (auto &scale : scales_)
    {
      scale->init_loaded ();
    }

  foreach_region ([&] (auto &chord_region) {
    chord_region.track_name_hash_ =
      dynamic_cast<Track &> (*this).get_name_hash ();
    chord_region.init_loaded ();
  });
}

void
ChordTrack::insert_scale (ScaleObject &scale, int idx)
{
  z_return_if_fail (idx >= 0);
  z_return_if_fail (name_hash_ != 0);
  beginInsertRows ({}, idx, idx);
  scale.set_track_name_hash (name_hash_);
  scales_.insert (scales_.begin () + idx, &scale);
  for (size_t i = 0; i < scales_.size (); i++)
    {
      auto &s = scales_[i];
      s->set_index_in_chord_track (i);
    }
  scale.setParent (this);
  endInsertRows ();

  // EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CREATED, ret.get ());
}

ScaleObject *
ChordTrack::get_scale_at_pos (const Position pos) const
{
  auto it = std::ranges::find_if (
    scales_ | std::views::reverse,
    [&pos] (const auto &scale) { return *scale->pos_ <= pos; });

  return it != scales_.rend () ? (*it) : nullptr;
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
      return co->pos_->frames_ <= local_frames;
    });

  return it != region->chord_objects_.rend () ? (*it) : nullptr;
}

void
ChordTrack::remove_scale (ScaleObject &scale, bool delete_scale)
{
  // Deselect the scale
  scale.select (false, false, false);

  // Find and remove the scale from the vector
  auto it =
    std::find_if (scales_.begin (), scales_.end (), [&scale] (const auto &s) {
      return s == &scale;
    });
  z_return_if_fail (it != scales_.end ());

  scale.index_in_chord_track_ = -1;
  int pos = std::distance (scales_.begin (), it);
  beginRemoveRows ({}, pos, pos);
  (*it)->setParent (nullptr);
  if (delete_scale)
    {
      (*it)->deleteLater ();
    }
  scales_.erase (it);

  // Update indices of remaining scales
  for (size_t i = pos; i < scales_.size (); i++)
    {
      scales_.at (i)->set_index_in_chord_track (static_cast<int> (i));
    }
  endRemoveRows ();

  /* EVENTS_PUSH (
    EventType::ET_ARRANGER_OBJECT_REMOVED, ArrangerObject::Type::ScaleObject); */
}

bool
ChordTrack::validate () const
{
  if (
    !Track::validate_base () || !ChannelTrack::validate_base ()
    || !AutomatableTrack::validate_base ())
    return false;

  bool valid = true;
  foreach_region ([&] (auto &region) {
    bool inner_valid = region.validate (Track::is_in_active_project (), 0);
    if (!inner_valid)
      {
        valid = false;
      }
  });
  z_return_val_if_fail (valid, false);

  for (size_t i = 0; i < scales_.size (); i++)
    {
      auto &m = scales_[i];
      z_return_val_if_fail (
        m->index_in_chord_track_ == static_cast<int> (i), false);
    }

  return true;
}