// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <algorithm>

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/audio_region.h"
#include "gui/dsp/audio_track.h"
#include "gui/dsp/engine.h"
#include "gui/dsp/laned_track.h"
#include "gui/dsp/marker.h"
#include "gui/dsp/marker_track.h"
#include "gui/dsp/tempo_track.h"
#include "gui/dsp/tracklist.h"
#include "gui/dsp/transport.h"
#include "utils/debug.h"
#include "utils/gtest_wrapper.h"
#include "utils/rt_thread_id.h"

using namespace zrythm;

/**
 * A buffer of n bars after the end of the last object.
 */
constexpr int BARS_END_BUFFER = 4;

/** Millisec to allow moving further backward when very close to the calculated
 * backward position. */
constexpr int REPEATED_BACKWARD_MS = 240;

void
Transport::init_common ()
{
  /* set playstate */
  play_state_ = PlayState::Paused;

  const bool testing_or_benchmarking = ZRYTHM_TESTING || ZRYTHM_BENCHMARKING;
  loop_ =
    testing_or_benchmarking ? true : gui::SettingsManager::transportLoop ();
  metronome_enabled_ =
    testing_or_benchmarking ? true : gui::SettingsManager::metronomeEnabled ();
  punch_mode_ =
    testing_or_benchmarking ? true : gui::SettingsManager::punchModeEnabled ();
  start_playback_on_midi_input_ =
    testing_or_benchmarking
      ? false
      : gui::SettingsManager::startPlaybackOnMidiInput ();
  recording_mode_ =
    testing_or_benchmarking
      ? RecordingMode::Takes
      : ENUM_INT_TO_VALUE (RecordingMode, gui::SettingsManager::recordingMode ());
}

void
Transport::init_loaded (Project * project, const TempoTrack * tempo_track)
{
  project_ = project;

  z_return_if_fail_cmp (total_bars_, >, 0);

  init_common ();

  if (tempo_track)
    {
      int beats_per_bar = tempo_track->get_beats_per_bar ();
      int beat_unit = tempo_track->get_beat_unit ();
      update_caches (beats_per_bar, beat_unit);
    }

  roll_->init_loaded (*this);
  stop_->init_loaded (*this);
  backward_->init_loaded (*this);
  forward_->init_loaded (*this);
  loop_toggle_->init_loaded (*this);
  rec_toggle_->init_loaded (*this);
}

