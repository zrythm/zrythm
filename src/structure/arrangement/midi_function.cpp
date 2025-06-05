// SPDX-FileCopyrightText: © 2020, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/arrangement/midi_function.h"
#include "utils/rt_thread_id.h"
#include "utils/string.h"

namespace zrythm::structure::arrangement
{

/**
 * Returns a string identifier for the type.
 */
utils::Utf8String
MidiFunction::type_to_string_id (Type type)
{
  return MidiFunctionType_to_string (type).to_lower ().replace (u8" ", u8"-");
}

/**
 * Returns a string identifier for the type.
 */
auto
MidiFunction::string_id_to_type (const char * id) -> Type
{
  for (
    size_t i = ENUM_VALUE_TO_INT (Type::Crescendo);
    i <= ENUM_VALUE_TO_INT (Type::Strum); i++)
    {
      Type cur = ENUM_INT_TO_VALUE (Type, i);
      auto str = type_to_string_id (cur);
      bool eq = str == id;
      if (eq)
        return cur;
    }
  z_return_val_if_reached (Type::Crescendo);
}

void
MidiFunction::apply (ArrangerObjectSpan sel_var, Type type, Options opts)
{
  using Position = dsp::Position;
  /* TODO */
  z_debug ("applying {}...", MidiFunctionType_to_string (type));

  const auto &sel = sel_var;
  switch (type)
    {
    case Type::Crescendo:
      {
        dsp::CurveOptions curve_opts;
        curve_opts.algo_ = opts.curve_algo_;
        curve_opts.curviness_ = opts.curviness_;
        int vel_interval = std::abs (opts.end_vel_ - opts.start_vel_);
        auto [first_note_var, first_pos] = sel.get_first_object_and_pos (false);
        auto [last_note_var, last_pos] =
          sel.get_last_object_and_pos (false, false);
        auto first_note = std::get<MidiNote *> (first_note_var);
        auto last_note = std::get<MidiNote *> (last_note_var);

        if (first_note == last_note)
          {
            first_note->vel_->vel_ = opts.start_vel_;
            break;
          }

        double total_ticks = last_pos.ticks_ - first_pos.ticks_;
        for (auto * mn : sel.template get_elements_by_type<MidiNote> ())
          {
            double mn_ticks_from_start =
              mn->get_position ().ticks_ - first_pos.ticks_;
            double vel_multiplier = curve_opts.get_normalized_y (
              mn_ticks_from_start / total_ticks,
              opts.start_vel_ > opts.end_vel_);
            mn->vel_->vel_ =
              (midi_byte_t) ((double) std::min (opts.start_vel_, opts.end_vel_)
                             + (vel_interval * vel_multiplier));
          }
      }
      break;
    case Type::Flam:
      {
        /* currently MIDI functions assume no new notes are
         * added so currently disabled */
        break;
#if 0
            std::vector<MidiNote *> new_midi_notes;
            for (auto * mn : sel.template get_elements_by_type<MidiNote> ())
              {
                double len = mn->get_length_in_ticks ();
                auto   new_mn = mn->clone_raw_ptr ();
                new_midi_notes.push_back (new_mn);
                double opt_ticks = Position::ms_to_ticks (
                  (signed_ms_t) opts.time_, AUDIO_ENGINE->sample_rate_,
                  AUDIO_ENGINE->ticks_per_frame_);
                new_mn->move (opt_ticks);
                if (opts.time_ >= 0)
                  {
                    /* make new note as long as existing note was and make
                     * existing note up to the new note */
                    new_mn->end_pos_->add_ticks (
                      len - opt_ticks, AUDIO_ENGINE->frames_per_tick_);
                    mn->end_pos_->add_ticks (
                      (-(new_mn->end_pos_->ticks_ - new_mn->pos_->ticks_)) + 1,
                      AUDIO_ENGINE->frames_per_tick_);
                  }
                else
                  {
                    /* make new note up to the existing note */
                    new_mn->end_pos_ = new_mn->pos_;
                    new_mn->end_pos_->add_ticks (
                      ((mn->end_pos_->ticks_ - mn->pos_->ticks_) - opt_ticks) - 1,
                      AUDIO_ENGINE->frames_per_tick_);
                  }
              }
            for (auto &mn : new_midi_notes)
              {
                auto * r = mn->get_region ();
                r->append_object (mn->get_uuid ());
                /* clone again because the selections are supposed to hold
                 * clones */
                // FIXME:
                // sel.add_object_owned (mn->clone_unique ());
              }
#endif
      }
      break;
    case Type::FlipVertical:
      {
        auto [lowest_note, highest_note] = sel.get_first_and_last_note ();
        z_return_if_fail (highest_note && lowest_note);
        int highest_pitch = highest_note->pitch_;
        int lowest_pitch = lowest_note->pitch_;
        int diff = highest_pitch - lowest_pitch;
        for (auto * mn : sel.template get_elements_by_type<MidiNote> ())
          {
            uint8_t new_val = diff - (mn->pitch_ - lowest_pitch);
            new_val += lowest_pitch;
            mn->pitch_ = new_val;
          }
      }
      break;
    case Type::FlipHorizontal:
      {
        auto copies = sel | std::ranges::to<std::vector> ();
        std::ranges::sort (copies, {}, ArrangerObjectSpan::position_projection);
        std::vector<Position> poses;
        for (
          auto * mn :
          ArrangerObjectSpan{ copies }.template get_elements_by_type<MidiNote> ())
          {
            poses.push_back (mn->get_position ());
          }
        int i = 0;
        for (
          auto * mn :
          ArrangerObjectSpan{ copies }.template get_elements_by_type<MidiNote> ())
          {
            double ticks = mn->get_length_in_ticks ();
            mn->set_position_unvalidated (poses[(copies.size () - i) - 1]);
            mn->set_end_position_unvalidated (mn->get_position ());
            mn->end_pos_->add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
            ++i;
          }
      }
      break;
    case Type::Legato:
      {
        auto copies = sel | std::ranges::to<std::vector> ();
        std::ranges::sort (copies, {}, ArrangerObjectSpan::position_projection);
        for (auto it = copies.begin (); it < (copies.end () - 1); ++it)
          {
            auto mn = std::get<MidiNote *> (*it);
            auto next_mn = std::get<MidiNote *> (*(it + 1));
            mn->set_end_position_unvalidated (next_mn->get_position ());
            /* make sure the note has a length */
            if (
              mn->get_end_position ().ticks_ - mn->get_position ().ticks_ < 1.0)
              {
                mn->end_pos_->add_ms (
                  40.0, AUDIO_ENGINE->get_sample_rate (),
                  AUDIO_ENGINE->ticks_per_frame_);
              }
          }
      }
      break;
    case Type::Portato:
      {
        /* do the same as legato but leave some space between the notes */
        auto copies = sel | std::ranges::to<std::vector> ();
        std::ranges::sort (copies, {}, ArrangerObjectSpan::position_projection);

        for (auto it = copies.begin (); it < (copies.end () - 1); ++it)
          {
            auto mn = std::get<MidiNote *> (*it);
            auto next_mn = std::get<MidiNote *> (*(it + 1));
            mn->set_end_position_unvalidated (next_mn->get_position ());
            mn->end_pos_->add_ms (
              -80.0, AUDIO_ENGINE->get_sample_rate (),
              AUDIO_ENGINE->ticks_per_frame_);
            /* make sure the note has a length */
            if (
              mn->get_end_position ().ticks_ - mn->get_position ().ticks_ < 1.0)
              {
                mn->set_end_position_unvalidated (next_mn->get_position ());
                mn->end_pos_->add_ms (
                  40.0, AUDIO_ENGINE->get_sample_rate (),
                  AUDIO_ENGINE->ticks_per_frame_);
              }
          }
      }
      break;
    case Type::Staccato:
      {
        for (auto it = sel.begin (); it < (sel.end () - 1); ++it)
          {
            auto mn = std::get<MidiNote *> (*it);
            mn->set_end_position_unvalidated (mn->get_position ());
            mn->end_pos_->add_ms (
              140.0, AUDIO_ENGINE->get_sample_rate (),
              AUDIO_ENGINE->ticks_per_frame_);
          }
      }
      break;
    case Type::Strum:
      {
        dsp::CurveOptions curve_opts;
        curve_opts.algo_ = opts.curve_algo_;
        curve_opts.curviness_ = opts.curviness_;

        auto copies = sel | std::ranges::to<std::vector> ();
        if (opts.ascending_)
          {
            std::ranges::sort (
              copies, {}, ArrangerObjectSpan::midi_note_pitch_projection);
          }
        else
          {
            std::ranges::sort (
              copies, std::ranges::greater{},
              ArrangerObjectSpan::midi_note_pitch_projection);
          }
        const auto * first_mn = std::get<MidiNote *> (copies.front ());
        for (auto it = copies.begin (); it != copies.end (); ++it)
          {
            auto * mn = std::get<MidiNote *> (*it);
            double ms_multiplier = curve_opts.get_normalized_y (
              (double) std::distance (copies.begin (), it)
                / (double) copies.size (),
              !opts.ascending_);
            double ms_to_add = ms_multiplier * opts.time_;
            z_trace ("multi {:f}, ms {:f}", ms_multiplier, ms_to_add);
            double len_ticks = mn->get_length_in_ticks ();
            mn->set_position_unvalidated (first_mn->get_position ());
            auto tmp_pos = mn->get_position ();
            tmp_pos.add_ms (
              ms_to_add, AUDIO_ENGINE->get_sample_rate (),
              AUDIO_ENGINE->ticks_per_frame_);
            mn->set_position_unvalidated (tmp_pos);
            mn->set_end_position_unvalidated (mn->get_position ());
            mn->end_pos_->add_ticks (len_ticks, AUDIO_ENGINE->frames_per_tick_);
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
}
