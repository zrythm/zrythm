// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <inttypes.h>

#include "actions/arranger_selections.h"
#include "dsp/arranger_object.h"
#include "dsp/audio_region.h"
#include "dsp/audio_track.h"
#include "dsp/automatable_track.h"
#include "dsp/automation_region.h"
#include "dsp/chord_region.h"
#include "dsp/chord_track.h"
#include "dsp/clip.h"
#include "dsp/control_port.h"
#include "dsp/engine.h"
#include "dsp/laned_track.h"
#include "dsp/piano_roll_track.h"
#include "dsp/processable_track.h"
#include "dsp/recordable_track.h"
#include "dsp/recording_event.h"
#include "dsp/recording_manager.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "dsp/transport.h"
#include "project.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtest_wrapper.h"
#include "utils/math.h"
#include "utils/midi.h"
#include "utils/mpmc_queue.h"
#include "utils/object_pool.h"
#include "utils/objects.h"
#include "zrythm.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

void
RecordingManager::handle_stop_recording (bool is_automation)
{
  z_return_if_fail (num_active_recordings_ > 0);

  /* skip if still recording */
  if (num_active_recordings_ > 1)
    {
      num_active_recordings_--;
      return;
    }

  z_info (
    "{}{}", "----- stopped recording", is_automation ? " (automation)" : "");

  /* cache the current selections */
  auto prev_selections = std::make_unique<TimelineSelections> (*TL_SELECTIONS);

  /* select all the recorded regions */
  TL_SELECTIONS->clear (false);
  for (auto &id : recorded_ids_)
    {
      if (
        (is_automation && !id.is_automation ())
        || (!is_automation && id.is_automation ()))
        continue;

      /* do some sanity checks for lane regions */
      if (region_type_has_lane (id.type_))
        {
          auto track = dynamic_cast<LanedTrack *> (
            TRACKLIST->find_track_by_name_hash (id.track_name_hash_));
          z_return_if_fail (track);

          std::visit (
            [&] (auto &&t) {
              z_return_if_fail (id.lane_pos_ < (int) t->lanes_.size ());
              auto &lane = t->lanes_[id.lane_pos_];
              z_return_if_fail (lane);
              z_return_if_fail (
                id.idx_ <= static_cast<int> (lane->regions_.size ()));
            },
            convert_to_variant<LanedTrackPtrVariant> (track));
        }

      auto region = Region::find (id);
      z_return_if_fail (region);
      TL_SELECTIONS->add_object_ref (region);

      if (is_automation)
        {
          auto automation_region =
            dynamic_pointer_cast<AutomationRegion> (region);
          automation_region->last_recorded_ap_ = nullptr;
        }
    }

  /* perform the create action */
  try
    {
      UNDO_MANAGER->perform (
        std::make_unique<ArrangerSelectionsAction::RecordAction> (
          *selections_before_start_, *TL_SELECTIONS, true));
    }
  catch (const ZrythmException &ex)
    {
      ex.handle (_ ("Failed to create recorded regions"));
    }

  /* update frame caches and write audio clips to pool */
  for (auto &id : recorded_ids_)
    {
      auto r = Region::find (id);
      if (r->is_audio ())
        {
          auto        ar = dynamic_pointer_cast<AudioRegion> (r);
          AudioClip * clip = ar->get_clip ();
          try
            {
              clip->write_to_pool (true, false);
              clip->finalize_buffered_write ();
            }
          catch (const ZrythmException &ex)
            {
              ex.handle ("Failed to write audio region clip to pool");
            }
        }
    }

  /* restore the selections */
  TL_SELECTIONS->clear (false);
  for (auto &obj : prev_selections->objects_)
    {
      auto found_obj = obj->find_in_project ();
      z_return_if_fail (found_obj);
      found_obj->select (true, true, false);
    }

  /* free the temporary selections */
  selections_before_start_.reset ();

  /* disarm transport record button */
  TRANSPORT->set_recording (false, true, true);

  num_active_recordings_--;
  recorded_ids_.clear ();
  z_return_if_fail (num_active_recordings_ == 0);
}