Transport::Transport (Project * parent)
    : QObject (parent), playhead_pos_ (new PositionProxy (this, nullptr, true)),
      cue_pos_ (new PositionProxy (this, nullptr, true)),
      loop_start_pos_ (new PositionProxy (this, nullptr, true)),
      loop_end_pos_ (new PositionProxy (this, nullptr, true)),
      punch_in_pos_ (new PositionProxy (this, nullptr, true)),
      punch_out_pos_ (new PositionProxy (this, nullptr, true)),
      project_ (parent), property_notification_timer_ (new QTimer (this))
{
  z_debug ("Creating transport...");

  property_notification_timer_->setInterval (16);
  property_notification_timer_->callOnTimeout (this, [this] () {
    if (needs_property_notification_.exchange (false))
      {
        Q_EMIT playStateChanged (play_state_);
      }
  });
  property_notification_timer_->start ();

  if (parent == nullptr)
    {
      init_common ();
      return;
    }

  /* set initial total number of beats this is applied to the ruler */
  total_bars_ = TRANSPORT_DEFAULT_TOTAL_BARS;

  z_return_if_fail (project_->audio_engine_->sample_rate_ > 0);

  double ticks_per_bar = Position::TICKS_PER_QUARTER_NOTE * 4.0;
  loop_end_pos_->ticks_ = 4 * ticks_per_bar;
  punch_in_pos_->ticks_ = 2 * ticks_per_bar;
  punch_out_pos_->ticks_ = 4 * ticks_per_bar;

  range_1_.ticks_ = 1 * ticks_per_bar;
  range_2_.ticks_ = 1 * ticks_per_bar;

  /*if (utils::math::floats_equal (*/
  /*frames_per_tick_before, 0))*/
  /*{*/
  /*AUDIO_ENGINE->frames_per_tick = 0;*/
  /*}*/

  /* create ports */
  roll_ = std::make_unique<MidiPort> (u8"Roll", PortFlow::Input);
  roll_->id_->sym_ = u8"roll";
  roll_->set_owner (*this);
  roll_->id_->flags_ |= PortIdentifier::Flags::Toggle;
  roll_->id_->flags2_ |= PortIdentifier::Flags2::TransportRoll;

  stop_ = std::make_unique<MidiPort> (u8"Stop", PortFlow::Input);
  stop_->id_->sym_ = u8"stop";
  stop_->set_owner (*this);
  stop_->id_->flags_ |= PortIdentifier::Flags::Toggle;
  stop_->id_->flags2_ |= PortIdentifier::Flags2::TransportStop;

  backward_ = std::make_unique<MidiPort> (u8"Backward", PortFlow::Input);
  backward_->id_->sym_ = u8"backward";
  backward_->set_owner (*this);
  backward_->id_->flags_ |= PortIdentifier::Flags::Toggle;
  backward_->id_->flags2_ |= PortIdentifier::Flags2::TransportBackward;

  forward_ = std::make_unique<MidiPort> (u8"Forward", PortFlow::Input);
  forward_->id_->sym_ = u8"forward";
  forward_->set_owner (*this);
  forward_->id_->flags_ |= PortIdentifier::Flags::Toggle;
  forward_->id_->flags2_ |= PortIdentifier::Flags2::TransportForward;

  loop_toggle_ = std::make_unique<MidiPort> (u8"Loop toggle", PortFlow::Input);
  loop_toggle_->id_->sym_ = u8"loop_toggle";
  loop_toggle_->set_owner (*this);
  loop_toggle_->id_->flags_ |= PortIdentifier::Flags::Toggle;
  loop_toggle_->id_->flags2_ |= PortIdentifier::Flags2::TransportLoopToggle;

  rec_toggle_ = std::make_unique<MidiPort> (u8"Rec toggle", PortFlow::Input);
  rec_toggle_->id_->sym_ = u8"rec_toggle";
  rec_toggle_->set_owner (*this);
  rec_toggle_->id_->flags_ |= PortIdentifier::Flags::Toggle;
  rec_toggle_->id_->flags2_ |= PortIdentifier::Flags2::TransportRecToggle;

  init_common ();
}

void
Transport::setLoopEnabled (bool enabled)
{
  if (loop_ == enabled)
    {
      return;
    }

  set_loop (enabled, true);
}

void
Transport::setRecordEnabled (bool enabled)
{
  if (recording_ == enabled)
    {
      return;
    }

  set_recording (enabled, true);
}

Transport::PlayState
Transport::getPlayState () const
{
  return play_state_;
}

void
Transport::setPlayState (PlayState state)
{
  if (play_state_ == state)
    {
      return;
    }

  play_state_ = state;
  Q_EMIT playStateChanged (play_state_);
}

PositionProxy *
Transport::getPlayheadPosition () const
{
  return playhead_pos_;
}

PositionProxy *
Transport::getCuePosition () const
{
  return cue_pos_;
}

PositionProxy *
Transport::getLoopStartPosition () const
{
  return loop_start_pos_;
}

PositionProxy *
Transport::getLoopEndPosition () const
{
  return loop_end_pos_;
}

PositionProxy *
Transport::getPunchInPosition () const
{
  return punch_in_pos_;
}

PositionProxy *
Transport::getPunchOutPosition () const
{
  return punch_out_pos_;
}

void
Transport::moveBackward ()
{
  move_backward (true);
}

void
Transport::moveForward ()
{
  move_forward (true);
}

