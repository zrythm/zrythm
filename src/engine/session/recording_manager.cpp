// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "dsp/file_audio_source.h"
#include "dsp/transport.h"
#include "engine/session/recording_event.h"
#include "engine/session/recording_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/arrangement/arranger_object.h"
#include "structure/arrangement/arranger_object_span.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/automation_region.h"
#include "structure/arrangement/chord_region.h"
#include "structure/tracks/track_all.h"
#include "structure/tracks/tracklist.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/gtest_wrapper.h"
#include "utils/math.h"
#include "utils/midi.h"
#include "utils/mpmc_queue.h"
#include "utils/object_pool.h"

using namespace zrythm::structure::arrangement;
using namespace zrythm::structure::tracks;

namespace zrythm::engine::session
{

MidiNote *
RecordingManager::pop_unended_note (MidiRegion &mr, int pitch)
{
  auto it = std::ranges::find_if (
    unended_notes_per_region_.at (mr.get_uuid ()),
    [pitch] (const auto mn) { return pitch == -1 || mn->pitch () == pitch; });

  if (it != unended_notes_per_region_.at (mr.get_uuid ()).end ())
    {
      MidiNote * mn = *it;
      unended_notes_per_region_.at (mr.get_uuid ()).erase (it);
      return mn;
    }

  return nullptr;
}

void
RecordingManager::start_unended_note (
  MidiRegion           &mr,
  double                start_pos,
  std::optional<double> end_pos,
  int                   pitch,
  int                   vel)
{
  /* set end pos */
  const auto real_end_pos = end_pos ? *end_pos : start_pos + 1;

  auto mn_builder = PROJECT->arrangerObjectFactory ()->get_builder<MidiNote> ();
  auto mn =
    mn_builder.with_start_ticks (start_pos)
      .with_end_ticks (real_end_pos)
      .with_pitch (pitch)
      .with_velocity (vel)
      .build_in_registry ();
  mr.add_object (mn);

  /* add to unended notes */
  unended_notes_per_region_.at (mr.get_uuid ())
    .push_back (std::get<MidiNote *> (mn.get_object ()));
}

std::optional<structure::arrangement::ArrangerObjectPtrVariant>
RecordingManager::get_recording_region_for_track (
  const structure::tracks::Track::Uuid &track_id) const
{
  auto it = recording_region_per_track_.find (track_id);
  if (it == recording_region_per_track_.end ())
    return std::nullopt;

  return PROJECT->get_arranger_object_registry ().find_by_id_or_throw (
    (*it).second);
}

void
RecordingManager::get_aps_since_last_recorded (
  const structure::arrangement::AutomationRegion &ar,
  signed_frame_t                                  pos,
  std::vector<AutomationPoint *>                 &aps) const
{
  aps.clear ();

  if (!last_recorded_aps_per_region_.contains (ar.get_uuid ()))
    return;

  const auto &last_recorded_ap =
    last_recorded_aps_per_region_.at (ar.get_uuid ());
  if (pos <= last_recorded_ap->position ()->samples ())
    return;

  for (auto * ap : ar.get_children_view ())
    {
      if (
        ap->position ()->samples () > last_recorded_ap->position ()->samples ()
        && ap->position ()->samples () <= pos)
        {
          aps.push_back (ap);
        }
    }
}

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
  // TODO: selection logic
  // auto prev_selections = TL_SELECTIONS->clone_unique ();

  /* select all the recorded regions */
  // TODO: selection logic
  // TL_SELECTIONS->clear (false);

// TODO
#if 0
  for (
    const auto &region_var : ArrangerObjectSpan{
      PROJECT->get_arranger_object_registry (), recorded_ids_ })
    {
      std::visit (
        [&] (auto &&region) {
          using ObjT = base_type<decltype (region)>;
          if constexpr (RegionObject<ObjT>)
            {
              if (is_automation != std::is_same_v<ObjT, AutomationRegion>)
                return;

              auto selection_mgr =
                PROJECT->getArrangerObjectFactory ()
                  ->get_selection_manager_for_object (*region);
              selection_mgr.append_to_selection (region->get_uuid ());
              if (is_automation)
                {
                  if constexpr (std::is_same_v<ObjT, AutomationRegion>)
                    {
                      last_recorded_aps_per_region_.erase (region->get_uuid ());
                    }
                }
            }
        },
        region_var);
    }
#endif

  /* perform the create action */
  try
    {
// FIXME: refactor and reenable
#if 0
      UNDO_MANAGER->perform (
        new gui::actions::ArrangerSelectionsAction::RecordAction (
          *dynamic_cast<TimelineSelections *> (selections_before_start_.get ()),
          *TL_SELECTIONS, true));
#endif
    }
  catch (const ZrythmException &ex)
    {
      ex.handle ("Failed to create recorded regions");
    }

  /* update frame caches and write audio clips to pool */
  for (
    const auto &region_var : ArrangerObjectSpan{
      PROJECT->get_arranger_object_registry (), recorded_ids_ })
    {
      std::visit (
        [&] (auto &&r) {
          using ObjT = base_type<decltype (r)>;
          if constexpr (std::is_same_v<ObjT, AudioRegion>)
            {
// TODO
#if 0
              FileAudioSource * clip = r->get_clip ();
              try
                {
                  AUDIO_POOL->write_clip (*clip, true, false);
                  clip->finalize_buffered_write ();
                }
              catch (const ZrythmException &ex)
                {
                  ex.handle ("Failed to write audio region clip to pool");
                }
#endif
            }
        },
        region_var);
    }

/* TODO: restore the selections */
#if 0
  TL_SELECTIONS->clear (false);
  for (auto &obj_var : prev_selections->objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjT = base_type<decltype (obj)>;
          auto found_obj_var = obj->find_in_project ();
          z_return_if_fail (found_obj_var);
          auto * found_obj = std::get<ObjT *> (found_obj_var.value ());
          found_obj->select (true, true, false);
        },
        obj_var);
    }

  /* free the temporary selections */
  selections_before_start_.reset ();
