// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/midi_event.h"
#include "gui/dsp/midi_note.h"
#include "gui/dsp/piano_roll_track.h"
#include "gui/dsp/velocity.h"

#include "dsp/position.h"
#include <fmt/format.h>

MidiNote::MidiNote (QObject * parent)
    : ArrangerObject (Type::MidiNote), QObject (parent), BoundedObject ()
{
  ArrangerObject::parent_base_qproperties (*this);
  BoundedObject::parent_base_qproperties (*this);
}

MidiNote::MidiNote (
  const RegionIdentifier &region_id,
  Position                start_pos,
  Position                end_pos,
  uint8_t                 val,
  uint8_t                 vel,
  QObject *               parent)
    : ArrangerObject (Type::MidiNote), QObject (parent),
      RegionOwnedObjectImpl (region_id), BoundedObject (),
      vel_ (new Velocity (this, vel)), pitch_ (val)
{
  ArrangerObject::parent_base_qproperties (*this);
  BoundedObject::parent_base_qproperties (*this);
  *static_cast<Position *> (pos_) = start_pos;
  *static_cast<Position *> (end_pos_) = end_pos;
}

void
MidiNote::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
  RegionOwnedObjectImpl::init_loaded_base ();
  vel_->midi_note_ = this;
}

std::string
MidiNote::print_to_str () const
{
  auto r = get_region ();
  auto region = r ? r->get_name () : "unknown";
  auto lane = r ? r->get_lane ()->get_name () : "unknown";
  return fmt::format (
    "[Region '{}' : Lane '{}'] => MidiNote #{}: {} ~ {} | pitch: {} | velocity: {}",
    region, lane, index_, *pos_, *end_pos_, pitch_, vel_->vel_);
}

ArrangerObjectPtrVariant
MidiNote::add_clone_to_project (bool fire_events) const
{
  auto ret = clone_raw_ptr ();
  get_region ()->append_object (ret, true);
  return ret;
}

ArrangerObjectPtrVariant
MidiNote::insert_clone_to_project () const
{
  auto ret = clone_raw_ptr ();
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

  auto track_var = get_track ();
  std::visit (
    [&] (auto &&track) {
      using TrackT = base_type<decltype (track)>;
      if constexpr (std::derived_from<TrackT, PianoRollTrack>)
        {
          auto &events =
            track->processor_->get_midi_in_port ().midi_events_.queued_events_;

          if (listen)
            {
              /* if note is on but pitch changed */
              if (currently_listened_ && pitch_ != last_listened_pitch_)
                {
                  /* create midi note off */
                  events.add_note_off (1, last_listened_pitch_, 0);

                  /* create note on at the new value */
                  events.add_note_on (1, pitch_, vel_->vel_, 0);
                  last_listened_pitch_ = pitch_;
                }
              /* if note is on and pitch is the same */
              else if (currently_listened_ && pitch_ == last_listened_pitch_)
                {
                  /* do nothing */
                }
              /* if note is not on */
              else if (!currently_listened_)
                {
                  /* turn it on */
                  events.add_note_on (1, pitch_, vel_->vel_, 0);
                  last_listened_pitch_ = pitch_;
                  currently_listened_ = 1;
                }
            }
          /* if turning listening off */
          else if (currently_listened_)
            {
              /* create midi note off */
              events.add_note_off (1, last_listened_pitch_, 0);
              currently_listened_ = false;
              last_listened_pitch_ = 255;
            }
        }
    },
    track_var);
}

std::string
MidiNote::get_val_as_string (PianoRoll::NoteNotation notation, bool use_markup)
  const
{
  const auto note_str_musical = dsp::ChordDescriptor::note_to_string (
    ENUM_INT_TO_VALUE (dsp::MusicalNote, pitch_ % 12));
  std::string note_str;
  if (notation == PianoRoll::NoteNotation::Musical)
    {
      note_str = note_str_musical;
    }
  else
    {
      note_str = fmt::format ("{}", pitch_);
    }

  const int note_val = pitch_ / 12 - 1;
  if (use_markup)
    {
      auto buf = fmt::format ("{}<sup>{}</sup>", note_str, note_val);
      if (DEBUGGING)
        {
          buf += fmt::format (" ({})", index_);
        }
      return buf;
    }

  return fmt::format ("{}{}", note_str, note_val);
}

void
MidiNote::set_val (const uint8_t val)
{
  z_return_if_fail (val < 128);

  /* if currently playing set a note off event. */
  if (is_hit (PLAYHEAD.frames_) && TRANSPORT->isRolling ())
    {
      auto region = dynamic_cast<MidiRegion *> (get_region ());
      z_return_if_fail (region);
      auto track_var = region->get_track ();
      std::visit (
        [&] (auto &&track) {
          using TrackT = base_type<decltype (track)>;
          if constexpr (std::derived_from<TrackT, PianoRollTrack>)
            {
              auto &midi_events =
                track->processor_->get_piano_roll_port ()
                  .midi_events_.queued_events_;

              uint8_t midi_ch = region->get_midi_ch ();
              midi_events.add_note_off (midi_ch, pitch_, 0);
            }
        },
        track_var);
    }

  pitch_ = val;
}

void
MidiNote::shift_pitch (const int delta)
{
  pitch_ = (uint8_t) ((int) pitch_ + delta);
  set_val (pitch_);
}

void
MidiNote::init_after_cloning (const MidiNote &other, ObjectCloneType clone_type)

{
  pitch_ = other.pitch_;
  vel_ = new Velocity (this, other.vel_->vel_);
  currently_listened_ = other.currently_listened_;
  last_listened_pitch_ = other.last_listened_pitch_;
  vel_->vel_at_start_ = other.vel_->vel_at_start_;
  BoundedObject::copy_members_from (other, clone_type);
  MuteableObject::copy_members_from (other, clone_type);
  RegionOwnedObject::copy_members_from (other, clone_type);
  ArrangerObject::copy_members_from (other, clone_type);
}

std::optional<ArrangerObjectPtrVariant>
MidiNote::find_in_project () const
{
  auto * r = MidiRegion::find (region_id_);
  z_return_val_if_fail (
    r && static_cast<int> (r->midi_notes_.size ()) > index_, std::nullopt);

  return r->midi_notes_[index_];
}

bool
MidiNote::validate (bool is_project, double frames_per_tick) const
{
  // TODO
  return true;
}