void
Transport::init_after_cloning (const Transport &other, ObjectCloneType clone_type)
{
  total_bars_ = other.total_bars_;
  has_range_ = other.has_range_;
  position_ = other.position_;

  loop_start_pos_->set_position_rtsafe (*other.loop_start_pos_);
  playhead_pos_->set_position_rtsafe (*other.playhead_pos_);
  loop_end_pos_->set_position_rtsafe (*other.loop_end_pos_);
  cue_pos_->set_position_rtsafe (*other.cue_pos_);
  punch_in_pos_->set_position_rtsafe (*other.punch_in_pos_);
  punch_out_pos_->set_position_rtsafe (*other.punch_out_pos_);
  range_1_ = other.range_1_;
  range_2_ = other.range_2_;

  // Clone ports if they exit using a lambda
  auto clone_port = [] (const auto &port) {
    return port ? port->clone_unique () : nullptr;
  };

  roll_ = clone_port (other.roll_);
  stop_ = clone_port (other.stop_);
  backward_ = clone_port (other.backward_);
  forward_ = clone_port (other.forward_);
  loop_toggle_ = clone_port (other.loop_toggle_);
  rec_toggle_ = clone_port (other.rec_toggle_);
}

void
Transport::set_play_state_rt_safe (PlayState state)
{
  if (play_state_ == state)
    {
      return;
    }

  play_state_ = state;
  needs_property_notification_.store (true);
}

void
Transport::prepare_audio_regions_for_stretch (
  std::optional<ArrangerObjectSpan> sel_var)
{
  if (sel_var)
    {
      for (auto * obj : sel_var->template get_elements_by_type<AudioRegion> ())
        {
          obj->before_length_ = obj->get_length_in_ticks ();
        }
    }
  else
    {
      for (
        auto * track :
        TRACKLIST->get_track_span ().get_elements_by_type<AudioTrack> ())
        {
          for (auto &lane_var : track->lanes_)
            {
              auto * lane = std::get<AudioLane *> (lane_var);
              for (auto * region : lane->get_children_view ())
                {
                  region->before_length_ = region->get_length_in_ticks ();
                }
            }
        }
    }
}

void
Transport::stretch_regions (
  std::optional<ArrangerObjectSpan> sel_var,
  bool                              with_fixed_ratio,
  double                            time_ratio,
  bool                              force)
{
  if (sel_var)
    {
      const auto &sel = *sel_var;
      for (auto * region : sel.template get_elements_derived_from<Region> ())
        {
          auto r_variant = convert_to_variant<RegionPtrVariant> (region);
          std::visit (
            [&] (auto &&r) {
              if constexpr (std::is_same_v<base_type<decltype (r)>, AudioRegion>)
                {
                  /* don't stretch audio regions with musical mode off */
                  if (!r->get_musical_mode () && !force)
                    return;
                }

              double ratio =
                with_fixed_ratio
                  ? time_ratio
                  : r->get_length_in_ticks () / r->before_length_;
              r->stretch (ratio);
            },
            r_variant);
        }
    }
  else
    {
      for (
        auto * track :
        TRACKLIST->get_track_span ().get_elements_by_type<AudioTrack> ())
        {
          for (auto &lane_var : track->lanes_)
            {
              auto * lane = std::get<AudioLane *> (lane_var);
              for (auto * region : lane->get_children_view ())
                {
                  /* don't stretch regions with musical mode off */
                  if (!region->get_musical_mode ())
                    continue;

                  double ratio =
                    with_fixed_ratio
                      ? time_ratio
                      : region->get_length_in_ticks () / region->before_length_;
                  region->stretch (ratio);
                }
            }
        }
    }
}

void
Transport::set_punch_mode_enabled (bool enabled)
{
  punch_mode_ = enabled;

  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      gui::SettingsManager::get_instance ()->set_punchModeEnabled (enabled);
    }
}