#endif

  /* disarm transport record button */
  TRANSPORT->setRecordEnabled (false);

  num_active_recordings_--;
  recorded_ids_.clear ();
  z_return_if_fail (num_active_recordings_ == 0);
}

void
RecordingManager::handle_recording (
  structure::tracks::TrackPtrVariant track_var,
  const EngineProcessTimeInfo       &time_nfo,
  const dsp::MidiEventVector *       midi_events,
  std::optional<structure::tracks::TrackProcessor::ConstStereoPortPair>
    stereo_ports)
{
#if 0
  z_info (
    "handling recording from {} ({} frames)",
    g_start_frames + ev->local_offset, ev->nframes);
#endif

  /* whether to skip adding track recording events */
  bool skip_adding_track_events = false;

  /* whether to skip adding automation recording events */
  bool skip_adding_automation_events = false;

  /* whether we are inside the punch range in punch mode or true if otherwise */
  bool inside_punch_range = false;

  z_return_if_fail_cmp (
    time_nfo.local_offset_ + time_nfo.nframes_, <=,
    AUDIO_ENGINE->get_block_length ());

  if (TRANSPORT->punchEnabled ())
    {
      inside_punch_range = TRANSPORT->position_is_inside_punch_range (
        units::samples (time_nfo.g_start_frame_w_offset_));
    }
  else
    {
      inside_punch_range = true;
    }

  /* ---- handle start/stop/pause recording events ---- */
  std::visit (
    [&] (auto &&tr) {
      using TrackT = base_type<decltype (tr)>;

      if (tr->recordableTrackMixin ())
        {
          /*  if not recording at all (recording stopped) */
          if (
            !TRANSPORT->recordEnabled ()
            || !tr->recordableTrackMixin ()->recording ()
            || !TRANSPORT->isRolling ())
            {
              /* if track had previously recorded */
              if (
                recording_region_per_track_.contains (tr->get_uuid ())
                && !std::ranges::contains (
                  tracks_recording_stop_was_sent_to_, tr->get_uuid ()))
                {
                  tracks_recording_stop_was_sent_to_.emplace (tr->get_uuid ());

                  /* send stop recording event */
                  auto re = event_obj_pool_.acquire ();
                  re->init (
                    RecordingEvent::Type::StopTrackRecording, *tr, time_nfo);
                  event_queue_.push_back (re);
                }
              skip_adding_track_events = true;
            }
          /* if pausing */
          else if (time_nfo.nframes_ == 0)
            {
              if (
                recording_region_per_track_.contains (tr->get_uuid ())
                || std::ranges::contains (
                  tracks_recording_start_was_sent_to_, tr->get_uuid ()))
                {
                  /* send pause event */
                  auto re = event_obj_pool_.acquire ();
                  re->init (
                    RecordingEvent::Type::PauseTrackRecording, *tr, time_nfo);
                  event_queue_.push_back (re);

                  skip_adding_track_events = true;
                }
            }
          /* if recording and inside punch range */
          else if (inside_punch_range)
            {
              /* if no recording started yet */
              if (
                !recording_region_per_track_.contains (tr->get_uuid ())
                && !std::ranges::contains (
                  tracks_recording_start_was_sent_to_, tr->get_uuid ()))
                {
                  tracks_recording_start_was_sent_to_.emplace (tr->get_uuid ());

                  /* send start recording event */
                  auto re = event_obj_pool_.acquire ();
                  re->init (
                    RecordingEvent::Type::StartTrackRecording, *tr, time_nfo);
                  event_queue_.push_back (re);
                }
            }
          else if (!inside_punch_range)
            {
              skip_adding_track_events = true;
            }
        }

// TODO
#if 0
      const auto &ats_in_record_mode = ats_in_record_mode_.at (tr->get_uuid ());
      for (auto at : ats_in_record_mode)
        {
          bool at_should_be_recording = at->should_be_recording (false);

          /* if should stop automation recording */
          if (
            Q_UNLIKELY (at->recording_started_)
            && (!TRANSPORT->isRolling () || !at_should_be_recording))
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
          else if (Q_UNLIKELY (at->recording_start_sent_ && time_nfo->nframes_ == 0) && (time_nfo->g_start_frame_w_offset_ == static_cast<unsigned_frame_t> (TRANSPORT->loop_end_pos_->getFrames())))
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
          if (TRANSPORT->isRolling () && at_should_be_recording)
            {
              /* if recording hasn't started yet */
              if (!at->recording_started_ && !at->recording_start_sent_)
                {
                  at->recording_start_sent_ = true;

                  /* send start recording event */
                  auto re = event_obj_pool_.acquire ();
                  re->init (
                    RecordingEvent::Type::StartAutomationRecording, *tr,
                    *time_nfo);
                  re->automation_track_idx_ = at->index_;
                  event_queue_.push_back (re);
                }
            }
        }
#endif

      /* ---- end handling start/stop/pause recording events ---- */

      if (!skip_adding_track_events)
        {
          /* add recorded track material to event queue */
          if (tr->pianoRollTrackMixin () || std::is_same_v<TrackT, ChordTrack>)
            {
              assert (midi_events != nullptr);
              for (const auto &me : *midi_events)
                {
                  auto re = event_obj_pool_.acquire ();
                  re->init (RecordingEvent::Type::Midi, *tr, time_nfo);
                  re->has_midi_event_ = true;
                  re->midi_event_ = me;
                  event_queue_.push_back (re);
                }

              if (midi_events->empty ())
                {
                  auto re = event_obj_pool_.acquire ();
                  re->init (RecordingEvent::Type::Midi, *tr, time_nfo);
                  re->has_midi_event_ = false;
                  event_queue_.push_back (re);
                }
            }
          else if (std::is_same_v<TrackT, AudioTrack>)
            {
              assert (stereo_ports.has_value ());
              auto re = event_obj_pool_.acquire ();
              re->init (RecordingEvent::Type::Audio, *tr, time_nfo);
              utils::float_ranges::copy (
                &re->lbuf_[time_nfo.local_offset_],
                &stereo_ports->first[time_nfo.local_offset_], time_nfo.nframes_);
              utils::float_ranges::copy (
                &re->rbuf_[time_nfo.local_offset_],
                &stereo_ports->second[time_nfo.local_offset_],
                time_nfo.nframes_);
              event_queue_.push_back (re);
            }
        }

      if (skip_adding_automation_events)
        return;

      /* add automation events */

      if (!TRANSPORT->isRolling ())
        return;

// TODO
#if 0
      for (auto at : atl->get_automation_tracks_in_record_mode ())
        {
          /* only proceed if automation should be recording */

          if (!at->recording_start_sent_) [[likely]]
            continue;

          if (!at->should_be_recording (false))
            continue;

          /* send recording event */
          auto re = event_obj_pool_.acquire ();
          re->init (RecordingEvent::Type::Automation, *tr, *time_nfo);
          re->automation_track_idx_ = at->index_;
          event_queue_.push_back (re);
        }
#endif
    },
    track_var);
}

