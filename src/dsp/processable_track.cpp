// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_region.h"
#include "dsp/chord_region.h"
#include "dsp/chord_track.h"
#include "dsp/engine.h"
#include "dsp/laned_track.h"
#include "dsp/processable_track.h"
#include "dsp/track_processor.h"
#include "dsp/transport.h"
#include "project.h"
#include "zrythm.h"

ProcessableTrack::ProcessableTrack ()
{
  processor_ = std::make_unique<TrackProcessor> (this);
}

void
ProcessableTrack::init_loaded ()
{
  processor_->track_ = this;
  processor_->init_loaded (this);
}

void
ProcessableTrack::copy_members_from (const ProcessableTrack &other)
{
  processor_ = other.processor_->clone_unique ();
  processor_->track_ = this;
}

bool
ProcessableTrack::get_monitor_audio () const
{
  z_return_val_if_fail (processor_ && processor_->monitor_audio_, false);

  return processor_->monitor_audio_->is_toggled ();
}

void
ProcessableTrack::
  set_monitor_audio (bool monitor, bool auto_select, bool fire_events)
{
  z_return_if_fail (processor_ && processor_->monitor_audio_);

  if (auto_select)
    {
      select (true, true, fire_events);
    }

  processor_->monitor_audio_->set_toggled (monitor, fire_events);
}

void
ProcessableTrack::fill_midi_events (
  const EngineProcessTimeInfo &time_nfo,
  MidiEventVector             &midi_events)
{
  fill_events_common (time_nfo, &midi_events, nullptr);
}