void
RecordingManager::handle_recording (
  const TrackProcessor *              track_processor,
  const EngineProcessTimeInfo * const time_nfo)
{
#if 0
  z_info (
    "handling recording from {} ({} frames)",
    g_start_frames + ev->local_offset, ev->nframes);
#endif

  /* whether to skip adding track recording
   * events */
  bool skip_adding_track_events = false;

  /* whether to skip adding automation recording
   * events */
  bool skip_adding_automation_events = false;

  /* whether we are inside the punch range in
   * punch mode or true if otherwise */
  bool inside_punch_range = false;

  z_return_if_fail_cmp (
    time_nfo->local_offset_ + time_nfo->nframes_, <=,
    AUDIO_ENGINE->block_length_);

  if (TRANSPORT->punch_mode_)
    {
      Position tmp ((signed_frame_t) time_nfo->g_start_frame_w_offset_);
      inside_punch_range = TRANSPORT->position_is_inside_punch_range (tmp);
    }
  else
    {
      inside_punch_range = true;
    }

  /* ---- handle start/stop/pause recording events ---- */
  auto   tr = track_processor->get_track ();
  auto   atl = &tr->get_automation_tracklist ();
  auto   recordable_track = dynamic_cast<RecordableTrack *> (tr);
  gint64 cur_time = g_get_monotonic_time ();

  /* if track type can't record do nothing */
  if (!tr->can_record ()) [[unlikely]]
    {
    }
  /* else if not recording at all (recording stopped) */
  else if (
    !TRANSPORT->is_recording () || !recordable_track->recording_
    || !TRANSPORT->is_rolling ())
    {
      /* if track had previously recorded */
      if (
        G_UNLIKELY (recordable_track->recording_region_)
        && !recordable_track->recording_stop_sent_)
        {
          recordable_track->recording_stop_sent_ = true;

          /* send stop recording event */
          auto re = event_obj_pool_.acquire ();
          re->init (RecordingEvent::Type::StopTrackRecording, *tr, *time_nfo);
          event_queue_.push_back (re);
        }
      skip_adding_track_events = true;
    }
  /* if pausing */
  else if (time_nfo->nframes_ == 0)
    {
      if (
        recordable_track->recording_region_
        || recordable_track->recording_start_sent_)
        {
          /* send pause event */
          auto re = event_obj_pool_.acquire ();
          re->init (RecordingEvent::Type::PauseTrackRecording, *tr, *time_nfo);
          event_queue_.push_back (re);

          skip_adding_track_events = true;
        }
    }
  /* if recording and inside punch range */
  else if (inside_punch_range)
    {
      /* if no recording started yet */
      if (
        !recordable_track->recording_region_
        && !recordable_track->recording_start_sent_)
        {
          recordable_track->recording_start_sent_ = true;

          /* send start recording event */
          auto re = event_obj_pool_.acquire ();
          re->init (RecordingEvent::Type::StartTrackRecording, *tr, *time_nfo);
          event_queue_.push_back (re);
        }
    }
  else if (!inside_punch_range)
    {
      skip_adding_track_events = true;
    }

  for (auto at : atl->ats_in_record_mode_)
    {
      bool at_should_be_recording = at->should_be_recording (cur_time, false);

      /* if should stop automation recording */
      if (
        G_UNLIKELY (at->recording_started_)
        && (!TRANSPORT->is_rolling () || !at_should_be_recording))
        {
          /* send stop automation recording event */
          auto re = event_obj_pool_.acquire ();
          re->init (
            RecordingEvent::Type::StopAutomationRecording, *tr, *time_nfo);
          re->automation_track_idx_ = at->index_;
          event_queue_.push_back (re);

          skip_adding_automation_events = true;
        }
      /* if pausing (only at loop end) */
      else if (G_UNLIKELY (at->recording_start_sent_ && time_nfo->nframes_ == 0) && (time_nfo->g_start_frame_w_offset_ == static_cast<unsigned_frame_t> (TRANSPORT->loop_end_pos_.frames_)))
        {
          /* send pause event */
          auto re = event_obj_pool_.acquire ();
          re->init (
            RecordingEvent::Type::PauseAutomationRecording, *tr, *time_nfo);
          re->automation_track_idx_ = at->index_;
          event_queue_.push_back (re);

          skip_adding_automation_events = true;
        }

      /* if automation should be recording */
      if (TRANSPORT->is_rolling () && at_should_be_recording)
        {
          /* if recording hasn't started yet */
          if (!at->recording_started_ && !at->recording_start_sent_)
            {
              at->recording_start_sent_ = true;

              /* send start recording event */
              auto re = event_obj_pool_.acquire ();
              re->init (
                RecordingEvent::Type::StartAutomationRecording, *tr, *time_nfo);
              re->automation_track_idx_ = at->index_;
              event_queue_.push_back (re);
            }
        }
    }

  /* ---- end handling start/stop/pause recording events ---- */

  if (!skip_adding_track_events)
    {
      /* add recorded track material to event queue */
      if (tr->has_piano_roll () || tr->is_chord ())
        {

          auto &midi_events =
            track_processor->midi_in_->midi_events_.active_events_;

          for (const auto &me : midi_events)
            {
              auto re = event_obj_pool_.acquire ();
              re->init (RecordingEvent::Type::Midi, *tr, *time_nfo);
              re->has_midi_event_ = true;
              re->midi_event_ = me;
              event_queue_.push_back (re);
            }

          if (midi_events.empty ())
            {
              auto re = event_obj_pool_.acquire ();
              re->init (RecordingEvent::Type::Midi, *tr, *time_nfo);
              re->has_midi_event_ = false;
              event_queue_.push_back (re);
            }
        }
      else if (tr->type_ == Track::Type::Audio)
        {
          auto re = event_obj_pool_.acquire ();
          re->init (RecordingEvent::Type::Audio, *tr, *time_nfo);
          dsp_copy (
            &re->lbuf_[time_nfo->local_offset_],
            &track_processor->stereo_in_->get_l ().buf_[time_nfo->local_offset_],
            time_nfo->nframes_);
          Port &r =
            track_processor->mono_ && track_processor->mono_->is_toggled ()
              ? track_processor->stereo_in_->get_l ()
              : track_processor->stereo_in_->get_r ();
          dsp_copy (
            &re->rbuf_[time_nfo->local_offset_],
            &r.buf_[time_nfo->local_offset_], time_nfo->nframes_);
          event_queue_.push_back (re);
        }
    }

  if (skip_adding_automation_events)
    return;

  /* add automation events */

  if (!TRANSPORT->is_rolling ())
    return;

  for (auto at : atl->ats_in_record_mode_)
    {
      /* only proceed if automation should be recording */

      if (!at->recording_start_sent_) [[likely]]
        continue;

      if (!at->should_be_recording (cur_time, false))
        continue;

      /* send recording event */
      auto re = event_obj_pool_.acquire ();
      re->init (RecordingEvent::Type::Automation, *tr, *time_nfo);
      re->automation_track_idx_ = at->index_;
      event_queue_.push_back (re);
    }
}
void
RecordingManager::delete_automation_points (
  AutomationTrack * at,
  AutomationRegion &region,
  Position          pos)
{
  region.get_aps_since_last_recorded (pos, pending_aps_);
  for (AutomationPoint * ap : pending_aps_)
    {
      region.remove_object (*ap, false);
    }

  /* create a new automation point at the pos with the previous value */
  if (region.last_recorded_ap_)
    {
      /* remove the last recorded AP if its previous AP also has the same value */
      AutomationPoint * ap_before_recorded =
        region.get_prev_ap (*region.last_recorded_ap_);
      float prev_fvalue = region.last_recorded_ap_->fvalue_;
      float prev_normalized_val = region.last_recorded_ap_->normalized_val_;
      if (
        ap_before_recorded
        && math_floats_equal (
          ap_before_recorded->fvalue_, region.last_recorded_ap_->fvalue_))
        {
          region.remove_object (*region.last_recorded_ap_, false);
        }

      Position adj_pos = pos;
      adj_pos.add_ticks (-region.pos_.ticks_);
      auto ap = region.append_object (
        std::make_shared<AutomationPoint> (
          prev_fvalue, prev_normalized_val, adj_pos),
        true);
      region.last_recorded_ap_ = ap.get ();
    }
}