void
RecordingManager::delete_automation_points (
  AutomationTrack * at,
  AutomationRegion &region,
  signed_frame_t    pos)
{
// TODO
#if 0
  region.get_aps_since_last_recorded (pos, pending_aps_);
  for (AutomationPoint * ap : pending_aps_)
    {
      region.remove_object (ap->get_uuid ());
    }

  /* create a new automation point at the pos with the previous value */
  if (region.last_recorded_ap_)
    {
      /* remove the last recorded AP if its previous AP also has the same value */
      AutomationPoint * ap_before_recorded =
        region.get_prev_ap (*region.last_recorded_ap_);
      float prev_fvalue = region.last_recorded_ap_->fvalue_;
      if (
        ap_before_recorded
        && utils::math::floats_equal (
          ap_before_recorded->fvalue_, region.last_recorded_ap_->fvalue_))
        {
          region.remove_object (region.last_recorded_ap_->get_uuid ());
        }

      Position adj_pos = pos;
      adj_pos.add_ticks (
        -region.get_position ().ticks_, AUDIO_ENGINE->frames_per_tick_);
      auto * ap = PROJECT->getArrangerObjectFactory ()->addAutomationPoint (
        &region, adj_pos.ticks_, prev_fvalue);
      region.last_recorded_ap_ = ap;
    }
#endif
}

AutomationPoint *
RecordingManager::create_automation_point (
  AutomationTrack * at,
  AutomationRegion &region,
  float             val,
  float             normalized_val,
  signed_frame_t    pos_frames)
{
  get_aps_since_last_recorded (region, pos_frames, pending_aps_);
  for (AutomationPoint * ap : pending_aps_)
    {
      region.remove_object (ap->get_uuid ());
    }

  auto adj_pos = pos_frames - region.position ()->samples ();
  if (
    last_recorded_aps_per_region_.contains (region.get_uuid ())
    && utils::math::floats_equal (
      last_recorded_aps_per_region_[region.get_uuid ()]->value (), normalized_val)
    && last_recorded_aps_per_region_[region.get_uuid ()]->position ()->samples ()
         == adj_pos)
    {
      /* this block is used to avoid duplicate automation points */
      /* TODO this shouldn't happen and needs investigation */
      return nullptr;
    }
  else
    {
// TODO
#if 0
      auto * ap = PROJECT->arrangerObjectCreator ()->addAutomationPoint (
        &region,
        region.get_tempo_map ()
          .samples_to_tick (units::samples (static_cast<double> (adj_pos)))
          .in (units::ticks),
        normalized_val);
      ap->curveOpts ()->setCurviness (1.0);
      ap->curveOpts ()->setAlgorithm (dsp::CurveOptions::Algorithm::Pulse);
      last_recorded_aps_per_region_.insert_or_assign (region.get_uuid (), ap);
      return ap;
#endif
    }

  return nullptr;
}

