// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/midi_event.h"
#include "common/dsp/midi_note.h"
#include "common/dsp/piano_roll_track.h"
#include "common/dsp/position.h"
#include "common/dsp/velocity.h"
#include "gui/backend/backend/midi_selections.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/gtk_widgets/arranger.h"
#include "gui/backend/gtk_widgets/arranger_wrapper.h"
#include "gui/backend/gtk_widgets/bot_dock_edge.h"
#include "gui/backend/gtk_widgets/center_dock.h"
#include "gui/backend/gtk_widgets/clip_editor.h"
#include "gui/backend/gtk_widgets/clip_editor_inner.h"
#include "gui/backend/gtk_widgets/main_window.h"
#include "gui/backend/gtk_widgets/midi_arranger.h"
#include "gui/backend/gtk_widgets/midi_editor_space.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <fmt/format.h>

MidiNote::MidiNote () : ArrangerObject (Type::MidiNote) {};

MidiNote::MidiNote (
  const RegionIdentifier &region_id,
  Position                start_pos,
  Position                end_pos,
  uint8_t                 val,
  uint8_t                 vel)
    : ArrangerObject (Type::MidiNote), RegionOwnedObjectImpl (region_id),
      vel_ (std::make_shared<Velocity> (this, vel)), val_ (val)
{
  pos_ = start_pos;
  end_pos_ = end_pos;
}

void
MidiNote::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
  RegionOwnedObjectImpl::init_loaded_base ();
  vel_->midi_note_ = this;
  vel_->init_loaded ();
}

std::string
MidiNote::print_to_str () const
{
  auto r = get_region ();
  auto region = r ? r->get_name () : "unknown";
  auto lane = r ? r->get_lane ()->get_name () : "unknown";
  return fmt::format (
    "[Region '{}' : Lane '{}'] => MidiNote #{}: {} ~ {} | pitch: {} | velocity: {}",
    region, lane, index_, pos_.to_string (), end_pos_.to_string (), val_,
    vel_->vel_);
}

MidiNote::ArrangerObjectPtr
MidiNote::add_clone_to_project (bool fire_events) const
{
  auto ret = clone_shared ();
  get_region ()->append_object (ret, true);
  return ret;
}

MidiNote::ArrangerObjectPtr
MidiNote::insert_clone_to_project () const
{
  auto ret = clone_shared ();
  get_region ()->insert_object (ret, index_, true);
  return ret;
}

std::string
MidiNote::gen_human_friendly_name () const
{
  return get_val_as_string (PianoRoll::NoteNotation::Musical, false);
}

void
MidiNote::listen (bool listen)
{
  /*z_info (*/
  /*"%s: %" PRIu8 " listen %d", __func__,*/
  /*mn->val, listen);*/

  auto track = dynamic_cast<PianoRollTrack *> (get_track ());
  z_return_if_fail (track && track->processor_->midi_in_);
  auto &events = track->processor_->midi_in_->midi_events_.queued_events_;

  if (listen)
    {
      /* if note is on but pitch changed */
      if (currently_listened_ && val_ != last_listened_val_)
        {
          /* create midi note off */
          events.add_note_off (1, last_listened_val_, 0);

          /* create note on at the new value */
          events.add_note_on (1, val_, vel_->vel_, 0);
          last_listened_val_ = val_;
        }
      /* if note is on and pitch is the same */
      else if (currently_listened_ && val_ == last_listened_val_)
        {
          /* do nothing */
        }
      /* if note is not on */
      else if (!currently_listened_)
        {
          /* turn it on */
          events.add_note_on (1, val_, vel_->vel_, 0);
          last_listened_val_ = val_;
          currently_listened_ = 1;
        }
    }
  /* if turning listening off */
  else if (currently_listened_)
    {
      /* create midi note off */
      events.add_note_off (1, last_listened_val_, 0);
      currently_listened_ = false;
      last_listened_val_ = 255;
    }
}

std::string
MidiNote::get_val_as_string (PianoRoll::NoteNotation notation, bool use_markup)
  const
{
  const char * note_str_musical = ChordDescriptor::note_to_string (
    ENUM_INT_TO_VALUE (MusicalNote, val_ % 12));
  char note_str[20];
  if (notation == PianoRoll::NoteNotation::Musical)
    {
      strcpy (note_str, note_str_musical);
    }
  else
    {
      sprintf (note_str, "%d", val_);
    }

  const int note_val = val_ / 12 - 1;
  if (use_markup)
    {
      auto buf = fmt::format ("{}<sup>{}</sup>", note_str, note_val);
      if (DEBUGGING)
        {
          buf += fmt::format (" ({})", index_);
        }
      return buf;
    }
  else
    {
      return fmt::format ("{}{}", note_str, note_val);
    }
}

void
MidiNote::set_val (const uint8_t val)
{
  z_return_if_fail (val < 128);

  /* if currently playing set a note off event. */
  if (is_hit (PLAYHEAD.frames_) && TRANSPORT->is_rolling ())
    {
      auto region = dynamic_cast<MidiRegion *> (get_region ());
      z_return_if_fail (region);
      auto track = dynamic_cast<PianoRollTrack *> (region->get_track ());
      z_return_if_fail (track);

      auto &midi_events =
        track->processor_->piano_roll_->midi_events_.queued_events_;

      uint8_t midi_ch = region->get_midi_ch ();
      midi_events.add_note_off (midi_ch, val_, 0);
    }

  val_ = val;
}

void
MidiNote::shift_pitch (const int delta)
{
  val_ = (uint8_t) ((int) val_ + delta);
  set_val (val_);
}

void
MidiNote::init_after_cloning (const MidiNote &other)
{
  val_ = other.val_;
  vel_ = std::make_shared<Velocity> (this, other.vel_->vel_);
  currently_listened_ = other.currently_listened_;
  last_listened_val_ = other.last_listened_val_;
  vel_->vel_at_start_ = other.vel_->vel_at_start_;
  LengthableObject::copy_members_from (other);
  MuteableObject::copy_members_from (other);
  RegionOwnedObject::copy_members_from (other);
  ArrangerObject::copy_members_from (other);
}

bool
MidiNote::is_hit (const signed_frame_t gframes) const
{
  auto region = dynamic_cast<MidiRegion *> (get_region ());

  /* get local positions */
  signed_frame_t local_pos = region->timeline_frames_to_local (gframes, true);

  /* add clip_start position to start from there */
  signed_frame_t clip_start_frames = region->clip_start_pos_.frames_;
  local_pos += clip_start_frames;

  /* check for note on event on the boundary */
  /* FIXME ok? it was < and >= before */
  if (pos_.frames_ <= local_pos && end_pos_.frames_ > local_pos)
    return true;

  return false;
}

ArrangerWidget *
MidiNote::get_arranger () const
{
  return (ArrangerWidget *) (MW_MIDI_ARRANGER);
}

MidiNote::ArrangerObjectPtr
MidiNote::find_in_project () const
{
  auto r = MidiRegion::find (region_id_);
  z_return_val_if_fail (
    r && static_cast<int> (r->midi_notes_.size ()) > index_, nullptr);

  return r->midi_notes_[index_];
}

bool
MidiNote::validate (bool is_project, double frames_per_tick) const
{
  // TODO
  return true;
}