AutomationPoint *
RecordingManager::create_automation_point (
  AutomationTrack * at,
  AutomationRegion &region,
  float             val,
  float             normalized_val,
  Position          pos)
{
  region.get_aps_since_last_recorded (pos, pending_aps_);
  for (AutomationPoint * ap : pending_aps_)
    {
      region.remove_object (*ap, false);
    }

  Position adj_pos = pos;
  adj_pos.add_ticks (-region.pos_.ticks_);
  if (
    region.last_recorded_ap_
    && math_floats_equal (
      region.last_recorded_ap_->normalized_val_, normalized_val)
    && region.last_recorded_ap_->pos_ == adj_pos)
    {
      /* this block is used to avoid duplicate automation points */
      /* TODO this shouldn't happen and needs investigation */
      return nullptr;
    }
  else
    {
      auto ap = region.append_object (
        std::make_shared<AutomationPoint> (val, normalized_val, adj_pos), true);
      ap->curve_opts_.curviness_ = 1.0;
      ap->curve_opts_.algo_ = CurveOptions::Algorithm::Pulse;
      region.last_recorded_ap_ = ap.get ();
      return ap.get ();
    }

  return nullptr;
}

void
RecordingManager::handle_pause_event (const RecordingEvent &ev)
{
  auto tr =
    TRACKLIST->find_track_by_name_hash<RecordableTrack> (ev.track_name_hash_);
  z_return_if_fail (tr);

  /* position to pause at */
  Position pause_pos;
  pause_pos.from_frames ((signed_frame_t) ev.g_start_frame_w_offset_);

#if 0
  z_debug ("track {} pause start frames {}, nframes {}", tr->name_.c_str(), pause_pos.frames_, ev.nframes_);
#endif

  if (ev.type_ == RecordingEvent::Type::PauseTrackRecording)
    {
      tr->recording_paused_ = true;

      /* get the recording region */
      Region * region = tr->recording_region_;
      z_return_if_fail (region);

      std::visit (
        [&] (auto &&r) {
          using T = base_type<decltype (r)>;
          if constexpr (std::derived_from<T, LaneOwnedObject>)
            {
              /* remember lane index */
              auto laned_track = dynamic_cast<LanedTrackImpl<T> *> (tr);
              laned_track->last_lane_idx_ = r->id_.lane_pos_;

              if (tr->in_signal_type_ == PortType::Event)
                {
                  auto midi_region = dynamic_cast<MidiRegion *> (r);
                  /* add midi note offs at the end */
                  MidiNote * mn;
                  while ((mn = midi_region->pop_unended_note (-1)))
                    {
                      mn->end_pos_setter (&pause_pos);
                    }
                }
            }
        },
        convert_to_variant<RegionPtrVariant> (region));
    }
  else if (ev.type_ == RecordingEvent::Type::PauseAutomationRecording)
    {
      auto &at = tr->get_automation_tracklist ().ats_[ev.automation_track_idx_];
      at->recording_paused_ = true;
    }
}

