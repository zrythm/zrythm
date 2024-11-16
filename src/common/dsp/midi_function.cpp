// SPDX-FileCopyrightText: © 2020, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/midi_function.h"
#include "common/utils/flags.h"
#include "common/utils/rt_thread_id.h"
#include "common/utils/string.h"
#include "gui/backend/backend/arranger_selections.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"

using namespace zrythm;

/**
 * Returns a string identifier for the type.
 */
std::string
midi_function_type_to_string_id (MidiFunctionType type)
{
  const auto  type_str = MidiFunctionType_to_string (type);
  std::string type_str_lower (type_str);
  string_to_lower_ascii (type_str_lower);
  auto substituted = string_replace (type_str_lower, " ", "-");

  return substituted;
}

/**
 * Returns a string identifier for the type.
 */
MidiFunctionType
midi_function_string_id_to_type (const char * id)
{
  for (
    size_t i = ENUM_VALUE_TO_INT (MidiFunctionType::Crescendo);
    i <= ENUM_VALUE_TO_INT (MidiFunctionType::Strum); i++)
    {
      MidiFunctionType cur = ENUM_INT_TO_VALUE (MidiFunctionType, i);
      auto             str = midi_function_type_to_string_id (cur);
      bool             eq = str == id;
      if (eq)
        return cur;
    }
  z_return_val_if_reached (MidiFunctionType::Crescendo);
}