void
Transport::set_start_playback_on_midi_input (bool enabled)
{
  start_playback_on_midi_input_ = enabled;
  gui::SettingsManager::get_instance ()->set_startPlaybackOnMidiInput (enabled);
}

void
Transport::set_recording_mode (RecordingMode mode)
{
  recording_mode_ = mode;

  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      gui::SettingsManager::get_instance ()->set_recordingMode (
        ENUM_VALUE_TO_INT (mode));
    }
}

void
Transport::update_caches (int beats_per_bar, int beat_unit)
{
  /**
   * Regarding calculation:
   * 3840 = TICKS_PER_QUARTER_NOTE * 4 to get the ticks per full note.
   * Divide by beat unit (e.g. if beat unit is 2, it means it is a 1/2th note,
   * so multiply 1/2 with the ticks per note
   */
  ticks_per_beat_ = 3840 / beat_unit;
  ticks_per_bar_ = ticks_per_beat_ * beats_per_bar;
  sixteenths_per_beat_ = 16 / beat_unit;
  sixteenths_per_bar_ = (sixteenths_per_beat_ * beats_per_bar);
  z_warn_if_fail (ticks_per_bar_ > 0.0);
  z_warn_if_fail (ticks_per_beat_ > 0.0);
}

void
Transport::requestPause (bool with_wait)
{
  auto * audio_engine_ = project_->audio_engine_.get ();
  /* can only be called from the gtk thread or when preparing to export */
  z_return_if_fail (
    !audio_engine_->run_.load () || ZRYTHM_IS_QT_THREAD
    || audio_engine_->preparing_to_export_);

  if (with_wait)
    {
      audio_engine_->port_operation_lock_.wait ();
    }

  set_play_state_rt_safe (PlayState::PauseRequested);

  playhead_before_pause_ = playhead_pos_->get_position ();
  if (
    !ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING
    && gui::SettingsManager::transportReturnToCue ())
    {
      auto tmp = cue_pos_->get_position ();
      move_playhead (tmp, true, false, true);
    }

  if (with_wait)
    {
      audio_engine_->port_operation_lock_.signal ();
    }
}

void
Transport::requestRoll (bool with_wait)
{
  auto * audio_engine_ = project_->audio_engine_.get ();

  /* can only be called from the gtk thread */
  z_return_if_fail (!audio_engine_->run_.load () || ZRYTHM_IS_QT_THREAD);

  std::optional<SemaphoreRAII<decltype (audio_engine_->port_operation_lock_)>>
    wait_sem;
  if (with_wait)
    {
      wait_sem.emplace (audio_engine_->port_operation_lock_);
    }

  if (gZrythm && !ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      /* handle countin */
      PrerollCountBars bars = ENUM_INT_TO_VALUE (
        PrerollCountBars, gui::SettingsManager::metronomeCountIn ());
      int  num_bars = preroll_count_bars_enum_to_int (bars);
      auto frames_per_bar =
        type_safe::get (audio_engine_->frames_per_tick_)
        * (double) ticks_per_bar_;
      countin_frames_remaining_ = (long) ((double) num_bars * frames_per_bar);
      if (metronome_enabled_)
        {
          SAMPLE_PROCESSOR->queue_metronome_countin ();
        }

      if (recording_)
        {
          /* handle preroll */
          bars = ENUM_INT_TO_VALUE (
            PrerollCountBars, gui::SettingsManager::recordingPreroll ());
          num_bars = preroll_count_bars_enum_to_int (bars);
          Position pos = playhead_pos_->get_position ();
          pos.add_bars (
            -num_bars, ticks_per_bar_, audio_engine_->frames_per_tick_);
          if (!pos.is_positive ())
            pos = Position ();
          preroll_frames_remaining_ = playhead_pos_->frames_ - pos.frames_;
          set_playhead_pos_rt_safe (pos);
#if 0
          z_debug (
            "preroll %ld frames",
            preroll_frames_remaining_);
#endif
        }
    }

  setPlayState (PlayState::RollRequested);
}

