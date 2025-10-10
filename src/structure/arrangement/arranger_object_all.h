// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/audio_source_object.h"
#include "structure/arrangement/automation_region.h"
#include "structure/arrangement/chord_object.h"
#include "structure/arrangement/chord_region.h"
#include "structure/arrangement/marker.h"
#include "structure/arrangement/midi_note.h"
#include "structure/arrangement/midi_region.h"
#include "structure/arrangement/scale_object.h"

namespace zrythm::structure::arrangement
{
/**
 * @brief Fills MIDI event queue from this MIDI or Chord region.
 *
 * The events are dequeued right after the call to this function.
 *
 * @note The caller already splits calls to this function at each
 * sub-loop inside the region, so region loop related logic is not
 * needed.
 *
 * @param note_off_at_end Whether a note off should be added at the
 * end frame (eg, when the caller knows there is a region loop or the
 * region ends).
 * @param is_note_off_for_loop_or_region_end Whether @p
 * note_off_at_end is for a region loop end or the region's end (in
 * this case normal note offs will be sent, otherwise a single ALL
 * NOTES OFF event will be sent).
 * @param midi_events MidiEvents (queued) to fill (from Piano Roll
 * Port for example).
 */
template <typename RegionT>
[[deprecated]] void
fill_midi_events (
  const RegionT               &r,
  const EngineProcessTimeInfo &time_nfo,
  bool                         note_off_at_end,
  bool                         is_note_off_for_loop_or_region_end,
  dsp::MidiEventVector        &midi_events)
  requires std::is_same_v<RegionT, MidiRegion>
           || std::is_same_v<RegionT, ChordRegion>
{
  /**
   * @brief Sends MIDI note off events or an "all notes off" event at
   * the current time.
   *
   * This is called on MIDI or Chord regions.
   *
   * @param is_note_off_for_loop_or_region_end Whether this is called
   * to send note off events for notes at the loop end/region end
   * boundary (as opposed to a transport loop boundary). If true,
   * separate MIDI note off events will be sent for notes at the
   * border. Otherwise, a single all notes off event will be sent.
   */
  const auto send_note_offs =
    [&] (
      dsp::MidiEventVector &midi_events, const EngineProcessTimeInfo time_nfo,
      bool is_note_off_for_loop_or_region_end) {
      midi_byte_t channel = 1;
      if constexpr (std::is_same_v<RegionT, MidiRegion>)
        {
          // TODO: set channel
          // channel = r.get_midi_ch ();
        }
      else if constexpr (std::is_same_v<RegionT, ChordRegion>)
        {
          /* FIXME set channel */
          // auto cr = dynamic_cast<ChordRegion *> (this);
        }

      /* -1 to send event 1 sample before the end point */
      const auto midi_time_for_note_off =
        (midi_time_t) ((time_nfo.local_offset_ + time_nfo.nframes_) - 1);
      const signed_frame_t frame_for_note_off =
        (signed_frame_t) (time_nfo.g_start_frame_w_offset_ + time_nfo.nframes_)
        - 1;
      if (
        is_note_off_for_loop_or_region_end
        && std::is_same_v<RegionT, MidiRegion>)
        {
          if constexpr (std::is_same_v<RegionT, MidiRegion>)
            {
              for (const auto &mn : r.get_children_view ())
                {
                  if (mn->bounds ()->is_hit (frame_for_note_off))
                    {
                      midi_events.add_note_off (
                        channel, mn->pitch (), midi_time_for_note_off);
                    }
                }
            }
        }
      else
        {
          midi_events.add_all_notes_off (channel, midi_time_for_note_off, true);
        }
    };

  /* send note offs if needed */
  if (note_off_at_end)
    {
      send_note_offs (midi_events, time_nfo, is_note_off_for_loop_or_region_end);
    }

  const auto r_local_pos = timeline_frames_to_local (
    r, (signed_frame_t) time_nfo.g_start_frame_w_offset_, true);

  auto process_object = [&]<typename ObjectType> (const ObjectType &obj) {
    if (obj.mute ()->muted ())
      return;

    dsp::ChordDescriptor * descr = nullptr;
    if constexpr (std::is_same_v<ObjectType, ChordObject>)
      {
        // TODO: get actual chord descriptor from this index
        [[maybe_unused]] const auto chord_descr_index =
          obj.chordDescriptorIndex ();
      }

    /* if object starts inside the current range */
    const auto obj_start_pos_frames = obj.position ()->samples ();
    if (
      obj_start_pos_frames >= 0 && obj_start_pos_frames >= r_local_pos
      && obj_start_pos_frames < r_local_pos + (signed_frame_t) time_nfo.nframes_)
      {
        auto _time =
          (midi_time_t) (time_nfo.local_offset_
                         + (obj_start_pos_frames - r_local_pos));

        if constexpr (std::is_same_v<RegionT, MidiRegion>)
          {
            midi_events.add_note_on (
              // TODO
              1,
#if 0
              r->get_midi_ch (),
#endif
              obj.pitch (), obj.velocity (), _time);
          }
        else if constexpr (std::is_same_v<ObjectType, ChordObject>)
          {
            midi_events.add_note_ons_from_chord_descr (
              *descr, 1, MidiNote::DEFAULT_VELOCITY, _time);
          }
      }

    signed_frame_t obj_end_frames = 0;
    if constexpr (std::is_same_v<ObjectType, MidiNote>)
      {
        obj_end_frames = obj.bounds ()->get_end_position_samples (true);
      }
    else if constexpr (std::is_same_v<ObjectType, ChordObject>)
      {
        // TODO: add 1 beat
        // obj_end_frames = obj_start_pos_frames + 1 beat;
        assert (false);
      }

    /* if note ends within the cycle */
    if (
      obj_end_frames >= r_local_pos
      && (obj_end_frames <= (r_local_pos + time_nfo.nframes_)))
      {
        auto _time =
          (midi_time_t) (time_nfo.local_offset_ + (obj_end_frames - r_local_pos));

        /* note actually ends 1 frame before the end point, not at
         * the end point */
        if (_time > 0)
          {
            _time--;
          }

        if constexpr (std::is_same_v<RegionT, MidiRegion>)
          {
            midi_events.add_note_off (
              // TODO
              1,
#if 0
              r->get_midi_ch (),
#endif
              obj.pitch (), _time);
          }
        else if constexpr (std::is_same_v<ObjectType, ChordObject>)
          {
            for (size_t l = 0; l < dsp::ChordDescriptor::MAX_NOTES; l++)
              {
                if (descr->notes_[l])
                  {
                    midi_events.add_note_off (1, l + 36, _time);
                  }
              }
          }
      }
  };

  /* process each object */
  if constexpr (
    std::is_same_v<RegionT, MidiRegion> || std::is_same_v<RegionT, ChordRegion>)
    {
      for (const auto &obj : r.get_children_view ())
        {
          process_object (*obj);
        }
    }
}

/**
 * Returns the number of frames until the next loop end point or the
 * end of the region.
 *
 * @param timeline_frames Global frames at start.
 * @return Number of frames and whether the return frames are for a loop (if
 * false, the return frames are for the region's end).
 */
template <RegionObject ObjectT>
[[gnu::nonnull]] std::pair<signed_frame_t, bool>
get_frames_till_next_loop_or_end (
  const ObjectT &obj,
  signed_frame_t timeline_frames)
{
  const auto * loop_range = obj.loopRange ();
  const auto   loop_size = loop_range->get_loop_length_in_frames ();
  assert (loop_size > 0);
  const auto           object_position_frames = obj.position ()->samples ();
  const signed_frame_t loop_end_frames =
    loop_range->loopEndPosition ()->samples ();
  const signed_frame_t clip_start_frames =
    loop_range->clipStartPosition ()->samples ();

  signed_frame_t local_frames = timeline_frames - object_position_frames;
  local_frames += clip_start_frames;
  while (local_frames >= loop_end_frames)
    {
      local_frames -= loop_size;
    }

  const signed_frame_t frames_till_next_loop = loop_end_frames - local_frames;
  const signed_frame_t frames_till_end =
    obj.bounds ()->get_end_position_samples (true) - timeline_frames;

  return std::make_pair (
    std::min (frames_till_end, frames_till_next_loop),
    frames_till_next_loop < frames_till_end);
}

inline std::pair<double, double>
get_object_tick_range (const ArrangerObject * obj)
{
  return std::make_pair (
    obj->position ()->ticks (),
    obj->position ()->ticks ()
      + (obj->bounds () != nullptr ? obj->bounds ()->length ()->ticks () : 0));
}
}