bool
RecordingManager::handle_resume_event (const RecordingEvent &ev)
{
  auto * tr =
    TRACKLIST->find_track_by_name_hash<AutomatableTrack> (ev.track_name_hash_);
  gint64 cur_time = g_get_monotonic_time ();

  /* position to resume from */
  Position resume_pos ((signed_frame_t) ev.g_start_frame_w_offset_);

  /* position 1 frame afterwards */
  Position end_pos = resume_pos;
  end_pos.add_frames (1);

  auto recording_track = dynamic_cast<RecordableTrack *> (tr);
  if (
    ev.type_ == RecordingEvent::Type::Midi
    || ev.type_ == RecordingEvent::Type::Audio)
    {
      /* not paused, nothing to do */
      if (!recording_track->recording_paused_)
        {
          return false;
        }

      recording_track->recording_paused_ = false;

      if (
        TRANSPORT->recording_mode_ == Transport::RecordingMode::Takes
        || TRANSPORT->recording_mode_ == Transport::RecordingMode::TakesMuted
        || ev.type_ == RecordingEvent::Type::Audio)
        {
          /* mute the previous region */
          if (
            (TRANSPORT->recording_mode_ == Transport::RecordingMode::TakesMuted
             || (TRANSPORT->recording_mode_ == Transport::RecordingMode::OverwriteEvents && ev.type_ == RecordingEvent::Type::Audio))
            && recording_track->recording_region_)
            {
              recording_track->recording_region_->set_muted (true, true);
            }

          auto success = std::visit (
            [&] (auto &&casted_tr) {
              using T = base_type<decltype (casted_tr)>;
              std::shared_ptr<Region> added_region;
              if constexpr (std::is_same_v<T, ChordTrack>)
                {
                  auto chord_track = dynamic_cast<ChordTrack *> (casted_tr);
                  auto new_region = std::make_shared<ChordRegion> (
                    resume_pos, end_pos, chord_track->regions_.size ());
                  try
                    {
                      added_region = casted_tr->Track::add_region (
                        std::move (new_region), nullptr, -1, true, true);
                    }
                  catch (const ZrythmException &ex)
                    {
                      ex.handle ("Failed to add region to track");
                      return false;
                    }
                }
              else if constexpr (std::derived_from<T, LanedTrack>)
                {
                  using RegionT = T::RegionType;
                  /* start new region in new lane */
                  int new_lane_pos = casted_tr->last_lane_idx_ + 1;
                  int idx_inside_lane =
                    (int) casted_tr->lanes_.size () > new_lane_pos
                      ? casted_tr->lanes_[new_lane_pos]->regions_.size ()
                      : 0;
                  std::shared_ptr<RegionT> new_region;
                  if constexpr (std::is_same_v<RegionT, MidiRegion>)
                    {
                      new_region = std::make_shared<MidiRegion> (
                        resume_pos, end_pos, casted_tr->get_name_hash (),
                        new_lane_pos, idx_inside_lane);
                    }
                  else if constexpr (std::is_same_v<RegionT, AudioRegion>)
                    {
                      auto name = AUDIO_POOL->gen_name_for_recording_clip (
                        *casted_tr, new_lane_pos);
                      new_region = std::make_shared<AudioRegion> (
                        -1, std::nullopt, true, nullptr, 1, name, 2,
                        BitDepth::BIT_DEPTH_32, resume_pos,
                        casted_tr->get_name_hash (), new_lane_pos,
                        idx_inside_lane);
                    }
                  else
                    {
                      static_assert (false, "unsupported region type");
                    }
                  try
                    {
                      added_region = casted_tr->add_region (
                        std::move (new_region), nullptr, new_lane_pos, true,
                        true);
                    }
                  catch (const ZrythmException &e)
                    {
                      e.handle ("Failed to add region to track");
                      return false;
                    }
                }
              else
                {
                  /* nothing to do for this track type */
                  return true;
                }

              if constexpr (std::derived_from<T, RecordableTrack>)
                {
                  /* remember region */
                  recorded_ids_.push_back (added_region->id_);
                  casted_tr->recording_region_ = added_region.get ();
                }

              return true;
            },
            convert_to_variant<TrackPtrVariant> (tr));

          if (!success)
            {
              return false;
            }
        }
      /* if MIDI and overwriting or merging events */
      else if (recording_track->recording_region_)
        {
          /* extend the previous region */
          if (resume_pos < recording_track->recording_region_->pos_)
            {
              double ticks_delta =
                recording_track->recording_region_->pos_.ticks_
                - resume_pos.ticks_;
              recording_track->recording_region_->set_start_pos_full_size (
                &resume_pos);
              recording_track->recording_region_->add_ticks_to_children (
                ticks_delta);
            }
          if (end_pos > recording_track->recording_region_->end_pos_)
            {
              recording_track->recording_region_->set_end_pos_full_size (
                &end_pos);
            }
        }
    }
  else if (ev.type_ == RecordingEvent::Type::Automation)
    {
      auto &at = tr->automation_tracklist_->ats_[ev.automation_track_idx_];
      z_return_val_if_fail (at, false);

      /* not paused, nothing to do */
      if (!at->recording_paused_)
        return false;

      auto  port = Port::find_from_identifier<ControlPort> (at->port_id_);
      float value = port->get_control_value (false);
      float normalized_value = port->get_control_value (true);

      /* get or start new region at resume pos */
      auto new_region = at->get_region_before_pos (resume_pos, true, false);
      if (!new_region && at->should_be_recording (cur_time, false))
        {
          /* create region */
          auto new_region_to_add = std::make_shared<AutomationRegion> (
            resume_pos, end_pos, tr->get_name_hash (), at->index_,
            at->regions_.size ());
          z_return_val_if_fail (new_region, false);
          try
            {
              auto ret = tr->add_region<AutomationRegion> (
                std::move (new_region_to_add), at.get (), -1, true, true);
              new_region = ret.get ();
            }
          catch (const ZrythmException &e)
            {
              e.handle ("Failed to add region to track");
              return false;
            }
        }
      z_return_val_if_fail (new_region, false);
      recorded_ids_.push_back (new_region->id_);

      if (at->should_be_recording (cur_time, true))
        {
          while (
            new_region->aps_.size () > 0
            && new_region->aps_[0]->pos_ == resume_pos)
            {
              new_region->remove_object (*new_region->aps_[0]);
            }

          /* create/replace ap at loop start */
          create_automation_point (
            at.get (), *new_region, value, normalized_value, resume_pos);
        }
    }

  return true;
}
void
RecordingManager::handle_audio_event (const RecordingEvent &ev)
{
  bool handled_resume = handle_resume_event (ev);
  (void) handled_resume;
  /*z_debug ("handled resume {}", handled_resume);*/

  auto tr =
    TRACKLIST->find_track_by_name_hash<RecordableTrack> (ev.track_name_hash_);

  /* get end position */
  unsigned_frame_t start_frames = ev.g_start_frame_w_offset_;
  unsigned_frame_t end_frames = start_frames + ev.nframes_;

  Position start_pos, end_pos;
  start_pos.from_frames ((signed_frame_t) start_frames);
  end_pos.from_frames ((signed_frame_t) end_frames);

  /* get the recording region */
  auto region = dynamic_cast<AudioRegion *> (tr->recording_region_);
  z_return_if_fail (region);

  /* the clip */
  auto clip = region->get_clip ();

  /* set region end pos */
  region->set_end_pos_full_size (&end_pos);

  signed_frame_t r_obj_len_frames =
    (region->end_pos_.frames_ - region->pos_.frames_);
  z_return_if_fail_cmp (r_obj_len_frames, >=, 0);
  clip->num_frames_ = (unsigned_frame_t) r_obj_len_frames;
  clip->frames_.setSize (1, clip->channels_ * clip->num_frames_);

  region->loop_end_pos_.from_frames (
    region->end_pos_.frames_ - region->pos_.frames_);
  region->fade_out_pos_ = region->loop_end_pos_;

  /* handle the samples normally */
  nframes_t cur_local_offset = 0;
  for (
    signed_frame_t i = (signed_frame_t) start_frames - region->pos_.frames_;
    i < (signed_frame_t) end_frames - region->pos_.frames_; ++i)
    {
      z_return_if_fail_cmp (i, >=, 0);
      z_return_if_fail_cmp (i, <, (signed_frame_t) clip->num_frames_);
      z_warn_if_fail (
        cur_local_offset >= ev.local_offset_
        && cur_local_offset < ev.local_offset_ + ev.nframes_);

      /* set clip frames */
      clip->frames_.setSample (
        0, i * clip->get_num_channels (), ev.lbuf_[cur_local_offset]);
      clip->frames_.setSample (
        0, i * clip->get_num_channels () + 1, ev.rbuf_[cur_local_offset]);

      cur_local_offset++;
    }

  clip->update_channel_caches ((size_t) clip->frames_written_);

  /* write to pool if 2 seconds passed since last write */
  gint64 cur_time = g_get_monotonic_time ();
  gint64 nano_sec_to_wait = 2 * 1000 * 1000;
  if (ZRYTHM_TESTING)
    {
      nano_sec_to_wait = 20 * 1000;
    }
  if ((cur_time - clip->last_write_) > nano_sec_to_wait)
    {
      try
        {
          clip->write_to_pool (true, false);
        }
      catch (const ZrythmException &ex)
        {
          ex.handle ("Failed to write audio clip to pool");
        }
    }
}