void
Transport::add_to_playhead (const signed_frame_t nframes)
{
  playhead_pos_->add_frames_rtsafe (nframes, AUDIO_ENGINE->ticks_per_frame_);
}

void
Transport::set_playhead_pos_rt_safe (const Position pos)
{
  playhead_pos_->set_position_rtsafe (pos);
}

void
Transport::set_playhead_to_bar (int bar)
{
  Position pos;
  pos.set_to_bar (
    bar, ticks_per_bar_, project_->audio_engine_->frames_per_tick_);
  set_playhead_pos_rt_safe (pos);
}

void
Transport::get_playhead_pos (Position * pos)
{
  z_return_if_fail (pos);

  *pos = playhead_pos_->get_position ();
}
bool
Transport::can_user_move_playhead () const
{
  if (
    recording_ && play_state_ == PlayState::Rolling && project_->audio_engine_
    && project_->audio_engine_->run_.load ())
    return false;
  else
    return true;
}

void
Transport::move_playhead (
  const Position &target,
  bool            panic,
  bool            set_cue_point,
  bool            fire_events)
{
  /* if currently recording, do nothing */
  if (!can_user_move_playhead ())
    {
      z_info ("currently recording - refusing to move playhead manually");
      return;
    }

  /* send MIDI note off on currently playing timeline objects */
  for (
    auto * track :
    TRACKLIST->get_track_span ().get_elements_derived_from<LanedTrack> ())
    {
      std::visit (
        [&] (auto &&t) {
          using TrackT = base_type<decltype (t)>;
          for (auto &lane_var : t->lanes_)
            {
              using TrackLaneT = TrackT::LanedTrackImpl::TrackLaneType;
              auto lane = std::get<TrackLaneT *> (lane_var);
              for (auto * region : lane->get_children_view ())
                {
                  if (!region->is_hit (playhead_pos_->frames_, true))
                    continue;

                  if constexpr (
                    std::is_same_v<typename TrackLaneT::RegionT, MidiRegion>)
                    {
                      for (auto * midi_note : region->get_children_view ())
                        {
                          if (midi_note->is_hit (playhead_pos_->frames_))
                            {
                              t->processor_->get_piano_roll_port ()
                                .midi_events_.queued_events_
                                .add_note_off (1, midi_note->pitch_, 0);
                            }
                        }
                    }
                }
            }
        },
        convert_to_variant<LanedTrackPtrVariant> (track));
    }

  /* move to new pos */
  playhead_pos_->set_position_rtsafe (target);

  if (set_cue_point)
    {
      /* move cue point */
      cue_pos_->set_position_rtsafe (target);
    }

  if (fire_events)
    {
      /* FIXME use another flag to decide when to do this */
      last_manual_playhead_change_ =
        Zrythm::getInstance ()->get_monotonic_time_usecs ();
    }
}

void
Transport::set_metronome_enabled (bool enabled)
{
  metronome_enabled_ = enabled;
  gui::SettingsManager::get_instance ()->set_metronomeEnabled (enabled);
}

double
Transport::get_ppqn () const
{
  int    beat_unit = P_TEMPO_TRACK->get_beat_unit ();
  double res = ticks_per_beat_ * ((double) beat_unit / 4.0);
  return res;
}

void
Transport::update_positions (bool update_from_ticks)
{
  assert (update_from_ticks); // only works this way for now

  auto * audio_engine_ = project_->audio_engine_.get ();

  playhead_pos_->update_from_ticks_rtsafe (audio_engine_->frames_per_tick_);
  cue_pos_->update_from_ticks_rtsafe (audio_engine_->frames_per_tick_);
  loop_start_pos_->update_from_ticks_rtsafe (audio_engine_->frames_per_tick_);
  loop_end_pos_->update_from_ticks_rtsafe (audio_engine_->frames_per_tick_);
  punch_in_pos_->update_from_ticks_rtsafe (audio_engine_->frames_per_tick_);
  punch_out_pos_->update_from_ticks_rtsafe (audio_engine_->frames_per_tick_);
}