void
RecordingManager::handle_pause_event (const RecordingEvent &ev)
{
  auto tr_var = *TRACKLIST->get_track (ev.track_uuid_);
  std::visit (
    [&] (auto &&tr) {
      using TrackT = base_type<decltype (tr)>;
      if (tr->automationTracklist ())
        {

          /* position to pause at */
          [[maybe_unused]] auto pause_pos =
            (signed_frame_t) ev.g_start_frame_w_offset_;

#if 0
  z_debug ("track {} pause start frames {}, nframes {}", tr->name_.c_str(), pause_pos.frames_, ev.nframes_);
#endif

          if (ev.type_ == RecordingEvent::Type::PauseTrackRecording)
            {
              if constexpr (RecordableTrack<TrackT>)
                {
                  tracks_recording_was_paused_.emplace (tr->get_uuid ());

                  /* get the recording region */
                  [[maybe_unused]] auto region_var =
                    recording_region_per_track_.at (tr->get_uuid ());

// TODO
#if 0
                  std::visit (
                    [&] (auto &&r) {
                      using RegionT = base_type<decltype (r)>;
                      if constexpr (std::derived_from<RegionT, LaneOwnedObject>)
                        if constexpr (std::derived_from<TrackT, LanedTrack>)
                          {
                            /* remember lane index */
                            tr->last_lane_idx_ =
                              r->get_lane ().get_index_in_track ();

                            if constexpr (std::is_same_v<RegionT, MidiRegion>)
                              {
                                /* add midi note offs at the end */
                                MidiNote * mn;
                                while ((mn = pop_unended_note (r, -1)))
                                  {
                                    mn->end_position_setter_validated (
                                      pause_pos, AUDIO_ENGINE->ticks_per_frame_);
                                  }
                              }
                          }
                    },
                    region_var);
#endif
                }
              else
                {
                  z_error ("track {} is not recordable", tr->get_name ());
                }
            }
          else if (ev.type_ == RecordingEvent::Type::PauseAutomationRecording)
            {
// TODO
#if 0
              auto at = tr->get_automation_tracklist ().get_automation_track_at (
                ev.automation_track_idx_);
              at->recording_paused_ = true;
#endif
            }
        }
      else
        {
          z_error ("track {} is not automatable", tr->get_name ());
        }
    },
    tr_var);
}