void
ProcessableTrack::fill_events_common (
  const EngineProcessTimeInfo &time_nfo,
  MidiEventVector *            midi_events,
  StereoPorts *                stereo_ports) const
{
  if (!is_auditioner () && !TRANSPORT->is_rolling ())
    {
      return;
    }

  const unsigned_frame_t g_end_frames =
    time_nfo.g_start_frame_w_offset_ + time_nfo.nframes_;

#if 0
  z_info (
    "%s: TRACK %s STARTING from %ld, "
    "local start frame {}, nframes {}",
    __func__, self->name, time_nfo->g_start_frame_w_offset,
    time_nfo->local_offset, time_nfo->nframes);
#endif

  // Track::Type tt = type_;

  const bool use_caches = !is_auditioner ();

  std::visit (
    [&] (auto &&track) {
      using TrackT = base_type<decltype (track)>;

      auto process_single_region = [&] (const auto &r) {
        using RegionT = base_type<decltype (r)>;

        /* skip region if muted */
        if (r.get_muted (true))
          {
            return;
          }

        /* skip if in bounce mode and the region should not be bounced */
        if (
          AUDIO_ENGINE->bounce_mode_ != BounceMode::BOUNCE_OFF
          && (!r.bounce_ || !bounce_))
          {
            return;
          }

        /* skip if region is not hit (inclusive of its last point) */
        if (
          !r.is_hit_by_range (
            (signed_frame_t) time_nfo.g_start_frame_w_offset_,
            (signed_frame_t) (midi_events ? g_end_frames : (g_end_frames - 1)),
            true))
          {
            return;
          }

        signed_frame_t num_frames_to_process = MIN (
          r.end_pos_.frames_ - (signed_frame_t) time_nfo.g_start_frame_w_offset_,
          (signed_frame_t) time_nfo.nframes_);
        nframes_t frames_processed = 0;

        while (num_frames_to_process > 0)
          {
            unsigned_frame_t cur_g_start_frame =
              time_nfo.g_start_frame_ + frames_processed;
            unsigned_frame_t cur_g_start_frame_w_offset =
              time_nfo.g_start_frame_w_offset_ + frames_processed;
            nframes_t cur_local_start_frame =
              time_nfo.local_offset_ + frames_processed;

            bool           is_loop_end;
            signed_frame_t cur_num_frames_till_next_r_loop_or_end;
            r.get_frames_till_next_loop_or_end (
              (signed_frame_t) cur_g_start_frame_w_offset,
              &cur_num_frames_till_next_r_loop_or_end, &is_loop_end);

#if 0
              z_info (
                "%s: cur num frames till next r "
                "loop or end %ld, "
                "num_frames_to_process %ld, "
                "cur local start frame {}",
                __func__, cur_num_frames_till_next_r_loop_or_end,
                num_frames_to_process,
                cur_local_start_frame);
#endif

            const bool is_region_end =
              (signed_frame_t) time_nfo.g_start_frame_w_offset_
                + num_frames_to_process
              == r.end_pos_.frames_;

            const bool is_transport_end =
              TRANSPORT->is_looping ()
              && (signed_frame_t) time_nfo.g_start_frame_w_offset_
                     + num_frames_to_process
                   == TRANSPORT->loop_end_pos_.frames_;

            /* whether we need a note off */
            const bool need_note_off =
              (cur_num_frames_till_next_r_loop_or_end < num_frames_to_process)
              || (cur_num_frames_till_next_r_loop_or_end == num_frames_to_process && !is_loop_end)
              /* region end */
              || is_region_end
              /* transport end */
              || is_transport_end;

            /* number of frames to process this time */
            cur_num_frames_till_next_r_loop_or_end = MIN (
              cur_num_frames_till_next_r_loop_or_end, num_frames_to_process);

            const EngineProcessTimeInfo nfo = {
              .g_start_frame_ = cur_g_start_frame,
              .g_start_frame_w_offset_ = cur_g_start_frame_w_offset,
              .local_offset_ = cur_local_start_frame,
              .nframes_ =
                static_cast<nframes_t> (cur_num_frames_till_next_r_loop_or_end)
            };

            if constexpr (RegionTypeWithMidiEvents<RegionT>)
              {
                z_return_if_fail (midi_events);
                r.fill_midi_events (
                  nfo, need_note_off,
                  !is_transport_end && (is_loop_end || is_region_end),
                  *midi_events);
              }
            else if constexpr (std::is_same_v<RegionT, AudioRegion>)
              {
                z_return_if_fail (stereo_ports);
                r.fill_stereo_ports (nfo, *stereo_ports);
              }
            else
              {
                static_assert (false, "Unknown region type");
              }

            frames_processed += cur_num_frames_till_next_r_loop_or_end;
            num_frames_to_process -= cur_num_frames_till_next_r_loop_or_end;
          } /* end while frames left */
      };

      /* go through each region collection (either each lane or the chord track)
       */
      if constexpr (std::is_same_v<TrackT, ChordTrack>)
        {
          if (use_caches)
            {
              for (auto &region : track->region_snapshots_)
                {
                  process_single_region (*region);
                }
            }
          else
            {
              for (auto &region : track->regions_)
                {
                  process_single_region (*region);
                }
            }
        }
      else if constexpr (std::derived_from<TrackT, LanedTrack>)
        {
          if (use_caches)
            {
              for (auto &lane : track->lane_snapshots_)
                {
                  for (auto &region : lane->region_snapshots_)
                    {
                      process_single_region (*region);
                    }
                }
            }
          else
            {
              for (auto &lane : track->lanes_)
                {
                  for (auto &region : lane->regions_)
                    {
                      process_single_region (*region);
                    }
                }
            }
        }
      else
        {
          /* unsupported track type */
          z_return_if_reached ();
        }

      if (midi_events)
        {
          midi_events->clear_duplicates ();

          /* sort events */
          midi_events->sort ();

#if 0
      if (midi_events_has_any (midi_events, F_QUEUED))
        {
          engine_process_time_info_print (time_nfo);
          midi_events_print (midi_events, F_QUEUED);
        }
#endif
        }
    },
    convert_to_variant<ProcessableTrackPtrVariant> (this));
}