#if 0
void
Transport::foreach_arranger_handle_playhead_auto_scroll (
  ArrangerWidget * arranger)
{
  arranger_widget_handle_playhead_auto_scroll (arranger, true);
}
#endif

void
Transport::move_to_marker_or_pos_and_fire_events (
  const Marker *   marker,
  const Position * pos)
{
  move_playhead (marker ? marker->get_position () : *pos, true, true, true);

  if (ZRYTHM_HAVE_UI)
    {
      // arranger_widget_foreach (foreach_arranger_handle_playhead_auto_scroll);
    }
}

/**
 * Moves the playhead to the start Marker.
 */
void
Transport::goto_start_marker ()
{
  auto start_marker = P_MARKER_TRACK->get_start_marker ();
  z_return_if_fail (start_marker);
  move_to_marker_or_pos_and_fire_events (start_marker, nullptr);
}

/**
 * Moves the playhead to the end Marker.
 */
void
Transport::goto_end_marker ()
{
  auto end_marker = P_MARKER_TRACK->get_end_marker ();
  z_return_if_fail (end_marker);
  move_to_marker_or_pos_and_fire_events (end_marker, nullptr);
}

/**
 * Moves the playhead to the prev Marker.
 */
void
Transport::goto_prev_marker ()
{
  /* gather all markers */
  std::vector<Position> markers;
  for (auto * marker : P_MARKER_TRACK->get_children_view ())
    {
      markers.push_back (marker->get_position ());
    }
  markers.emplace_back (cue_pos_->get_position ());
  markers.emplace_back (loop_start_pos_->get_position ());
  markers.emplace_back (loop_end_pos_->get_position ());
  markers.emplace_back ();
  std::ranges::sort (markers);

  for (int i = (int) markers.size () - 1; i >= 0; i--)
    {
      if (markers[i] >= *playhead_pos_)
        continue;

      if (
        isRolling () && i > 0
        && (playhead_pos_->to_ms (AUDIO_ENGINE->sample_rate_)
            - markers[i].to_ms (AUDIO_ENGINE->sample_rate_))
             < REPEATED_BACKWARD_MS)
        {
          continue;
        }

      move_to_marker_or_pos_and_fire_events (nullptr, &markers[i]);
      break;
    }
}

/**
 * Moves the playhead to the next Marker.
 */
void
Transport::goto_next_marker ()
{
  /* gather all markers */
  std::vector<Position> markers;
  for (auto * marker : P_MARKER_TRACK->get_children_view ())
    {
      markers.push_back (marker->get_position ());
    }
  markers.push_back (cue_pos_->get_position ());
  markers.push_back (loop_start_pos_->get_position ());
  markers.push_back (loop_end_pos_->get_position ());
  markers.emplace_back ();
  std::ranges::sort (markers);

  for (auto &marker : markers)
    {
      if (marker > *playhead_pos_)
        {
          move_to_marker_or_pos_and_fire_events (nullptr, &marker);
          break;
        }
    }
}

/**
 * Enables or disables loop.
 */
void
Transport::set_loop (bool enabled, bool with_wait)
{
  auto * audio_engine_ = project_->audio_engine_.get ();

  /* can only be called from the gtk thread */
  z_return_if_fail (!audio_engine_->run_.load () || ZRYTHM_IS_QT_THREAD);

  std::optional<SemaphoreRAII<decltype (audio_engine_->port_operation_lock_)>>
    sem;
  if (with_wait)
    {
      sem.emplace (audio_engine_->port_operation_lock_);
    }

  loop_ = enabled;

  if (gZrythm && !ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      gui::SettingsManager::get_instance ()->set_transportLoop (enabled);
    }

  Q_EMIT (loopEnabledChanged (loop_));
}

