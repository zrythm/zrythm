// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "engine/device_io/engine.h"
#include "engine/session/transport.h"
#include "gui/backend/backend/project.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/chord_region.h"
#include "structure/tracks/chord_track.h"
#include "structure/tracks/laned_track.h"
#include "structure/tracks/processable_track.h"
#include "structure/tracks/track_processor.h"

namespace zrythm::structure::tracks
{
ProcessableTrack::ProcessableTrack (Dependencies dependencies)
    : automatable_track_mixin_ (
        utils::make_qobject_unique<AutomatableTrackMixin> (dependencies)),
      processor_ (
        utils::make_qobject_unique<TrackProcessor> (
          *this,
          TrackProcessor::ProcessorBaseDependencies{
            .port_registry_ = dependencies.port_registry_,
            .param_registry_ = dependencies.param_registry_ })),
      port_registry_ (dependencies.port_registry_),
      param_registry_ (dependencies.param_registry_)
{
}

void
ProcessableTrack::init_loaded (
  PluginRegistry                  &plugin_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry)
{
}

void
init_from (
  ProcessableTrack       &obj,
  const ProcessableTrack &other,
  utils::ObjectCloneType  clone_type)
{
  init_from (*obj.processor_, *other.processor_, clone_type);
  init_from (
    *obj.automatable_track_mixin_, *other.automatable_track_mixin_, clone_type);
  obj.processor_->track_ = &obj;
}

void
ProcessableTrack::fill_events_common (
  const EngineProcessTimeInfo &time_nfo,
  dsp::MidiEventVector *       midi_events,
  std::optional<std::pair<std::span<float>, std::span<float>>> stereo_ports) const
{
  if (!is_auditioner () && !TRANSPORT->isRolling ())
    {
      return;
    }

  const unsigned_frame_t g_end_frames =
    time_nfo.g_start_frame_w_offset_ + time_nfo.nframes_;

  const bool use_caches = !is_auditioner ();

  std::visit (
    [&] (auto &&track) {
      using TrackT = base_type<decltype (track)>;

      auto process_single_region = [&] (const auto * r) {
        using RegionT = base_type<decltype (r)>;

        // skip region if muted (TODO: check parents)
        if (r->regionMixin ()->mute ()->muted ())
          {
            return;
          }

        /* skip if in bounce mode and the region should not be bounced */
// TODO
#if 0
        if (
          AUDIO_ENGINE->bounce_mode_ != engine::device_io::BounceMode::Off
          && (!r->bounce_ || !bounce_))
          {
            return;
          }
#endif

        /* skip if region is not hit (inclusive of its last point) */
        if (
          !r->regionMixin ()->bounds ()->is_hit_by_range (
            std::make_pair (
              (signed_frame_t) time_nfo.g_start_frame_w_offset_,
              (signed_frame_t) (midi_events ? g_end_frames : (g_end_frames - 1))),
            true))
          {
            return;
          }

        signed_frame_t num_frames_to_process = std::min (
          r->regionMixin ()->bounds ()->get_end_position_samples (true)
            - (signed_frame_t) time_nfo.g_start_frame_w_offset_,
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

            auto [cur_num_frames_till_next_r_loop_or_end, is_loop_end] =
              get_frames_till_next_loop_or_end (
                *r, (signed_frame_t) cur_g_start_frame_w_offset);

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
              == r->regionMixin ()->bounds ()->get_end_position_samples (true);

            const bool is_transport_end =
              TRANSPORT->is_looping ()
              && (signed_frame_t) time_nfo.g_start_frame_w_offset_
                     + num_frames_to_process
                   == TRANSPORT->loop_end_pos_->getFrames ();

            /* whether we need a note off */
            const bool need_note_off =
              (cur_num_frames_till_next_r_loop_or_end < num_frames_to_process)
              || (cur_num_frames_till_next_r_loop_or_end == num_frames_to_process && !is_loop_end)
              /* region end */
              || is_region_end
              /* transport end */
              || is_transport_end;

            /* number of frames to process this time */
            cur_num_frames_till_next_r_loop_or_end = std::min (
              cur_num_frames_till_next_r_loop_or_end, num_frames_to_process);

            const EngineProcessTimeInfo nfo = {
              .g_start_frame_ = cur_g_start_frame,
              .g_start_frame_w_offset_ = cur_g_start_frame_w_offset,
              .local_offset_ = cur_local_start_frame,
              .nframes_ =
                static_cast<nframes_t> (cur_num_frames_till_next_r_loop_or_end)
            };

            if constexpr (
              std::is_same_v<RegionT, arrangement::MidiRegion>
              || std::is_same_v<RegionT, arrangement::ChordRegion>)
              {
                z_return_if_fail (midi_events);
                arrangement::fill_midi_events (
                  *r, nfo, need_note_off,
                  !is_transport_end && (is_loop_end || is_region_end),
                  *midi_events);
              }
            else if constexpr (
              std::is_same_v<RegionT, structure::arrangement::AudioRegion>)
              {
                z_return_if_fail (stereo_ports);
                r->fill_stereo_ports (nfo, *stereo_ports);
              }
            else
              {
                DEBUG_TEMPLATE_PARAM (RegionT)
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
              // TODO: actually use caches
              std::ranges::for_each (
                track->structure::arrangement::template ArrangerObjectOwner<
                  structure::arrangement::ChordRegion>::get_children_view (),
                process_single_region);
            }
          else
            {
              std::ranges::for_each (
                track->structure::arrangement::template ArrangerObjectOwner<
                  structure::arrangement::ChordRegion>::get_children_view (),
                process_single_region);
            }
        }
      else if constexpr (std::derived_from<TrackT, LanedTrack>)
        {
          if (use_caches)
            {
              for (auto &lane : track->lane_snapshots_)
                {
                  // TODO: use snapshots/caches
                  std::ranges::for_each (
                    lane->get_children_view (), process_single_region);
                }
            }
          else
            {
              for (auto &lane_var : track->lanes_)
                {
                  using TrackLaneT = TrackT::LanedTrackImpl::TrackLaneType;
                  auto lane = std::get<TrackLaneT *> (lane_var);
                  std::ranges::for_each (
                    lane->get_children_view (), process_single_region);
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

void
from_json (const nlohmann::json &j, ProcessableTrack &p)
{
  j[ProcessableTrack::kProcessorKey].get_to (*p.processor_);
  j[ProcessableTrack::kAutomatableTrackMixinKey].get_to (
    *p.automatable_track_mixin_);
}
}
