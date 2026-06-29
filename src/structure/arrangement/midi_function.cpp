// SPDX-FileCopyrightText: © 2020, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/zrythm.h"
#include "structure/arrangement/midi_function.h"
#include "structure/arrangement/midi_note.h"
#include "structure/project/project.h"
#include "utils/enum_utils.h"
#include "utils/rt_thread_id.h"
#include "utils/utf8_string.h"

DEFINE_ENUM_FORMATTER (
  zrythm::structure::arrangement::MidiFunction::Type,
  MidiFunctionType,
  QT_TR_NOOP_UTF8 ("Crescendo"),
  QT_TR_NOOP_UTF8 ("Flam"),
  QT_TR_NOOP_UTF8 ("Flip H"),
  QT_TR_NOOP_UTF8 ("Flip V"),
  QT_TR_NOOP_UTF8 ("Legato"),
  QT_TR_NOOP_UTF8 ("Portato"),
  QT_TR_NOOP_UTF8 ("Staccato"),
  QT_TR_NOOP_UTF8 ("Strum"));

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
MidiFunction::apply (std::span<MidiNote *> sel, Type type, Options opts)
{
  z_debug ("applying {}...", MidiFunctionType_to_string (type));

  auto get_first_and_last_pos = [&] () -> std::pair<double, double> {
    auto ticks =
      sel | std::views::transform ([] (auto * mn) {
        return mn->position ()->ticks ();
      });
    auto [min_it, max_it] = std::ranges::minmax_element (ticks);
    return { *min_it, *max_it };
  };

  switch (type)
    {
    case Type::Crescendo:
      {
        dsp::CurveOptions curve_opts;
        curve_opts.algo_ = opts.curve_algo_;
        curve_opts.curviness_ = opts.curviness_;
        int vel_interval = std::abs (opts.end_vel_ - opts.start_vel_);
        auto [first_pos, last_pos] = get_first_and_last_pos ();

        if (sel.size () == 1)
          {
            sel[0]->setVelocity (opts.start_vel_);
            break;
          }

        double total_ticks = last_pos - first_pos;
        if (total_ticks == 0.0)
          break;
        for (auto * mn : sel)
          {
            double mn_ticks_from_start = mn->position ()->ticks () - first_pos;
            double vel_multiplier = curve_opts.get_normalized_y (
              mn_ticks_from_start / total_ticks,
              opts.start_vel_ > opts.end_vel_);
            mn->setVelocity ((
              midi_byte_t) ((double) std::min (opts.start_vel_, opts.end_vel_)
                            + (vel_interval * vel_multiplier)));
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
            for (auto * mn : sel)
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
                auto * r = mn->get_clip ();
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
        auto pitches = sel | std::views::transform (&MidiNote::pitch);
        auto [min_it, max_it] = std::ranges::minmax_element (pitches);
        int highest_pitch = *max_it;
        int lowest_pitch = *min_it;
        int diff = highest_pitch - lowest_pitch;
        for (auto * mn : sel)
          {
            uint8_t new_val = diff - (mn->pitch () - lowest_pitch);
            new_val += lowest_pitch;
            mn->setPitch (new_val);
          }
      }
      break;
    case Type::FlipHorizontal:
      {
        auto copies = sel | std::ranges::to<std::vector> ();
        std::ranges::sort (copies, {}, [] (auto * mn) {
          return mn->position ()->ticks ();
        });
        std::vector<double> poses;
        for (auto * mn : copies)
          {
            poses.push_back (mn->position ()->ticks ());
          }
        int i = 0;
        for (auto * mn : copies)
          {
            double ticks = mn->length ()->ticks ();
            mn->position ()->setTicks (poses[(copies.size () - i) - 1]);
            mn->length ()->setTicks (ticks);
            ++i;
          }
      }
      break;
    case Type::Legato:
      {
        auto copies = sel | std::ranges::to<std::vector> ();
        std::ranges::sort (copies, {}, [] (auto * mn) {
          return mn->position ()->ticks ();
        });
        for (auto it = copies.begin (); it < (copies.end () - 1); ++it)
          {
            auto * mn = *it;
            auto * next_mn = *(it + 1);
            /* 48 to make sure the note has a length */
            mn->length ()->setTicks (
              std::max (
                48.0,
                next_mn->position ()->ticks () - mn->position ()->ticks ()));
          }
      }
      break;
    case Type::Portato:
      {
        /* do the same as legato but leave some space between the notes */
        auto copies = sel | std::ranges::to<std::vector> ();
        std::ranges::sort (copies, {}, [] (auto * mn) {
          return mn->position ()->ticks ();
        });

        for (auto it = copies.begin (); it < (copies.end () - 1); ++it)
          {
            auto * mn = *it;
            auto * next_mn = *(it + 1);
            mn->length ()->setTicks (
              std::max (
                48.0,
                (next_mn->position ()->ticks () - mn->position ()->ticks ())
                  - 48.0));
          }
      }
      break;
    case Type::Staccato:
      {
        for (auto * mn : sel)
          {
            mn->length ()->setTicks (48.0);
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
            std::ranges::sort (copies, {}, &MidiNote::pitch);
          }
        else
          {
            std::ranges::sort (copies, std::ranges::greater{}, &MidiNote::pitch);
          }
        auto * first_mn = copies.front ();
        for (auto it = copies.begin (); it != copies.end (); ++it)
          {
            auto * mn = *it;
            double ms_multiplier = curve_opts.get_normalized_y (
              (double) std::distance (copies.begin (), it)
                / (double) copies.size (),
              !opts.ascending_);
            double ms_to_add = ms_multiplier * opts.time_;
            z_trace ("multi {:f}, ms {:f}", ms_multiplier, ms_to_add);
            [[maybe_unused]] double len_ticks = mn->length ()->ticks ();
            mn->position ()->setTicks (first_mn->position ()->ticks ());
// TODO
#if 0
            auto tmp_pos = mn->get_position ();
            tmp_pos.add_ms (
              ms_to_add, AUDIO_ENGINE->get_sample_rate (),
              AUDIO_ENGINE->ticks_per_frame_);
            mn->set_position_unvalidated (tmp_pos);
            mn->set_end_position_unvalidated (mn->get_position ());
            mn->end_pos_->add_ticks (len_ticks, AUDIO_ENGINE->frames_per_tick_);
#endif
          }
      }
      break;
    default:
      break;
    }

  /* set last action */
  // TODO
  // gui::SettingsManager::get_instance ()->set_lastMidiFunction (
  // ENUM_VALUE_TO_INT (type));

  // EVENTS_PUSH (EventType::ET_EDITOR_FUNCTION_APPLIED, nullptr);
}
}