void
Transport::position_add_frames (Position &pos, const signed_frame_t frames) const
{
  Position pos_before_adding = pos;
  pos.add_frames (frames, AUDIO_ENGINE->ticks_per_frame_);

  /* if start frames were before the loop-end point and the new frames are after
   * (loop crossed) */
  if (
    loop_ && pos_before_adding.frames_ < loop_end_pos_->frames_
    && pos.frames_ >= loop_end_pos_->frames_)
    {
      /* adjust the new frames */
      pos.add_ticks (
        loop_start_pos_->ticks_ - loop_end_pos_->ticks_,
        AUDIO_ENGINE->frames_per_tick_);

      z_warn_if_fail (pos.frames_ < loop_end_pos_->frames_);
    }
}

void
Transport::set_has_range (bool has_range)
{
  has_range_ = has_range;

  // EVENTS_PUSH (EventType::ET_RANGE_SELECTION_CHANGED, nullptr);
}

std::pair<dsp::Position, dsp::Position>
Transport::get_range_positions () const
{
  return range_1_ <= range_2_
           ? std::make_pair (range_1_, range_2_)
           : std::make_pair (range_2_, range_1_);
}

std::pair<dsp::Position, dsp::Position>
Transport::get_loop_range_positions () const
{
  return std::make_pair (
    loop_start_pos_->get_position (), loop_end_pos_->get_position ());
}

void
Transport::set_range (
  bool             range1,
  const Position * start_pos,
  const Position * pos,
  bool             snap)
{
  Position * pos_to_set = range1 ? &range_1_ : &range_2_;

  Position init_pos;
  if (*pos < init_pos)
    {
      *pos_to_set = init_pos;
    }
  else
    {
      *pos_to_set = *pos;
    }

  if (snap)
    {
      *pos_to_set =
        SNAP_GRID_TIMELINE->snap (*pos_to_set, start_pos, nullptr, nullptr);
    }
}

void
Transport::set_loop_range (
  bool             range1,
  const Position * start_pos,
  const Position * pos,
  bool             snap)
{
  auto * pos_to_set = range1 ? loop_start_pos_ : loop_end_pos_;

  Position init_pos;
  if (*pos < init_pos)
    {
      pos_to_set->set_position_rtsafe (init_pos);
    }
  else
    {
      pos_to_set->set_position_rtsafe (*pos);
    }

  if (snap)
    {
      *static_cast<Position *> (pos_to_set) =
        SNAP_GRID_TIMELINE->snap (*pos_to_set, start_pos, nullptr, nullptr);
    }
}

bool
Transport::position_is_inside_punch_range (const Position pos)
{
  return pos.frames_ >= punch_in_pos_->frames_
         && pos.frames_ < punch_out_pos_->frames_;
}

void
Transport::recalculate_total_bars (std::optional<ArrangerObjectSpan> sel_var)
{
  if (!ZRYTHM_HAVE_UI)
    return;

  int total_bars = total_bars_;
  if (sel_var)
    {
      const auto &sel = *sel_var;
      for (const auto &obj_var : sel)
        {
          std::visit (
            [&] (auto &&obj) {
              using ObjT = base_type<decltype (obj)>;
              Position pos;
              if constexpr (std::derived_from<ObjT, BoundedObject>)
                {
                  pos = obj->get_end_position ();
                }
              else
                {
                  pos = obj->get_position ();
                }
              int pos_bars = pos.get_total_bars (
                true, ticks_per_bar_, project_->audio_engine_->frames_per_tick_);
              if (pos_bars > total_bars - 3)
                {
                  total_bars = pos_bars + BARS_END_BUFFER;
                }
            },
            obj_var);
        }
    }
  /* else no selections, calculate total bars for
   * every object */
  else
    {
      total_bars = TRACKLIST->get_track_span ().get_total_bars (
        *this, TRANSPORT_DEFAULT_TOTAL_BARS);

      total_bars += BARS_END_BUFFER;
    }

  update_total_bars (total_bars, true);
}

