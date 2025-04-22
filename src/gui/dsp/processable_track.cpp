// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/audio_region.h"
#include "gui/dsp/chord_region.h"
#include "gui/dsp/chord_track.h"
#include "gui/dsp/engine.h"
#include "gui/dsp/laned_track.h"
#include "gui/dsp/processable_track.h"
#include "gui/dsp/track_processor.h"
#include "gui/dsp/transport.h"

ProcessableTrack::ProcessableTrack (
  PortRegistry &port_registry,
  bool          new_identity)
    : port_registry_ (port_registry)
{
  if (new_identity)
    {
      processor_ = std::make_unique<TrackProcessor> (*this, port_registry, true);
    }
}

ProcessableTrack::ProcessableTrack (const DeserializationDependencyHolder &dh)
    : ProcessableTrack (dh.get<std::reference_wrapper<PortRegistry>> ().get (), false)
{
}

void
ProcessableTrack::define_base_fields (const utils::serialization::Context &ctx)
{
  using T = ISerializable<ProcessableTrack>;
  auto new_ctx = ctx;
  if (ctx.is_deserializing ())
    {
      new_ctx.dependency_holder_.put (std::ref (*this));
    }
  T::serialize_fields (new_ctx, T::make_field ("processor", processor_));
}

void
ProcessableTrack::init_loaded (
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry)
{
  processor_->track_ = this;
  processor_->init_loaded (this, port_registry);
}

void
ProcessableTrack::copy_members_from (
  const ProcessableTrack &other,
  ObjectCloneType         clone_type)
{
  processor_ = other.processor_->clone_unique (
    ObjectCloneType::Snapshot, *this, port_registry_, false);
  processor_->track_ = this;
}

void
ProcessableTrack::process_block (EngineProcessTimeInfo time_nfo)
{
  processor_->process (time_nfo);
}

bool
ProcessableTrack::get_monitor_audio () const
{
  return processor_->get_monitor_audio_port ().is_toggled ();
}

void
ProcessableTrack::
  set_monitor_audio (bool monitor, bool auto_select, bool fire_events)
{
  if (auto_select)
    {
      TRACKLIST->get_selection_manager ().select_unique (get_uuid ());
    }

  processor_->get_monitor_audio_port ().set_toggled (monitor, fire_events);
}

void
ProcessableTrack::fill_midi_events (
  const EngineProcessTimeInfo &time_nfo,
  MidiEventVector             &midi_events)
{
  fill_events_common (time_nfo, &midi_events, std::nullopt);
}

void
ProcessableTrack::fill_events_common (
  const EngineProcessTimeInfo                       &time_nfo,
  MidiEventVector *                                  midi_events,
  std::optional<std::pair<AudioPort &, AudioPort &>> stereo_ports) const
{
  if (!is_auditioner () && !TRANSPORT->isRolling ())
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

      auto process_single_region = [&] (const auto * r) {
        using RegionT = base_type<decltype (r)>;

        /* skip region if muted */
        if (r->get_muted (true))
          {
            return;
          }

        /* skip if in bounce mode and the region should not be bounced */
        if (
          AUDIO_ENGINE->bounce_mode_ != BounceMode::BOUNCE_OFF
          && (!r->bounce_ || !bounce_))
          {
            return;
          }

        /* skip if region is not hit (inclusive of its last point) */
        if (
          !r->is_hit_by_range (
            (signed_frame_t) time_nfo.g_start_frame_w_offset_,
            (signed_frame_t) (midi_events ? g_end_frames : (g_end_frames - 1)),
            true))
          {
            return;
          }

        signed_frame_t num_frames_to_process = std::min (
          r->end_pos_->frames_
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

            bool           is_loop_end;
            signed_frame_t cur_num_frames_till_next_r_loop_or_end;
            r->get_frames_till_next_loop_or_end (
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
              == r->end_pos_->frames_;

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

            if constexpr (RegionTypeWithMidiEvents<RegionT>)
              {
                z_return_if_fail (midi_events);
                r->fill_midi_events (
                  track, nfo, need_note_off,
                  !is_transport_end && (is_loop_end || is_region_end),
                  *midi_events);
              }
            else if constexpr (std::is_same_v<RegionT, AudioRegion>)
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
              std::ranges::for_each (
                track->ArrangerObjectOwner<
                  ChordRegion>::get_children_snapshots_view (),
                process_single_region);
            }
          else
            {
              std::ranges::for_each (
                track->ArrangerObjectOwner<ChordRegion>::get_children_view (),
                process_single_region);
            }
        }
      else if constexpr (std::derived_from<TrackT, LanedTrack>)
        {
          if (use_caches)
            {
              for (auto &lane : track->lane_snapshots_)
                {
                  std::ranges::for_each (
                    lane->get_children_snapshots_view (), process_single_region);
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
ProcessableTrack::append_member_ports (
  std::vector<Port *> &ports,
  bool                 include_plugins) const
{
  processor_->append_ports (ports);
}