void
RecordingManager::handle_midi_event (const RecordingEvent &ev)
{
  handle_resume_event (ev);

  auto tr =
    TRACKLIST->find_track_by_name_hash<RecordableTrack> (ev.track_name_hash_);

  z_return_if_fail (tr->recording_region_);

  Position start_pos, end_pos;
  start_pos.from_frames ((signed_frame_t) ev.g_start_frame_w_offset_);
  end_pos.from_frames (
    (signed_frame_t) ev.g_start_frame_w_offset_ + (signed_frame_t) ev.nframes_);

  std::visit (
    [&] (auto &&region) {
      using RegionT = base_type<decltype (region)>;

      /* set region end pos */
      bool set_end_pos = false;
      switch (TRANSPORT->recording_mode_)
        {
        case Transport::RecordingMode::OverwriteEvents:
        case Transport::RecordingMode::MergeEvents:
          if (region->end_pos_ < end_pos)
            {
              set_end_pos = true;
            }
          break;
        case Transport::RecordingMode::Takes:
        case Transport::RecordingMode::TakesMuted:
          set_end_pos = true;
          break;
        }
      if (set_end_pos)
        {
          region->set_end_pos_full_size (&end_pos);
        }

      tr->recording_region_ = region;

      /* get local positions */
      Position local_pos = start_pos;
      Position local_end_pos = end_pos;
      local_pos.add_ticks (-region->pos_.ticks_);
      local_end_pos.add_ticks (-region->pos_.ticks_);

      /* if overwrite mode, clear any notes inside the range */
      if (
        TRANSPORT->recording_mode_ == Transport::RecordingMode::OverwriteEvents)
        {
          if constexpr (std::is_same_v<RegionT, MidiRegion>)
            {
              for (int i = region->midi_notes_.size () - 1; i >= 0; --i)
                {
                  auto &mn = region->midi_notes_[i];

                  if (mn->is_hit_by_range (
                        local_pos, local_end_pos, false, false, true))
                    {
                      region->remove_object (*mn, true);
                    }
                }
            }
        }

      if (!ev.has_midi_event_)
        return;

      /* convert MIDI data to midi notes */
      MidiNote *  mn;
      const auto &mev = ev.midi_event_;
      const auto &buf = mev.raw_buffer_.data ();

      if constexpr (std::is_same_v<RegionT, ChordRegion>)
        {
          if (midi_is_note_on (buf))
            {
              midi_byte_t             note_number = midi_get_note_number (buf);
              const ChordDescriptor * descr =
                CHORD_EDITOR->get_chord_from_note_number (note_number);
              z_return_if_fail (descr);
              int  chord_idx = CHORD_EDITOR->get_chord_index (*descr);
              auto co = std::make_shared<ChordObject> (
                region->id_, chord_idx, region->chord_objects_.size ());
              region->append_object (std::move (co), true);
              co->set_position (
                &local_pos, ArrangerObject::PositionType::Start, false);
            }
        }
      /* else if not chord track */
      else if constexpr (std::is_same_v<RegionT, MidiRegion>)
        {
          if (midi_is_note_on (buf))
            {
              z_return_if_fail (region);
              region->start_unended_note (
                &local_pos, &local_end_pos, midi_get_note_number (buf),
                midi_get_velocity (buf), true);
            }
          else if (midi_is_note_off (buf))
            {
              z_return_if_fail (region);
              mn = region->pop_unended_note (midi_get_note_number (buf));
              if (mn)
                {
                  mn->end_pos_setter (&local_end_pos);
                }
            }
          else
            {
              /* TODO */
            }
        }
    },
    convert_to_variant<RegionPtrVariant> (tr->recording_region_));
}

