// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <algorithm>

#include "engine/device_io/engine.h"
#include "engine/session/transport.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/marker.h"
#include "structure/tracks/audio_track.h"
#include "structure/tracks/laned_track.h"
#include "structure/tracks/marker_track.h"
#include "structure/tracks/tracklist.h"
#include "utils/debug.h"
#include "utils/gtest_wrapper.h"
#include "utils/rt_thread_id.h"

namespace zrythm::engine::session
{
/**
 * A buffer of n bars after the end of the last object.
 */
// constexpr int BARS_END_BUFFER = 4;

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
Transport::init_loaded (Project * project)
{
  project_ = project;

  z_return_if_fail_cmp (total_bars_, >, 0);

  init_common ();

  int beats_per_bar =
    project->get_tempo_map ().time_signature_at_tick (0).numerator;
  int beat_unit =
    project->get_tempo_map ().time_signature_at_tick (0).denominator;
  update_caches (beats_per_bar, beat_unit);
}

Transport::Transport (Project * parent)
    : QObject (parent), playhead_ (parent->get_tempo_map ()),
      playhead_adapter_ (new dsp::PlayheadQmlWrapper (playhead_, this)),
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

  z_return_if_fail (project_->audio_engine_->get_sample_rate () > 0);

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
  roll_ = std::make_unique<dsp::MidiPort> (u8"Roll", PortFlow::Input);
  roll_->set_symbol (u8"roll");
  roll_->set_full_designation_provider (this);
  // roll_->id_->flags_ |= PortIdentifier::Flags::Trigger;

  stop_ = std::make_unique<dsp::MidiPort> (u8"Stop", PortFlow::Input);
  stop_->set_symbol (u8"stop");
  stop_->set_full_designation_provider (this);
  // stop_->id_->flags_ |= PortIdentifier::Flags::Trigger;

  backward_ = std::make_unique<dsp::MidiPort> (u8"Backward", PortFlow::Input);
  backward_->set_symbol (u8"backward");
  backward_->set_full_designation_provider (this);
  // backward_->id_->flags_ |= PortIdentifier::Flags::Trigger;

  forward_ = std::make_unique<dsp::MidiPort> (u8"Forward", PortFlow::Input);
  forward_->set_symbol (u8"forward");
  forward_->set_full_designation_provider (this);
  // forward_->id_->flags_ |= PortIdentifier::Flags::Trigger;

  loop_toggle_ =
    std::make_unique<dsp::MidiPort> (u8"Loop toggle", PortFlow::Input);
  loop_toggle_->set_symbol (u8"loop_toggle");
  loop_toggle_->set_full_designation_provider (this);
  // loop_toggle_->id_->flags_ |= PortIdentifier::Flags::Toggle;

