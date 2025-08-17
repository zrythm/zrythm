// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2022 Robert Panovics <robert.panovics at gmail dot com>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "dsp/midi_event.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/midi_region.h"

namespace zrythm::structure::arrangement
{
MidiRegion::MidiRegion (
  const dsp::TempoMap          &tempo_map,
  ArrangerObjectRegistry       &object_registry,
  dsp::FileAudioSourceRegistry &file_audio_source_registry,
  QObject *                     parent)
    : ArrangerObject (Type::MidiRegion, tempo_map, parent),
      ArrangerObjectOwner (object_registry, file_audio_source_registry, *this),
      region_mixin_ (utils::make_qobject_unique<RegionMixin> (*position ()))
{
}

void
init_from (
  MidiRegion            &obj,
  const MidiRegion      &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
  init_from (*obj.region_mixin_, *other.region_mixin_, clone_type);
  init_from (
    static_cast<ArrangerObjectOwner<MidiNote> &> (obj),
    static_cast<const ArrangerObjectOwner<MidiNote> &> (other), clone_type);
}

void
MidiRegion::add_midi_region_events (
  dsp::MidiEventVector &events,
  std::optional<double> start,
  std::optional<double> end,
  const bool            add_region_start,
  const bool            full) const
{
  double region_start = 0;
  if (add_region_start)
    region_start = position ()->ticks ();

  const double loop_length_in_ticks =
    regionMixin ()->loopRange ()->get_loop_length_in_ticks ();
  const auto &length = regionMixin ()->bounds ()->length ();
  const auto &loop_start_pos =
    regionMixin ()->loopRange ()->loopStartPosition ();
  const auto &loop_end_pos = regionMixin ()->loopRange ()->loopEndPosition ();
  const auto &clip_start_pos =
    regionMixin ()->loopRange ()->clipStartPosition ();
  int number_of_loop_repeats = (int) std::ceil (
    (length->ticks () - loop_start_pos->ticks () + clip_start_pos->ticks ())
    / loop_length_in_ticks);

  for (const auto * mn : get_children_view ())
    {
      /**
       * Returns whether the given note is not muted and starts within any
       * playable part of the region.
       */
      const auto is_note_playable = [&] (const MidiNote &midi_note) {
        if (midi_note.mute ()->muted ())
          {
            return false;
          }

        const auto note_start_pos_samples = midi_note.position ()->samples ();
        const auto loop_start_pos_samples = loop_start_pos->samples ();
        const auto loop_end_pos_samples = loop_end_pos->samples ();
        const auto clip_start_pos_samples = clip_start_pos->samples ();
        if (
          // note not in loop range
          (note_start_pos_samples < loop_start_pos_samples
           || note_start_pos_samples >= loop_end_pos_samples)
          &&
          // note not between clip start and loop start
          (note_start_pos_samples < clip_start_pos_samples
           || note_start_pos_samples >= loop_start_pos_samples))
          {
            return false;
          }

        return true;
      };

      if (full && !is_note_playable (*mn))
        {
          continue;
        }

      int  repeat_counter = 0;
      bool write_only_once = true;

      do
        {
          auto       mn_pos_ticks = mn->position ()->ticks ();
          const auto mn_length_ticks = mn->bounds ()->length ()->ticks ();
          auto       mn_end_pos_ticks = mn_pos_ticks + mn_length_ticks;

          if (full)
            {
              if (
                mn_pos_ticks >= loop_start_pos->ticks ()
                && mn_pos_ticks < loop_end_pos->ticks ())
                {
                  write_only_once = false;
                }

              /**
               * Set positions to the exact values in the export region as it is
               * played inside Zrythm.
               *
               * @param[in,out] start_pos start position of the event in ticks
               * @param[in,out] end_pos end position of the event
               * @param repeat_index repetition counter for loop offset
               */
              const auto get_note_positions_in_export =
                [this, loop_length_in_ticks] (
                  double &start_pos, double &end_pos, int repeat_index) {
                  const auto * loop_range = regionMixin ()->loopRange ();
                  double       export_start_pos{};
                  double       export_end_pos{
                    regionMixin ()->bounds ()->length ()->ticks ()
                  };

                  end_pos = std::min (
                    loop_range->loopEndPosition ()->ticks (), end_pos);

                  if (start_pos < loop_range->clipStartPosition ()->ticks ())
                    {
                      ++repeat_index;
                    }

                  start_pos +=
                    (loop_length_in_ticks * repeat_index
                     - loop_range->clipStartPosition ()->ticks ());
                  end_pos +=
                    (loop_length_in_ticks * repeat_index
                     - loop_range->clipStartPosition ()->ticks ());
                  start_pos = std::max (start_pos, export_start_pos);
                  end_pos = std::min (end_pos, export_end_pos);
                };

              get_note_positions_in_export (
                mn_pos_ticks, mn_end_pos_ticks, repeat_counter);

              // if adjusted note start position is outside the region bounds,
              // skip
              if (
                mn_pos_ticks < 0.0
                || mn_pos_ticks >= regionMixin ()->bounds ()->length ()->ticks ())
                {
                  continue;
                }
            }

          double note_global_start_ticks = mn_pos_ticks + region_start;
          double note_global_end_ticks = mn_end_pos_ticks + region_start;
          bool   write_note = true;
          if (start && note_global_end_ticks < *start)
            write_note = false;
          if (end && note_global_start_ticks > *end)
            write_note = false;

          if (write_note)
            {
              if (start)
                {
                  note_global_start_ticks -= *start;
                  note_global_end_ticks -= *start;
                }
              events.add_note_on (
                1, mn->pitch (), mn->velocity (),
                (midi_time_t) (note_global_start_ticks));
              events.add_note_off (
                1, mn->pitch (), (midi_time_t) (note_global_end_ticks));
            }
        }
      while (++repeat_counter < number_of_loop_repeats && !write_only_once);
    }

  events.sort ();
}
}