void
RecordingManager::handle_automation_event (const RecordingEvent &ev)
{
  handle_resume_event (ev);

  auto * tr =
    TRACKLIST->find_track_by_name_hash<AutomatableTrack> (ev.track_name_hash_);
  auto &atl = tr->automation_tracklist_;
  auto &at = atl->ats_[ev.automation_track_idx_];
  auto  port = Port::find_from_identifier<ControlPort> (at->port_id_);
  float value = port->get_control_value (false);
  float normalized_value = port->get_control_value (true);
  if (ZRYTHM_TESTING)
    {
      math_assert_nonnann (value);
      math_assert_nonnann (normalized_value);
    }
  bool automation_value_changed =
    !port->value_changed_from_reading_
    && !math_floats_equal (value, at->last_recorded_value_);
  gint64 cur_time = g_get_monotonic_time ();

  /* get end position */
  unsigned_frame_t start_frames = ev.g_start_frame_w_offset_;
  unsigned_frame_t end_frames = start_frames + ev.nframes_;

  Position start_pos (static_cast<signed_frame_t> (start_frames));
  Position end_pos (static_cast<signed_frame_t> (end_frames));

  bool new_region_created = false;

  /* get the recording region */
  auto region = at->get_region_before_pos (start_pos, true, false);

  auto region_at_end = at->get_region_before_pos (end_pos, true, false);
  if (!region && automation_value_changed)
    {
      z_debug ("creating new automation region (automation value changed)");
      /* create region */
      Position pos_to_end_new_r;
      if (region_at_end)
        {
          pos_to_end_new_r = region_at_end->pos_;
        }
      else
        {
          pos_to_end_new_r = end_pos;
        }
      region =
        tr->add_region (
            std::make_shared<AutomationRegion> (
              start_pos, pos_to_end_new_r, tr->get_name_hash (), at->index_,
              at->regions_.size ()),
            at.get (), -1, true, true)
          .get ();
      new_region_created = true;
      z_return_if_fail (region);

      recorded_ids_.push_back (region->id_);
    }

  at->recording_region_ = region;

  if (new_region_created || (region && region->end_pos_ < end_pos))
    {
      /* set region end pos */
      region->set_end_pos_full_size (&end_pos);
    }

  at->recording_region_ = region;

  /* handle the samples normally */
  if (automation_value_changed)
    {
      create_automation_point (
        at.get (), *region, value, normalized_value, start_pos);
      at->last_recorded_value_ = value;
    }
  else if (at->record_mode_ == AutomationRecordMode::Latch)
    {
      z_return_if_fail (region);
      delete_automation_points (at.get (), *region, start_pos);
    }

  /* if we left touch mode, set last recorded ap to NULL */
  if (
    at->record_mode_ == AutomationRecordMode::Touch
    && !at->should_be_recording (cur_time, true) && at->recording_region_)
    {
      at->recording_region_->last_recorded_ap_ = nullptr;
    }
}