void
midi_function_apply (
  MidiSelections  &sel,
  MidiFunctionType type,
  MidiFunctionOpts opts)
{
  /* TODO */
  z_debug ("applying {}...", MidiFunctionType_to_string (type));

  switch (type)
    {
    case MidiFunctionType::Crescendo:
      {
        CurveOptions curve_opts;
        curve_opts.algo_ = opts.curve_algo_;
        curve_opts.curviness_ = opts.curviness_;
        int vel_interval = std::abs (opts.end_vel_ - opts.start_vel_);
        auto [first_note, first_pos] =
          sel.get_first_object_and_pos<MidiNote> (false);
        auto [last_note, last_pos] =
          sel.get_last_object_and_pos<MidiNote> (false, false);
        z_return_if_fail (first_note && last_note);
        if (first_note == last_note)
          {
            first_note->vel_->vel_ = opts.start_vel_;
            break;
          }

        double total_ticks = last_pos.ticks_ - first_pos.ticks_;
        for (auto mn : sel.objects_ | type_is<MidiNote> ())
          {
            double mn_ticks_from_start = mn->pos_->ticks_ - first_pos.ticks_;
            double vel_multiplier = curve_opts.get_normalized_y (
              mn_ticks_from_start / total_ticks,
              opts.start_vel_ > opts.end_vel_);
            mn->vel_->vel_ =
              (midi_byte_t) ((double) std::min (opts.start_vel_, opts.end_vel_)
                             + (vel_interval * vel_multiplier));
          }
      }
      break;
    case MidiFunctionType::Flam:
      {
        /* currently MIDI functions assume no new notes are
         * added so currently disabled */
        break;
        std::vector<MidiNote *> new_midi_notes;
        for (auto mn : sel.objects_ | type_is<MidiNote> ())
          {
            double len = mn->get_length_in_ticks ();
            auto   new_mn = mn->clone_raw_ptr ();
            new_midi_notes.push_back (new_mn);
            double opt_ticks = Position::ms_to_ticks ((signed_ms_t) opts.time_);
            new_mn->move (opt_ticks);
            if (opts.time_ >= 0)
              {
                /* make new note as long as existing note was and make existing
                 * note up to the new note */
                new_mn->end_pos_->add_ticks (len - opt_ticks);
                mn->end_pos_->add_ticks (
                  (-(new_mn->end_pos_->ticks_ - new_mn->pos_->ticks_)) + 1);
              }
            else
              {
                /* make new note up to the existing note */
                new_mn->end_pos_ = new_mn->pos_;
                new_mn->end_pos_->add_ticks (
                  ((mn->end_pos_->ticks_ - mn->pos_->ticks_) - opt_ticks) - 1);
              }
          }
        for (auto &mn : new_midi_notes)
          {
            auto r = MidiRegion::find (mn->region_id_);
            r->append_object (mn, false);
            /* clone again because the selections are supposed to hold clones */
            sel.add_object_owned (mn->clone_unique ());
          }
      }
      break;
    case MidiFunctionType::FlipVertical:
      {
        auto highest_note = sel.get_highest_note ();
        auto lowest_note = sel.get_lowest_note ();
        z_return_if_fail (highest_note && lowest_note);
        int highest_pitch = highest_note->val_;
        int lowest_pitch = lowest_note->val_;
        int diff = highest_pitch - lowest_pitch;
        for (auto mn : sel.objects_ | type_is<MidiNote> ())
          {
            uint8_t new_val = diff - (mn->val_ - lowest_pitch);
            new_val += lowest_pitch;
            mn->val_ = new_val;
          }
      }
      break;
    case MidiFunctionType::FlipHorizontal:
      {
        sel.sort_by_positions (false);
        std::vector<Position> poses;
        for (auto mn : sel.objects_ | type_is<MidiNote> ())
          {
            poses.push_back (*mn->pos_);
          }
        int i = 0;
        for (auto mn : sel.objects_ | type_is<MidiNote> ())
          {
            double ticks = mn->get_length_in_ticks ();
            *static_cast<Position *> (mn->pos_) =
              poses[(sel.objects_.size () - i) - 1];
            *static_cast<Position *> (mn->end_pos_) =
              *static_cast<Position *> (mn->pos_);
            mn->end_pos_->add_ticks (ticks);
            ++i;
          }
      }
      break;
    case MidiFunctionType::Legato:
      {
        sel.sort_by_positions (false);
        for (
          auto it = sel.objects_.begin (); it < (sel.objects_.end () - 1); ++it)
          {
            auto mn = std::get<MidiNote *> (*it);
            auto next_mn = std::get<MidiNote *> (*(it + 1));
            mn->end_pos_ = next_mn->pos_;
            /* make sure the note has a length */
            if (mn->end_pos_->ticks_ - mn->pos_->ticks_ < 1.0)
              {
                mn->end_pos_->add_ms (40.0);
              }
          }
      }
      break;
    case MidiFunctionType::Portato:
      {
        /* do the same as legato but leave some space between the notes */
        sel.sort_by_positions (false);

        for (
          auto it = sel.objects_.begin (); it < (sel.objects_.end () - 1); ++it)
          {
            auto mn = std::get<MidiNote *> (*it);
            auto next_mn = std::get<MidiNote *> (*(it + 1));
            mn->end_pos_ = next_mn->pos_;
            mn->end_pos_->add_ms (-80.0);
            /* make sure the note has a length */
            if (mn->end_pos_->ticks_ - mn->pos_->ticks_ < 1.0)
              {
                *static_cast<Position *> (mn->end_pos_) =
                  *static_cast<Position *> (next_mn->pos_);
                mn->end_pos_->add_ms (40.0);
              }
          }
      }
      break;
    case MidiFunctionType::Staccato:
      {
        for (
          auto it = sel.objects_.begin (); it < (sel.objects_.end () - 1); ++it)
          {
            auto mn = std::get<MidiNote *> (*it);
            *static_cast<Position *> (mn->end_pos_) =
              *static_cast<Position *> (mn->pos_);
            mn->end_pos_->add_ms (140.0);
          }
      }
      break;
    case MidiFunctionType::Strum:
      {
        CurveOptions curve_opts;
        curve_opts.algo_ = opts.curve_algo_;
        curve_opts.curviness_ = opts.curviness_;

        sel.sort_by_pitch (!opts.ascending_);
        const auto * first_mn = std::get<MidiNote *> (sel.objects_.front ());
        for (auto it = sel.objects_.begin (); it != sel.objects_.end (); ++it)
          {
            auto * mn = std::get<MidiNote *> (*it);
            double ms_multiplier = curve_opts.get_normalized_y (
              (double) std::distance (sel.objects_.begin (), it)
                / (double) sel.get_num_objects (),
              !opts.ascending_);
            double ms_to_add = ms_multiplier * opts.time_;
            z_trace ("multi {:f}, ms {:f}", ms_multiplier, ms_to_add);
            double len_ticks = mn->get_length_in_ticks ();
            mn->pos_ =
              first_mn->pos_; // FIXME!!!!!!! setting pointers instead of
                              // assigning position !!!!! fix others above too
            mn->pos_->add_ms (ms_to_add);
            mn->end_pos_ = mn->pos_;
            mn->end_pos_->add_ticks (len_ticks);
          }
      }
      break;
    default:
      break;
    }

  /* set last action */
  if (ZRYTHM_HAVE_UI)
    {
      gui::SettingsManager::get_instance ()->set_lastMidiFunction (
        ENUM_VALUE_TO_INT (type));
    }

  // EVENTS_PUSH (EventType::ET_EDITOR_FUNCTION_APPLIED, nullptr);
}