  rec_toggle_ =
    std::make_unique<dsp::MidiPort> (u8"Rec toggle", PortFlow::Input);
  rec_toggle_->set_symbol (u8"rec_toggle");
  rec_toggle_->set_full_designation_provider (this);
  // rec_toggle_->id_->flags_ |= PortIdentifier::Flags::Toggle;

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

dsp::PlayheadQmlWrapper *
Transport::getPlayhead () const
{
  return playhead_adapter_.get ();
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
init_from (
  Transport             &obj,
  const Transport       &other,
  utils::ObjectCloneType clone_type)
{
  obj.total_bars_ = other.total_bars_;
  obj.has_range_ = other.has_range_;
  obj.position_ = other.position_;

  obj.loop_start_pos_->set_position_rtsafe (*other.loop_start_pos_);
  obj.playhead_.set_position_ticks (other.playhead_.position_ticks ());
  obj.loop_end_pos_->set_position_rtsafe (*other.loop_end_pos_);
  obj.cue_pos_->set_position_rtsafe (*other.cue_pos_);
  obj.punch_in_pos_->set_position_rtsafe (*other.punch_in_pos_);
  obj.punch_out_pos_->set_position_rtsafe (*other.punch_out_pos_);
  obj.range_1_ = other.range_1_;
  obj.range_2_ = other.range_2_;

  // TODO
#if 0
  // Clone ports if they exit using a lambda
  auto clone_port = [] (const auto &port) {
    return port ? utils::clone_unique (*port) : nullptr;
  };

  obj.roll_ = clone_port (other.roll_);
  obj.stop_ = clone_port (other.stop_);
  obj.backward_ = clone_port (other.backward_);
  obj.forward_ = clone_port (other.forward_);
  obj.loop_toggle_ = clone_port (other.loop_toggle_);
  obj.rec_toggle_ = clone_port (other.rec_toggle_);
#endif
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
  std::optional<structure::arrangement::ArrangerObjectSpan> sel_var)
{
// TODO
#if 0
  if (sel_var)
    {
      for (
        auto * obj :
        sel_var->template get_elements_by_type<
          structure::arrangement::AudioRegion> ())
        {
          obj->before_length_ = obj->get_length_in_ticks ();
        }
    }
  else
    {
      for (
        auto * track :
        TRACKLIST->get_track_span ()
          .get_elements_by_type<structure::tracks::AudioTrack> ())
        {
          for (auto &lane_var : track->lanes_)
            {
              auto * lane = std::get<structure::tracks::AudioLane *> (lane_var);
              for (auto * region : lane->get_children_view ())
                {
                  region->before_length_ = region->get_length_in_ticks ();
                }
            }
        }
    }
#endif
}

void
Transport::stretch_regions (
  std::optional<structure::arrangement::ArrangerObjectSpan> sel_var,
  bool                                                      with_fixed_ratio,
  double                                                    time_ratio,
  bool                                                      force)
{
// TODO
#if 0
  if (sel_var)
    {
      const auto &sel = *sel_var;
      for (
        auto * region :
        sel.template get_elements_derived_from<structure::arrangement::Region> ())
        {
          auto r_variant = convert_to_variant<
            structure::arrangement::RegionPtrVariant> (region);
          std::visit (
            [&] (auto &&r) {
              if constexpr (
                std::is_same_v<
                  base_type<decltype (r)>, structure::arrangement::AudioRegion>)
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
        TRACKLIST->get_track_span ()
          .get_elements_by_type<structure::tracks::AudioTrack> ())
        {
          for (auto &lane_var : track->lanes_)
            {
              auto * lane = std::get<structure::tracks::AudioLane *> (lane_var);
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
#endif
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

  playhead_before_pause_ = playhead_.position_ticks ();
  if (
    !ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING
    && gui::SettingsManager::transportReturnToCue ())
    {
      auto tmp = cue_pos_->get_position ();
      move_playhead (tmp.ticks_, false);
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
          auto       pos_tick = playhead_.position_ticks ();
          const auto pos_musical =
            playhead_.get_tempo_map ().tick_to_musical_position (
              static_cast<int64_t> (pos_tick));
          auto new_pos_musical = pos_musical;
          new_pos_musical.bar = std::max (new_pos_musical.bar - num_bars, 1);
          pos_tick = static_cast<double> (
            playhead_.get_tempo_map ().musical_position_to_tick (
              new_pos_musical));
          auto pos_frame = playhead_.get_tempo_map ().tick_to_samples (pos_tick);
          preroll_frames_remaining_ = static_cast<signed_frame_t> (std::round (
            playhead_.get_tempo_map ().tick_to_samples (
              playhead_.position_ticks ())
            - pos_frame));
          playhead_adapter_->setTicks (pos_tick);
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
Transport::add_to_playhead_in_audio_thread (const signed_frame_t nframes)
{
  const auto cur_pos = get_playhead_position_in_audio_thread ();
  auto       new_pos =
    get_playhead_position_after_adding_frames_in_audio_thread (nframes);
  auto diff = new_pos - cur_pos;
  playhead_.advance_processing (diff);
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

dsp::Position
Transport::get_playhead_position_in_gui_thread () const
{
  return { playhead_.position_ticks (), AUDIO_ENGINE->frames_per_tick_ };
}

void
Transport::move_playhead (double target_ticks, bool set_cue_point)
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
    TRACKLIST->get_track_span ()
      .get_elements_derived_from<structure::tracks::LanedTrack> ())
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
                  const auto playhead_pos =
                    get_playhead_position_in_gui_thread ();
                  if (!region->regionMixin ()->bounds ()->is_hit (
                        playhead_pos.frames_, true))
                    continue;

                  if constexpr (
                    std::is_same_v<
                      typename TrackLaneT::RegionT,
                      structure::arrangement::MidiRegion>)
                    {
                      for (auto * midi_note : region->get_children_view ())
                        {
                          if (
                            midi_note->bounds ()->is_hit (playhead_pos.frames_))
                            {
                              t->processor_->get_piano_roll_port ()
                                .midi_events_.queued_events_
                                .add_note_off (1, midi_note->pitch (), 0);
                            }
                        }
                    }
                }
            }
        },
        convert_to_variant<structure::tracks::LanedTrackPtrVariant> (track));
    }

  /* move to new pos */
  playhead_adapter_->setTicks (target_ticks);

  if (set_cue_point)
    {
      /* move cue point */
      Position target{ target_ticks, AUDIO_ENGINE->frames_per_tick_ };
      cue_pos_->set_position_rtsafe (target);
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
  return dsp::TempoMap::get_ppq ();
}

void
Transport::update_positions (bool update_from_ticks)
{
  assert (update_from_ticks); // only works this way for now

  auto * audio_engine_ = project_->audio_engine_.get ();

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
  const structure::arrangement::Marker * marker,
  std::optional<double>                  pos_ticks)
{
  move_playhead (
    (marker != nullptr) ? marker->position ()->ticks () : *pos_ticks, true);

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
  move_to_marker_or_pos_and_fire_events (start_marker, std::nullopt);
}

/**
 * Moves the playhead to the end Marker.
 */
void
Transport::goto_end_marker ()
{
  auto end_marker = P_MARKER_TRACK->get_end_marker ();
  z_return_if_fail (end_marker);
  move_to_marker_or_pos_and_fire_events (end_marker, std::nullopt);
}

/**
 * Moves the playhead to the prev Marker.
 */
void
Transport::goto_prev_marker ()
{
  /* gather all markers */
  std::vector<double> marker_ticks;
  for (auto * marker : P_MARKER_TRACK->get_children_view ())
    {
      marker_ticks.push_back (marker->position ()->ticks ());
    }
  marker_ticks.emplace_back (cue_pos_->get_position ().ticks_);
  marker_ticks.emplace_back (loop_start_pos_->get_position ().ticks_);
  marker_ticks.emplace_back (loop_end_pos_->get_position ().ticks_);
  marker_ticks.emplace_back ();
  std::ranges::sort (marker_ticks);

  for (int i = (int) marker_ticks.size () - 1; i >= 0; i--)
    {
      if (marker_ticks[i] >= playhead_.position_ticks ())
        continue;

      if (
        isRolling () && i > 0
        && (playhead_.get_tempo_map ().tick_to_seconds (
              playhead_.position_ticks ())
              / 1000.0
            - PROJECT->get_tempo_map ().tick_to_seconds (marker_ticks[i]) / 1000.0)
             < static_cast<double> (REPEATED_BACKWARD_MS))
        {
          continue;
        }

      move_to_marker_or_pos_and_fire_events (nullptr, marker_ticks[i]);
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
  std::vector<double> marker_ticks;
  for (auto * marker : P_MARKER_TRACK->get_children_view ())
    {
      marker_ticks.push_back (marker->position ()->ticks ());
    }
  marker_ticks.push_back (cue_pos_->get_position ().ticks_);
  marker_ticks.push_back (loop_start_pos_->get_position ().ticks_);
  marker_ticks.push_back (loop_end_pos_->get_position ().ticks_);
  marker_ticks.emplace_back ();
  std::ranges::sort (marker_ticks);

  for (auto &marker : marker_ticks)
    {
      if (marker > playhead_.position_ticks ())
        {
          move_to_marker_or_pos_and_fire_events (nullptr, marker);
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

signed_frame_t
Transport::get_playhead_position_after_adding_frames_in_audio_thread (
  const signed_frame_t frames) const
{
  const auto pos_before_adding =
    static_cast<int64_t> (playhead_.position_during_processing_rounded ());
  auto new_pos = pos_before_adding + frames;

  /* if start frames were before the loop-end point and the new frames are after
   * (loop crossed) */
  if (
    loop_ && pos_before_adding < loop_end_pos_->frames_
    && new_pos >= loop_end_pos_->frames_)
    {
      /* adjust the new frames */
      new_pos += loop_start_pos_->frames_ - loop_end_pos_->frames_;
      assert (new_pos < loop_end_pos_->frames_);
    }

  return new_pos;
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
      *pos_to_set = SNAP_GRID_TIMELINE->snap (
        *pos_to_set, start_pos, nullptr, std::nullopt);
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
      *static_cast<Position *> (pos_to_set) = SNAP_GRID_TIMELINE->snap (
        *pos_to_set, start_pos, nullptr, std::nullopt);
    }
}

bool
Transport::position_is_inside_punch_range (const Position pos)
{
  return pos.frames_ >= punch_in_pos_->frames_
         && pos.frames_ < punch_out_pos_->frames_;
}

void
Transport::recalculate_total_bars (
  std::optional<structure::arrangement::ArrangerObjectSpan> sel_var)
{
// TODO
#if 0
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
              if constexpr (structure::arrangement::BoundedObject<ObjT>)
                {
                  pos =
                    structure::arrangement::ArrangerObjectSpan::bounds_projection (
                      obj)
                      ->get_end_position ();
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
#endif
}

utils::Utf8String
Transport::get_full_designation_for_port (const dsp::Port &port) const
{
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("Transport/{}", port.get_label ()));
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
  bool     ret = SNAP_GRID_TIMELINE->get_nearby_snap_point (
    pos, playhead_.position_ticks (), true);
  assert (ret);
  /* if prev snap point is exactly at the playhead or very close it, go back
   * more */
  Position playhead_pos{
    playhead_.position_ticks (), audio_engine_->frames_per_tick_
  };
  if (
    pos.frames_ > 0
    && (pos.frames_ == playhead_pos.frames_ || (isRolling () && (playhead_pos.to_ms (audio_engine_->get_sample_rate()) - pos.to_ms (audio_engine_->get_sample_rate())) < REPEATED_BACKWARD_MS)))
    {
      Position tmp = pos;
      tmp.add_ticks (-1, audio_engine_->frames_per_tick_);
      ret = SNAP_GRID_TIMELINE->get_nearby_snap_point (pos, tmp.ticks_, true);
      z_return_if_fail (ret);
    }
  move_playhead (pos.ticks_, true);
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

  Position                    pos;
  [[maybe_unused]] const bool ret = SNAP_GRID_TIMELINE->get_nearby_snap_point (
    pos, playhead_.position_ticks (), false);
  assert (ret);
  move_playhead (pos.ticks_, true);
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
}