void
RecordingManager::handle_start_recording (
  const RecordingEvent &ev,
  bool                  is_automation)
{
  auto tr =
    TRACKLIST->find_track_by_name_hash<AutomatableTrack> (ev.track_name_hash_);
  z_return_if_fail (tr);
  gint64            cur_time = g_get_monotonic_time ();
  AutomationTrack * at = nullptr;
  if (is_automation)
    {
      at = tr->automation_tracklist_->ats_[ev.automation_track_idx_].get ();
    }

  if (num_active_recordings_ == 0)
    {
      selections_before_start_ =
        std::make_unique<TimelineSelections> (*TL_SELECTIONS);
    }

  /* this could be called multiple times, ignore if already processed */
  auto * recordable_track = dynamic_cast<RecordableTrack *> (tr);
  if (
    !is_automation && (recordable_track->recording_region_ != nullptr)
    && !is_automation)
    {
      z_warning ("record start already processed");
      num_active_recordings_++;
      return;
    }

  /* get end position */
  unsigned_frame_t start_frames = ev.g_start_frame_w_offset_;
  unsigned_frame_t end_frames = start_frames + ev.nframes_;

  z_debug ("start {}, end {}", start_frames, end_frames);

  z_return_if_fail (start_frames < end_frames);

  Position start_pos (static_cast<signed_frame_t> (start_frames));
  Position end_pos (static_cast<signed_frame_t> (end_frames));

  if (is_automation)
    {
      /* don't unset recording paused, this will be unset by handle_resume() */
      /*at->recording_paused = false;*/

      /* nothing, wait for event to start writing data */
      auto * port = Port::find_from_identifier<ControlPort> (at->port_id_);
      float  value = port->get_control_value (false);

      if (at->should_be_recording (cur_time, true))
        {
          /* set recorded value to something else to force the recorder to start
           * writing */
          // z_info ("SHOULD BE RECORDING");
          at->last_recorded_value_ = value + 2.f;
        }
      else
        {
          // z_info ("SHOULD NOT BE RECORDING");
          /** set the current value so that nothing is recorded until it changes
           */
          at->last_recorded_value_ = value;
        }
    }
  else
    {
      recordable_track->recording_paused_ = false;

      try
        {
          if (tr->has_piano_roll ())
            {
              auto piano_roll_track = dynamic_cast<PianoRollTrack *> (tr);
              /* create region */
              int  new_lane_pos = piano_roll_track->lanes_.size () - 1;
              auto region = tr->add_region (
                std::make_shared<MidiRegion> (
                  start_pos, end_pos, tr->get_name_hash (), new_lane_pos,
                  piano_roll_track->lanes_[new_lane_pos]->regions_.size ()),
                nullptr, new_lane_pos, true, true);
              z_return_if_fail (region);

              recordable_track->recording_region_ = region.get ();
              recorded_ids_.push_back (region->id_);
            }
          else if (tr->is_chord ())
            {
              auto chord_track = dynamic_cast<ChordTrack *> (tr);
              auto region = tr->add_region (
                std::make_shared<ChordRegion> (
                  start_pos, end_pos, chord_track->regions_.size ()),
                nullptr, -1, true, true);
              z_return_if_fail (region);

              recordable_track->recording_region_ = region.get ();
              recorded_ids_.push_back (region->id_);
            }
          else if (tr->type_ == Track::Type::Audio)
            {
              auto audio_track = dynamic_cast<AudioTrack *> (tr);
              /* create region */
              int  new_lane_pos = audio_track->lanes_.size () - 1;
              auto name =
                AUDIO_POOL->gen_name_for_recording_clip (*tr, new_lane_pos);
              auto region = tr->add_region (
                std::make_shared<AudioRegion> (
                  -1, std::nullopt, true, nullptr, ev.nframes_, name, 2,
                  BitDepth::BIT_DEPTH_32, start_pos, tr->get_name_hash (),
                  new_lane_pos,
                  audio_track->lanes_[new_lane_pos]->regions_.size ()),
                nullptr, new_lane_pos, true, true);
              z_return_if_fail (region);

              recordable_track->recording_region_ = region.get ();
              recorded_ids_.push_back (region->id_);
            }
        }
      catch (const ZrythmException &ex)
        {
          ex.handle ("Failed to add region");
          return;
        }
    }

  num_active_recordings_++;
}
int
RecordingManager::process_events ()
{
  SemaphoreRAII lock (processing_sem_);
  z_return_val_if_fail (!currently_processing_, G_SOURCE_REMOVE);
  currently_processing_ = true;

  RecordingEvent * ev;
  while (event_queue_.pop_front (ev))
    {
      if (freeing_)
        {
          goto return_to_pool;
        }

      switch (ev->type_)
        {
        case RecordingEvent::Type::Midi:
          handle_midi_event (*ev);
          break;
        case RecordingEvent::Type::Audio:
          handle_audio_event (*ev);
          break;
        case RecordingEvent::Type::Automation:
          handle_automation_event (*ev);
          break;
        case RecordingEvent::Type::PauseTrackRecording:
          z_debug ("-------- PAUSE TRACK RECORDING");
          handle_pause_event (*ev);
          break;
        case RecordingEvent::Type::PauseAutomationRecording:
          z_debug ("-------- PAUSE AUTOMATION RECORDING");
          handle_pause_event (*ev);
          break;
        case RecordingEvent::Type::StopTrackRecording:
          {
            auto tr = TRACKLIST->find_track_by_name_hash<RecordableTrack> (
              ev->track_name_hash_);
            z_return_val_if_fail (tr, G_SOURCE_CONTINUE);
            z_debug ("-------- STOP TRACK RECORDING ({})", tr->name_);
            handle_stop_recording (false);
            tr->recording_region_ = nullptr;
            tr->recording_start_sent_ = false;
            tr->recording_stop_sent_ = false;
          }
          z_debug ("num active recordings: {}", num_active_recordings_);
          break;
        case RecordingEvent::Type::StopAutomationRecording:
          z_debug ("-------- STOP AUTOMATION RECORDING");
          {
            auto tr = TRACKLIST->find_track_by_name_hash<AutomatableTrack> (
              ev->track_name_hash_);
            auto &at =
              tr->automation_tracklist_->ats_[ev->automation_track_idx_];
            z_return_val_if_fail (at, G_SOURCE_REMOVE);
            if (at->recording_started_)
              {
                handle_stop_recording (true);
              }
            at->recording_started_ = false;
            at->recording_start_sent_ = false;
            at->recording_region_ = nullptr;
          }
          z_debug ("num active recordings: {}", num_active_recordings_);
          break;
        case RecordingEvent::Type::StartTrackRecording:
          {
            auto tr = TRACKLIST->find_track_by_name_hash<RecordableTrack> (
              ev->track_name_hash_);
            z_return_val_if_fail (tr, G_SOURCE_CONTINUE);
            z_debug ("-------- START TRACK RECORDING ({})", tr->name_);
            handle_start_recording (*ev, false);
            z_debug ("num active recordings: {}", num_active_recordings_);
          }
          break;
        case RecordingEvent::Type::StartAutomationRecording:
          z_info ("-------- START AUTOMATION RECORDING");
          {
            auto tr = TRACKLIST->find_track_by_name_hash<AutomatableTrack> (
              ev->track_name_hash_);
            auto &at =
              tr->automation_tracklist_->ats_[ev->automation_track_idx_];
            z_return_val_if_fail (at, G_SOURCE_REMOVE);
            if (!at->recording_started_)
              {
                handle_start_recording (*ev, true);
              }
            at->recording_started_ = true;
          }
          z_debug ("num active recordings: {}", num_active_recordings_);
          break;
        }

return_to_pool:
      event_obj_pool_.release (ev);
    }

  currently_processing_ = false;

  return G_SOURCE_CONTINUE;
}

RecordingManager::RecordingManager ()
{
  recorded_ids_.reserve (8000);

  const size_t max_events = ZRYTHM_TESTING || ZRYTHM_BENCHMARKING ? 400 : 10000;
  event_obj_pool_.reserve (max_events);
  event_queue_.reserve (max_events);

  source_id_ =
    g_timeout_add (12, (GSourceFunc) process_events_source_func, this);
}

RecordingManager::~RecordingManager ()
{
  z_info ("{}: Freeing...", __func__);

  freeing_ = true;

  /* stop source func */
  g_source_remove_and_zero (source_id_);

  /* process pending events */
  process_events ();

  z_info ("{}: done", __func__);
}