bool
Transport::is_in_active_project () const
{
  return project_ == Project::get_active_instance ();
}

void
Transport::set_port_metadata_from_owner (
  dsp::PortIdentifier &id,
  PortRange           &range) const
{
  id.owner_type_ = PortIdentifier::OwnerType::Transport;
}

utils::Utf8String
Transport::get_full_designation_for_port (const dsp::PortIdentifier &id) const
{
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("Transport/{}", id.label_));
}

void
Transport::update_total_bars (int total_bars, bool fire_events)
{
  z_return_if_fail (total_bars >= TRANSPORT_DEFAULT_TOTAL_BARS);

  if (total_bars_ == total_bars)
    return;

  total_bars_ = total_bars;

  if (fire_events)
    {
      // EVENTS_PUSH (EventType::ET_TRANSPORT_TOTAL_BARS_CHANGED, nullptr);
    }
}

void
Transport::move_backward (bool with_wait)
{
  auto * audio_engine_ = project_->audio_engine_.get ();

  /* can only be called from the gtk thread */
  z_return_if_fail (!AUDIO_ENGINE->run_.load () || ZRYTHM_IS_QT_THREAD);

  std::optional<SemaphoreRAII<decltype (audio_engine_->port_operation_lock_)>>
    sem;
  if (with_wait)
    {
      sem.emplace (audio_engine_->port_operation_lock_);
    }

  Position pos;
  bool     ret =
    SNAP_GRID_TIMELINE->get_nearby_snap_point (pos, *playhead_pos_, true);
  z_return_if_fail (ret);
  /* if prev snap point is exactly at the playhead or very close it, go back
   * more */
  if (
    pos.frames_ > 0
    && (pos == *playhead_pos_ || (isRolling () && (playhead_pos_->to_ms (audio_engine_->sample_rate_) - pos.to_ms (audio_engine_->sample_rate_)) < REPEATED_BACKWARD_MS)))
    {
      Position tmp = pos;
      tmp.add_ticks (-1, audio_engine_->frames_per_tick_);
      ret = SNAP_GRID_TIMELINE->get_nearby_snap_point (pos, tmp, true);
      z_return_if_fail (ret);
    }
  move_playhead (pos, true, true, true);
}

void
Transport::move_forward (bool with_wait)
{
  auto * audio_engine_ = project_->audio_engine_.get ();

  /* can only be called from the gtk thread */
  z_return_if_fail (!audio_engine_->run_.load () || ZRYTHM_IS_QT_THREAD);

  std::optional<SemaphoreRAII<decltype (audio_engine_->port_operation_lock_)>>
    sem;
  if (with_wait)
    {
      sem.emplace (audio_engine_->port_operation_lock_);
    }

  Position pos;
  bool     ret =
    SNAP_GRID_TIMELINE->get_nearby_snap_point (pos, *playhead_pos_, false);
  z_return_if_fail (ret);
  move_playhead (pos, true, true, true);
}

/**
 * Sets recording on/off.
 */
void
Transport::set_recording (bool record, bool with_wait)
{
  auto * audio_engine_ = project_->audio_engine_.get ();

  /* can only be called from the gtk thread */
  z_return_if_fail (!audio_engine_->run_.load () || ZRYTHM_IS_QT_THREAD);

  std::optional<SemaphoreRAII<decltype (audio_engine_->port_operation_lock_)>>
    sem;
  if (with_wait)
    {
      sem.emplace (audio_engine_->port_operation_lock_);
    }

  recording_ = record;

  Q_EMIT recordEnabledChanged (recording_);
}

QString
Transport::getPlayheadPositionString (const TempoTrack * tempo_track) const
{
  return playhead_pos_->getStringDisplay (this, tempo_track);
}