bool
RecordingManager::handle_resume_event (const RecordingEvent &ev)
{
  return true;
  // TODO
#if 0
  auto tr_var = *TRACKLIST->get_track (ev.track_uuid_);
  return std::visit (
    [&] (auto &&tr) {
      using TrackT = base_type<decltype (tr)>;

      /* position to resume from */
      Position resume_pos (
        (signed_frame_t) ev.g_start_frame_w_offset_,
        AUDIO_ENGINE->ticks_per_frame_);

      /* position 1 frame afterwards */
      Position end_pos = resume_pos;
      end_pos.add_frames (1, AUDIO_ENGINE->ticks_per_frame_);

      if constexpr (std::derived_from<TrackT, RecordableTrack>)
        {
          if (
            ev.type_ == RecordingEvent::Type::Midi
            || ev.type_ == RecordingEvent::Type::Audio)
            {
              /* not paused, nothing to do */
              if (!tr->recording_paused_)
                {
                  return false;
                }

              tr->recording_paused_ = false;
              auto recording_region_optvar = tr->get_recording_region ();

              if (
                TRANSPORT->recording_mode_ == Transport::RecordingMode::Takes
                || TRANSPORT->recording_mode_ == Transport::RecordingMode::TakesMuted
                || ev.type_ == RecordingEvent::Type::Audio)
                {
                  /* mute the previous region */
                  if (
                    (TRANSPORT->recording_mode_
                       == Transport::RecordingMode::TakesMuted
                     || (TRANSPORT->recording_mode_ == Transport::RecordingMode::OverwriteEvents && ev.type_ == RecordingEvent::Type::Audio))
                    && recording_region_optvar)
                    {
                      std::visit (
                        [&] (auto &&r) {
                          if constexpr (RegionObject<base_type<decltype (r)>>)
                            {
                              r->mute ()->setMuted (true);
                            }
                        },
                        *recording_region_optvar);
                    }

                  auto success = [&] () {
                    std::optional<ArrangerObjectUuid> added_region_id;
                    if constexpr (std::is_same_v<TrackT, ChordTrack>)
                      {
                        try
                          {
                            auto * added_region =
                              PROJECT->getArrangerObjectFactory ()
                                ->addEmptyChordRegion (tr, resume_pos.ticks_);
                            added_region->set_end_pos_full_size (
                              end_pos, AUDIO_ENGINE->frames_per_tick_);
                            added_region_id.emplace (added_region->get_uuid ());
                          }
                        catch (const ZrythmException &ex)
                          {
                            ex.handle ("Failed to add region to track");
                            return false;
                          }
                      }
                    else if constexpr (std::derived_from<TrackT, LanedTrack>)
                      {
                        using RegionT = TrackT::RegionT;
                        using TrackLaneT = TrackT::TrackLaneType;
                        /* start new region in new lane */
                        int new_lane_pos = tr->last_lane_idx_ + 1;
                        // int idx_inside_lane =
                        //   (int) tr->lanes_.size () > new_lane_pos
                        //     ? std::get<TrackLaneT *> (tr->lanes_.at
                        //     (new_lane_pos))
                        //         ->region_list_->regions_.size ()
                        //     : 0;
                        TrackLaneT &lane = tr->get_lane_at (new_lane_pos);
                        try
                          {
                            if constexpr (std::is_same_v<RegionT, MidiRegion>)
                              {
                                auto * new_region =
                                  PROJECT->getArrangerObjectFactory ()
                                    ->addEmptyMidiRegion (
                                      &lane, resume_pos.ticks_);
                                new_region->set_end_pos_full_size (
                                  end_pos, AUDIO_ENGINE->frames_per_tick_);
                                added_region_id = new_region->get_uuid ();
                              }
                            else if constexpr (
                              std::is_same_v<RegionT, AudioRegion>)
                              {
                                const auto name = gen_name_for_recording_clip (
                                  tr->get_name (), new_lane_pos);
                                auto * new_region =
                                  PROJECT->getArrangerObjectFactory ()
                                    ->add_empty_audio_region_for_recording (
                                      lane, 2, name, resume_pos.ticks_);
                                added_region_id = new_region->get_uuid ();
                              }
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

                    if constexpr (std::derived_from<TrackT, RecordableTrack>)
                      {
                        /* remember region */
                        recorded_ids_.push_back (added_region_id.value ());
                        tr->recording_region_ = added_region_id.value ();
                      }

                    return true;
                  }();

                  if (!success)
                    {
                      return false;
                    }
                }
              /* if MIDI and overwriting or merging events */
              else if (tr->recording_region_)
                {
                  /* extend the previous region */
                  auto region_var = tr->get_recording_region ().value ();
                  std::visit (
                    [&] (auto &&region) {
                      if constexpr (
                        RegionWithChildren<base_type<decltype (region)>>)
                        {
                          if (resume_pos < region->get_position ())
                            {
                              const double ticks_delta =
                                region->get_position ().ticks_
                                - resume_pos.ticks_;
                              region->set_start_pos_full_size (
                                resume_pos, AUDIO_ENGINE->frames_per_tick_);
                              region->add_ticks_to_children (
                                ticks_delta, AUDIO_ENGINE->frames_per_tick_);
                            }
                          if (end_pos > *region->end_pos_)
                            {
                              region->set_end_pos_full_size (
                                end_pos, AUDIO_ENGINE->frames_per_tick_);
                            }
                        }
                    },
                    region_var);
                }
            }
          else if (ev.type_ == RecordingEvent::Type::Automation)
            {
              if constexpr (AutomatableTrack<TrackT>)
                {
                  auto * at = tr->automation_tracklist_->get_automation_track_at (
                    ev.automation_track_idx_);
                  z_return_val_if_fail (at, false);

                  /* not paused, nothing to do */
                  if (!at->recording_paused_)
                    return false;

                  auto port_var = PROJECT->find_port_by_id (at->port_id_);
                  z_return_val_if_fail (
                    port_var && std::holds_alternative<ControlPort *> (*port_var),
                    false);
                  auto * port = std::get<ControlPort *> (*port_var);
                  float  value = port->get_control_value (false);
                  float  normalized_value = port->get_control_value (true);

                  /* get or start new region at resume pos */
                  auto new_region =
                    at->get_region_before_pos (resume_pos, true, false);
                  if (!new_region && at->should_be_recording (false))
                    {
                      /* create region */
                      try
                        {
                          auto ret =
                            PROJECT->getArrangerObjectFactory ()
                              ->addEmptyAutomationRegion (at, resume_pos.ticks_);
                          ret->set_end_pos_full_size (
                            end_pos, AUDIO_ENGINE->frames_per_tick_);
                          new_region = ret;
                        }
                      catch (const ZrythmException &e)
                        {
                          e.handle ("Failed to add region to track");
                          return false;
                        }
                    }
                  z_return_val_if_fail (new_region, false);
                  recorded_ids_.push_back (new_region->get_uuid ());

                  if (at->should_be_recording (true))
                    {
                      while (
                        new_region->get_children_vector ().size () > 0
                        && new_region->get_children_view ().front ()->get_position ()
                             == resume_pos)
                        {
                          new_region->remove_object (
                            new_region->get_children_vector ().front ().id ());
                        }

                      /* create/replace ap at loop start */
                      create_automation_point (
                        at, *new_region, value, normalized_value, resume_pos);
                    }
                }
              else
                {
                  z_return_val_if_reached (false);
                }
            }
        }

      return true;
    },
    tr_var);
#endif
}

void
RecordingManager::handle_audio_event (const RecordingEvent &ev)
{
// TODO
#if 0
  bool handled_resume = handle_resume_event (ev);
  (void) handled_resume;
  /*z_debug ("handled resume {}", handled_resume);*/

  auto * tr = std::get<AudioTrack *> (*TRACKLIST->get_track (ev.track_uuid_));
  /* get end position */
  unsigned_frame_t start_frames = ev.g_start_frame_w_offset_;
  unsigned_frame_t end_frames = start_frames + ev.nframes_;

  Position start_pos, end_pos;
  start_pos.from_frames (
    (signed_frame_t) start_frames, AUDIO_ENGINE->ticks_per_frame_);
  end_pos.from_frames (
    (signed_frame_t) end_frames, AUDIO_ENGINE->ticks_per_frame_);

  /* get the recording region */
  auto region = std::get<AudioRegion *> (tr->get_recording_region ().value ());

  /* the clip */
  auto clip = region->get_clip ();

  /* set region end pos */
  region->set_end_pos_full_size (end_pos, AUDIO_ENGINE->frames_per_tick_);

  signed_frame_t r_obj_len_frames =
    (region->get_end_position ().frames_ - region->get_position ().frames_);
  z_return_if_fail_cmp (r_obj_len_frames, >=, 0);

  region->loop_end_pos_.from_frames (
    region->get_end_position ().frames_ - region->get_position ().frames_,
    AUDIO_ENGINE->ticks_per_frame_);
  region->fade_out_pos_ = region->loop_end_pos_;

  /* handle the samples normally */
  utils::audio::AudioBuffer buf_to_append{
    clip->get_num_channels (), (int) ev.nframes_
  };
  buf_to_append.copyFrom (
    0, 0, ev.lbuf_.data (), static_cast<int> (ev.nframes_));
  buf_to_append.copyFrom (
    1, 0, ev.rbuf_.data (), static_cast<int> (ev.nframes_));
  clip->expand_with_frames (buf_to_append);

  /* write to pool if enough time passed since last write */
  if (clip->enough_time_elapsed_since_last_write ())
    {
      try
        {
          AUDIO_POOL->write_clip (*clip, true, false);
        }
      catch (const ZrythmException &ex)
        {
          ex.handle ("Failed to write audio clip to pool");
        }
    }
#endif
}

void
RecordingManager::handle_midi_event (const RecordingEvent &ev)
{
// TODO
#if 0
  handle_resume_event (ev);

  auto tr_var = *TRACKLIST->get_track (ev.track_uuid_);
  std::visit (
    [&] (auto &&tr) {
      using TrackT = base_type<decltype (tr)>;
      if constexpr (std::derived_from<TrackT, RecordableTrack>)
        {
          z_return_if_fail (tr->recording_region_);

          Position start_pos, end_pos;
          start_pos.from_frames (
            (signed_frame_t) ev.g_start_frame_w_offset_,
            AUDIO_ENGINE->ticks_per_frame_);
          end_pos.from_frames (
            (signed_frame_t) ev.g_start_frame_w_offset_
              + (signed_frame_t) ev.nframes_,
            AUDIO_ENGINE->ticks_per_frame_);

          std::visit (
            [&] (auto &&region) {
              using RegionT = base_type<decltype (region)>;

              if constexpr (
                std::derived_from<RegionT, structure::arrangement::Region>)
                {
                  /* set region end pos */
                  bool set_end_pos = false;
                  switch (TRANSPORT->recording_mode_)
                    {
                    case Transport::RecordingMode::OverwriteEvents:
                    case Transport::RecordingMode::MergeEvents:
                      if (*region->end_pos_ < end_pos)
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
                      region->set_end_pos_full_size (
                        end_pos, AUDIO_ENGINE->frames_per_tick_);
                    }

                  tr->recording_region_ = region->get_uuid ();

                  /* get local positions */
                  Position local_pos = start_pos;
                  Position local_end_pos = end_pos;
                  local_pos.add_ticks (
                    -region->get_position ().ticks_,
                    AUDIO_ENGINE->frames_per_tick_);
                  local_end_pos.add_ticks (
                    -region->get_position ().ticks_,
                    AUDIO_ENGINE->frames_per_tick_);

                  /* if overwrite mode, clear any notes inside the range */
                  if (
                    TRANSPORT->recording_mode_
                    == Transport::RecordingMode::OverwriteEvents)
                    {
                      if constexpr (std::is_same_v<RegionT, MidiRegion>)
                        {
                          for (
                            const auto &mn :
                            region->get_children_view () | std::views::reverse)
                            {
                              if (
                                mn->is_hit_by_range (
                                  local_pos, local_end_pos, false, false, true))
                                {
                                  region->remove_object (mn->get_uuid ());
                                }
                            }
                        }
                    }

                  if (!ev.has_midi_event_)
                    return;

                  /* convert MIDI data to midi notes */
                  MidiNote *  mn;
                  const auto &mev = ev.midi_event_;
                  const auto &buf = mev.raw_buffer_;

                  if constexpr (std::is_same_v<RegionT, ChordRegion>)
                    {
                      if (utils::midi::midi_is_note_on (buf))
                        {
                          const midi_byte_t note_number =
                            utils::midi::midi_get_note_number (buf);
                          const dsp::ChordDescriptor * descr =
                            CHORD_EDITOR->get_chord_from_note_number (
                              note_number);
                          z_return_if_fail (descr);
                          const int chord_idx =
                            CHORD_EDITOR->get_chord_index (*descr);
                          auto co =
                            PROJECT->getArrangerObjectFactory ()->addChordObject (
                              region, local_pos.ticks_, chord_idx);
                          co->set_position_unvalidated (local_pos);
                        }
                    }
                  /* else if not chord track */
                  else if constexpr (std::is_same_v<RegionT, MidiRegion>)
                    {
                      if (utils::midi::midi_is_note_on (buf))
                        {
                          start_unended_note (
                            region, &local_pos, &local_end_pos,
                            utils::midi::midi_get_note_number (buf),
                            utils::midi::midi_get_velocity (buf), true);
                        }
                      else if (utils::midi::midi_is_note_off (buf))
                        {
                          mn = pop_unended_note (
                            region, utils::midi::midi_get_note_number (buf));
                          if (mn)
                            {
                              mn->end_position_setter_validated (
                                local_end_pos, AUDIO_ENGINE->ticks_per_frame_);
                            }
                        }
                      else
                        {
                          /* TODO */
                        }
                    }
                }
            },
            tr->get_recording_region ().value ());
        }
      else
        {
          z_return_if_reached ();
        }
    },
    tr_var);
#endif
}

void
RecordingManager::handle_automation_event (const RecordingEvent &ev)
{
// TODO
#if 0
  handle_resume_event (ev);

  auto tr_var = *TRACKLIST->get_track (ev.track_uuid_);
  std::visit (
    [&] (auto &&tr) {
      using TrackT = base_type<decltype (tr)>;
      if constexpr (AutomatableTrack<TrackT>)
        {
          auto  &atl = tr->automation_tracklist_;
          auto * at = atl->get_automation_track_at (ev.automation_track_idx_);
          auto   port_var = PROJECT->find_port_by_id (at->port_id_);
          z_return_if_fail (
            port_var.has_value ()
            && std::holds_alternative<ControlPort *> (port_var.value ()));
          auto * port = std::get<ControlPort *> (port_var.value ());
          float  value = port->get_control_value (false);
          float  normalized_value = port->get_control_value (true);
          if (ZRYTHM_TESTING)
            {
              utils::math::assert_nonnann (value);
              utils::math::assert_nonnann (normalized_value);
            }
          bool automation_value_changed =
            !port->value_changed_from_reading_
            && !utils::math::floats_equal (value, at->last_recorded_value_);

          /* get end position */
          unsigned_frame_t start_frames = ev.g_start_frame_w_offset_;
          unsigned_frame_t end_frames = start_frames + ev.nframes_;

          Position start_pos (
            static_cast<signed_frame_t> (start_frames),
            AUDIO_ENGINE->ticks_per_frame_);
          Position end_pos (
            static_cast<signed_frame_t> (end_frames),
            AUDIO_ENGINE->ticks_per_frame_);

          bool new_region_created = false;

          /* get the recording region */
          auto region = at->get_region_before_pos (start_pos, true, false);

          auto region_at_end = at->get_region_before_pos (end_pos, true, false);
          if (!region && automation_value_changed)
            {
              z_debug (
                "creating new automation region (automation value changed)");
              /* create region */
              Position pos_to_end_new_r;
              if (region_at_end)
                {
                  pos_to_end_new_r = region_at_end->get_position ();
                }
              else
                {
                  pos_to_end_new_r = end_pos;
                }
              region =
                PROJECT->getArrangerObjectFactory ()
                  ->addEmptyAutomationRegion (at, start_pos.ticks_);
              region->set_end_pos_full_size (
                pos_to_end_new_r, AUDIO_ENGINE->frames_per_tick_);
              new_region_created = true;
              z_return_if_fail (region);

              recorded_ids_.push_back (region->get_uuid ());
            }

          at->recording_region_ = region;

          if (new_region_created || (region && *region->end_pos_ < end_pos))
            {
              /* set region end pos */
              region->set_end_pos_full_size (
                end_pos, AUDIO_ENGINE->frames_per_tick_);
            }

          at->recording_region_ = region;

          /* handle the samples normally */
          if (automation_value_changed)
            {
              create_automation_point (
                at, *region, value, normalized_value, start_pos);
              at->last_recorded_value_ = value;
            }
          else if (
            at->record_mode_ == AutomationTrack::AutomationRecordMode::Latch)
            {
              z_return_if_fail (region);
              delete_automation_points (at, *region, start_pos);
            }

          /* if we left touch mode, set last recorded ap to NULL */
          if (
            at->record_mode_ == AutomationTrack::AutomationRecordMode::Touch
            && !at->should_be_recording (true) && at->recording_region_)
            {
              at->recording_region_->last_recorded_ap_ = nullptr;
            }
        }
      else
        {
          z_return_if_reached ();
        }
    },
    tr_var);
#endif
}

void
RecordingManager::handle_start_recording (
  const RecordingEvent &ev,
  bool                  is_automation)
{
// TODO
#if 0
  auto tr_var = *TRACKLIST->get_track (ev.track_uuid_);
  std::visit (
    [&] (auto &&tr) {
      using TrackT = base_type<decltype (tr)>;
      AutomationTrack * at{};
      if (is_automation)
        {
          if constexpr (AutomatableTrack<TrackT>)
            {
              at = tr->automation_tracklist_->get_automation_track_at (
                ev.automation_track_idx_);
            }
          else
            {
              z_return_if_reached ();
            }
        }

      if (num_active_recordings_ == 0)
        {
// TODO
#  if 0
          auto objs = TRACKLIST->get_timeline_objects_in_range ();
          auto obj_span = ArrangerObjectSpan{
            objs
          } | std::views::filter (ArrangerObjectSpan::selected_projection);
          objects_before_start_ =
            ArrangerObjectSpan{ obj_span }.create_snapshots (*PROJECT->getArrangerObjectFactory(), this);
#  endif
        }

      /* this could be called multiple times, ignore if already processed */
      auto * recordable_track = dynamic_cast<RecordableTrack *> (tr);
      if (
        !is_automation && recordable_track->get_recording_region ().has_value ()
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

      Position start_pos (
        static_cast<signed_frame_t> (start_frames),
        AUDIO_ENGINE->ticks_per_frame_);
      Position end_pos (
        static_cast<signed_frame_t> (end_frames),
        AUDIO_ENGINE->ticks_per_frame_);

      if (is_automation)
        {
          /* don't unset recording paused, this will be unset by handle_resume()
           */
          /*at->recording_paused = false;*/

          /* nothing, wait for event to start writing data */
          auto port_var = PROJECT->find_port_by_id (at->port_id_);
          z_return_if_fail (
            port_var.has_value ()
            && std::holds_alternative<ControlPort *> (port_var.value ()));
          auto * port = std::get<ControlPort *> (port_var.value ());
          float  value = port->get_control_value (false);

          if (at->should_be_recording (true))
            {
              /* set recorded value to something else to force the recorder to
               * start writing */
              // z_info ("SHOULD BE RECORDING");
              at->last_recorded_value_ = value + 2.f;
            }
          else
            {
              // z_info ("SHOULD NOT BE RECORDING");
              /** set the current value so that nothing is recorded until it
               * changes
               */
              at->last_recorded_value_ = value;
            }
        }
      else
        {
          recordable_track->recording_paused_ = false;

          try
            {
              if constexpr (std::derived_from<TrackT, PianoRollTrack>)
                {
                  /* create region at last lane */
                  auto region =
                    PROJECT->getArrangerObjectFactory ()->addEmptyMidiRegion (
                      std::get<MidiLane *> (tr->lanes_.back ()),
                      start_pos.ticks_);
                  region->set_end_pos_full_size (
                    end_pos, AUDIO_ENGINE->frames_per_tick_);

                  recordable_track->recording_region_ = region->get_uuid ();
                  recorded_ids_.push_back (region->get_uuid ());
                }
              else if constexpr (std::is_same_v<TrackT, ChordTrack>)
                {
                  auto region =
                    PROJECT->getArrangerObjectFactory ()
                      ->addEmptyChordRegion (tr, start_pos.ticks_);
                  region->set_end_pos_full_size (
                    end_pos, AUDIO_ENGINE->frames_per_tick_);

                  recordable_track->recording_region_ = region->get_uuid ();
                  recorded_ids_.push_back (region->get_uuid ());
                }
              else if constexpr (std::is_same_v<TrackT, AudioTrack>)
                {
                  /* create region */
                  int        new_lane_pos = tr->lanes_.size () - 1;
                  const auto name =
                    gen_name_for_recording_clip (tr->get_name (), new_lane_pos);
                  auto region =
                    PROJECT->getArrangerObjectFactory ()
                      ->add_empty_audio_region_for_recording (
                        *std::get<AudioLane *> (tr->lanes_.back ()), 2, name,
                        start_pos.ticks_);
                  z_return_if_fail (region);

                  recordable_track->recording_region_ = region->get_uuid ();
                  recorded_ids_.push_back (region->get_uuid ());
                }
            }
          catch (const ZrythmException &ex)
            {
              ex.handle ("Failed to add region");
              return;
            }
        }

      ++num_active_recordings_;
    },
    tr_var);
#endif
}

void
RecordingManager::process_events ()
{
  SemaphoreRAII lock (processing_sem_);
  z_return_if_fail (!currently_processing_);
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
            auto tr_var = *TRACKLIST->get_track (ev->track_uuid_);
            std::visit (
              [&] (auto &&tr) {
                using TrackT = base_type<decltype (tr)>;
                if constexpr (RecordableTrack<TrackT>)
                  {
                    z_debug (
                      "-------- STOP TRACK RECORDING ({})", tr->get_name ());
                    handle_stop_recording (false);
                    recording_region_per_track_.erase (tr->get_uuid ());
                    tracks_recording_start_was_sent_to_.erase (tr->get_uuid ());
                    tracks_recording_stop_was_sent_to_.erase (tr->get_uuid ());
                  }
                else
                  {
                    z_return_if_reached ();
                  }
              },
              tr_var);
          }
          z_debug ("num active recordings: {}", num_active_recordings_);
          break;
        case RecordingEvent::Type::StopAutomationRecording:
          z_debug ("-------- STOP AUTOMATION RECORDING");
          {
            auto tr_var = *TRACKLIST->get_track (ev->track_uuid_);
            std::visit (
              [&] (auto &tr) {
                using TrackT = base_type<decltype (tr)>;
                if constexpr (AutomatableTrack<TrackT>)
                  {
// TODO
#if 0
                    auto * at =
                      tr->automation_tracklist_->get_automation_track_at (
                        ev->automation_track_idx_);
                    z_return_if_fail (at);
                    if (at->recording_started_)
                      {
                        handle_stop_recording (true);
                      }
                    at->recording_started_ = false;
                    at->recording_start_sent_ = false;
                    at->recording_region_ = nullptr;
#endif
                  }
                else
                  {
                    z_return_if_reached ();
                  }
              },
              tr_var);
          }
          z_debug ("num active recordings: {}", num_active_recordings_);
          break;
        case RecordingEvent::Type::StartTrackRecording:
          {
            auto tr_var = *TRACKLIST->get_track (ev->track_uuid_);
            std::visit (
              [&] (auto &&tr) {
                z_debug ("-------- START TRACK RECORDING ({})", tr->get_name ());
                handle_start_recording (*ev, false);
                z_debug ("num active recordings: {}", num_active_recordings_);
              },
              tr_var);
          }
          break;
        case RecordingEvent::Type::StartAutomationRecording:
          z_info ("-------- START AUTOMATION RECORDING");
          {
            auto tr_var = *TRACKLIST->get_track (ev->track_uuid_);
            std::visit (
              [&] (auto &tr) {
                using TrackT = base_type<decltype (tr)>;
                if constexpr (AutomatableTrack<TrackT>)
                  {
// TODO
#if 0
                    auto * at =
                      tr->automation_tracklist_->get_automation_track_at (
                        ev->automation_track_idx_);
                    z_return_if_fail (at);
                    if (!at->recording_started_)
                      {
                        handle_start_recording (*ev, true);
                      }
                    at->recording_started_ = true;
#endif
                  }
                else
                  {
                    z_return_if_reached ();
                  }
              },
              tr_var);
            z_debug ("num active recordings: {}", num_active_recordings_);
          }
          break;
        }

return_to_pool:
      event_obj_pool_.release (ev);
    }

  currently_processing_ = false;
}

RecordingManager::RecordingManager (QObject * parent) : QObject (parent)
{
  recorded_ids_.reserve (8000);

  const size_t max_events = ZRYTHM_TESTING || ZRYTHM_BENCHMARKING ? 400 : 10000;
  event_obj_pool_.reserve (max_events);
  event_queue_.reserve (max_events);

  auto * timer = new QTimer (this);
  timer->setInterval (12);
  timer->setSingleShot (false);
  connect (timer, &QTimer::timeout, this, &RecordingManager::process_events);
  timer->start ();
}

RecordingManager::~RecordingManager ()
{
  z_info ("{}: Freeing...", __func__);

  freeing_ = true;

  /* process pending events */
  process_events ();

  z_info ("{}: done", __func__);
}
}
