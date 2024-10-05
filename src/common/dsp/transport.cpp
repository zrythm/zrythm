// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/settings/g_settings_manager.h"
#include "gui/cpp/gtk_widgets/arranger.h"
#include "gui/cpp/gtk_widgets/digital_meter.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/main_window.h"
#include "gui/cpp/gtk_widgets/midi_modifier_arranger.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include "common/dsp/audio_region.h"
#include "common/dsp/audio_track.h"
#include "common/dsp/engine.h"
#include "common/dsp/laned_track.h"
#include "common/dsp/marker.h"
#include "common/dsp/marker_track.h"
#include "common/dsp/tempo_track.h"
#include "common/dsp/tracklist.h"
#include "common/dsp/transport.h"
#include "common/utils/debug.h"
#include "common/utils/flags.h"
#include "common/utils/gtest_wrapper.h"
#include "common/utils/rt_thread_id.h"

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
    testing_or_benchmarking ? true : g_settings_get_boolean (S_TRANSPORT, "loop");
  metronome_enabled_ =
    testing_or_benchmarking
      ? true
      : g_settings_get_boolean (S_TRANSPORT, "metronome-enabled");
  punch_mode_ =
    testing_or_benchmarking
      ? true
      : g_settings_get_boolean (S_TRANSPORT, "punch-mode");
  start_playback_on_midi_input_ =
    testing_or_benchmarking
      ? false
      : g_settings_get_boolean (S_TRANSPORT, "start-on-midi-input");
  recording_mode_ =
    testing_or_benchmarking
      ? RecordingMode::Takes
      : (RecordingMode) g_settings_get_enum (S_TRANSPORT, "recording-mode");
}

void
Transport::init_loaded (AudioEngine * engine, const TempoTrack * tempo_track)
{
  audio_engine_ = engine;

  z_return_if_fail_cmp (total_bars_, >, 0);

  init_common ();

  if (tempo_track)
    {
      int beats_per_bar = tempo_track->get_beats_per_bar ();
      int beat_unit = tempo_track->get_beat_unit ();
      update_caches (beats_per_bar, beat_unit);
    }

#define INIT_LOADED_PORT(x) x->init_loaded (this)

  INIT_LOADED_PORT (roll_);
  INIT_LOADED_PORT (stop_);
  INIT_LOADED_PORT (backward_);
  INIT_LOADED_PORT (forward_);
  INIT_LOADED_PORT (loop_toggle_);
  INIT_LOADED_PORT (rec_toggle_);

#undef INIT_LOADED_PORT
}

Transport::Transport (AudioEngine * engine) : audio_engine_ (engine)
{
  z_debug ("Creating transport...");

  /* set initial total number of beats this is applied to the ruler */
  total_bars_ = TRANSPORT_DEFAULT_TOTAL_BARS;

  z_return_if_fail (engine->sample_rate_ > 0);

  double ticks_per_bar = TICKS_PER_QUARTER_NOTE * 4.0;
  loop_end_pos_.ticks_ = 4 * ticks_per_bar;
  punch_in_pos_.ticks_ = 2 * ticks_per_bar;
  punch_out_pos_.ticks_ = 4 * ticks_per_bar;

  range_1_.ticks_ = 1 * ticks_per_bar;
  range_2_.ticks_ = 1 * ticks_per_bar;

  /*if (math_doubles_equal (*/
  /*frames_per_tick_before, 0))*/
  /*{*/
  /*AUDIO_ENGINE->frames_per_tick = 0;*/
  /*}*/

  /* create ports */
  roll_ = std::make_unique<MidiPort> ("Roll", PortFlow::Input);
  roll_->id_.sym_ = ("roll");
  roll_->set_owner<Transport> (this);
  roll_->id_.flags_ |= PortIdentifier::Flags::Toggle;
  roll_->id_.flags2_ |= PortIdentifier::Flags2::TransportRoll;

  stop_ = std::make_unique<MidiPort> ("Stop", PortFlow::Input);
  stop_->id_.sym_ = ("stop");
  stop_->set_owner<Transport> (this);
  stop_->id_.flags_ |= PortIdentifier::Flags::Toggle;
  stop_->id_.flags2_ |= PortIdentifier::Flags2::TransportStop;

  backward_ = std::make_unique<MidiPort> ("Backward", PortFlow::Input);
  backward_->id_.sym_ = ("backward");
  backward_->set_owner<Transport> (this);
  backward_->id_.flags_ |= PortIdentifier::Flags::Toggle;
  backward_->id_.flags2_ |= PortIdentifier::Flags2::TransportBackward;

  forward_ = std::make_unique<MidiPort> ("Forward", PortFlow::Input);
  forward_->id_.sym_ = ("forward");
  forward_->set_owner<Transport> (this);
  forward_->id_.flags_ |= PortIdentifier::Flags::Toggle;
  forward_->id_.flags2_ |= PortIdentifier::Flags2::TransportForward;

  loop_toggle_ = std::make_unique<MidiPort> ("Loop toggle", PortFlow::Input);
  loop_toggle_->id_.sym_ = ("loop_toggle");
  loop_toggle_->set_owner<Transport> (this);
  loop_toggle_->id_.flags_ |= PortIdentifier::Flags::Toggle;
  loop_toggle_->id_.flags2_ |= PortIdentifier::Flags2::TransportLoopToggle;

  rec_toggle_ = std::make_unique<MidiPort> ("Rec toggle", PortFlow::Input);
  rec_toggle_->id_.sym_ = ("rec_toggle");
  rec_toggle_->set_owner<Transport> (this);
  rec_toggle_->id_.flags_ |= PortIdentifier::Flags::Toggle;
  rec_toggle_->id_.flags2_ |= PortIdentifier::Flags2::TransportRecToggle;

  init_common ();
}

void
Transport::init_after_cloning (const Transport &other)
{
  total_bars_ = other.total_bars_;
  has_range_ = other.has_range_;
  position_ = other.position_;

  loop_start_pos_ = other.loop_start_pos_;
  playhead_pos_ = other.playhead_pos_;
  loop_end_pos_ = other.loop_end_pos_;
  cue_pos_ = other.cue_pos_;
  punch_in_pos_ = other.punch_in_pos_;
  punch_out_pos_ = other.punch_out_pos_;
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
Transport::prepare_audio_regions_for_stretch (TimelineSelections * sel)
{
  if (sel)
    {
      for (auto obj : sel->objects_ | type_is<AudioRegion> ())
        {
          obj->before_length_ = obj->get_length_in_ticks ();
        }
    }
  else
    {
      for (auto track : TRACKLIST->tracks_ | type_is<AudioTrack> ())
        {
          for (auto &lane : track->lanes_)
            {
              for (auto &region : lane->regions_)
                {
                  region->before_length_ = region->get_length_in_ticks ();
                }
            }
        }
    }
}

void
Transport::stretch_regions (
  TimelineSelections * sel,
  bool                 with_fixed_ratio,
  double               time_ratio,
  bool                 force)
{
  if (sel)
    {
      for (auto region : sel->objects_ | type_is<Region> ())
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
      for (auto track : TRACKLIST->tracks_ | type_is<AudioTrack> ())
        {
          for (auto &lane : track->lanes_)
            {
              for (auto &region : lane->regions_)
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
      g_settings_set_boolean (S_TRANSPORT, "punch-mode", enabled);
    }
}

void
Transport::set_start_playback_on_midi_input (bool enabled)
{
  start_playback_on_midi_input_ = enabled;
  g_settings_set_boolean (S_TRANSPORT, "start-on-midi-input", enabled);
}

void
Transport::set_recording_mode (RecordingMode mode)
{
  recording_mode_ = mode;

  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      g_settings_set_enum (
        S_TRANSPORT, "recording-mode", ENUM_VALUE_TO_INT (mode));
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
Transport::request_pause (bool with_wait)
{
  /* can only be called from the gtk thread or when preparing to export */
  z_return_if_fail (
    !audio_engine_->run_.load () || ZRYTHM_APP_IS_GTK_THREAD
    || audio_engine_->preparing_to_export_);

  if (with_wait)
    {
      audio_engine_->port_operation_lock_.acquire ();
    }

  play_state_ = PlayState::PauseRequested;

  playhead_before_pause_ = playhead_pos_;
  if (
    !ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING
    && g_settings_get_boolean (S_TRANSPORT, "return-to-cue"))
    {
      move_playhead (&cue_pos_, F_PANIC, F_NO_SET_CUE_POINT, true);
    }

  if (with_wait)
    {
      audio_engine_->port_operation_lock_.release ();
    }
}

void
Transport::request_roll (bool with_wait)
{
  /* can only be called from the gtk thread */
  z_return_if_fail (!audio_engine_->run_.load () || ZRYTHM_APP_IS_GTK_THREAD);

  std::optional<SemaphoreRAII<std::counting_semaphore<>>> wait_sem;
  if (with_wait)
    {
      wait_sem.emplace (audio_engine_->port_operation_lock_);
    }

  if (gZrythm && !ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      /* handle countin */
      PrerollCountBars bars = ENUM_INT_TO_VALUE (
        PrerollCountBars,
        g_settings_get_enum (S_TRANSPORT, "metronome-countin"));
      int    num_bars = preroll_count_bars_enum_to_int (bars);
      double frames_per_bar =
        audio_engine_->frames_per_tick_ * (double) ticks_per_bar_;
      countin_frames_remaining_ = (long) ((double) num_bars * frames_per_bar);
      if (metronome_enabled_)
        {
          SAMPLE_PROCESSOR->queue_metronome_countin ();
        }

      if (recording_)
        {
          /* handle preroll */
          bars = ENUM_INT_TO_VALUE (
            PrerollCountBars,
            g_settings_get_enum (S_TRANSPORT, "recording-preroll"));
          num_bars = preroll_count_bars_enum_to_int (bars);
          Position pos = playhead_pos_;
          pos.add_bars (-num_bars);
          pos.print ();
          if (!pos.is_positive ())
            pos = Position ();
          preroll_frames_remaining_ = playhead_pos_.frames_ - pos.frames_;
          set_playhead_pos (pos);
#if 0
          z_debug (
            "preroll %ld frames",
            preroll_frames_remaining_);
#endif
        }
    }

  play_state_ = PlayState::RollRequested;
}

void
Transport::add_to_playhead (const signed_frame_t nframes)
{
  playhead_pos_.add_frames (nframes);
  EVENTS_PUSH (EventType::ET_PLAYHEAD_POS_CHANGED, nullptr);
}

void
Transport::set_playhead_pos (const Position pos)
{
  playhead_pos_ = pos;
  EVENTS_PUSH (EventType::ET_PLAYHEAD_POS_CHANGED_MANUALLY, nullptr);
}

void
Transport::set_playhead_to_bar (int bar)
{
  Position pos;
  pos.set_to_bar (bar);
  set_playhead_pos (pos);
}

/**
 * Getter for playhead Position.
 */
void
Transport::get_playhead_pos (Position * pos)
{
  z_return_if_fail (pos);

  *pos = playhead_pos_;
}
bool
Transport::can_user_move_playhead () const
{
  if (
    recording_ && play_state_ == PlayState::Rolling && audio_engine_
    && audio_engine_->run_.load ())
    return false;
  else
    return true;
}

void
Transport::move_playhead (
  const Position * target,
  bool             panic,
  bool             set_cue_point,
  bool             fire_events)
{
  /* if currently recording, do nothing */
  if (!can_user_move_playhead ())
    {
      z_info ("currently recording - refusing to move playhead manually");
      return;
    }

  /* send MIDI note off on currently playing timeline objects */
  for (auto track : TRACKLIST->tracks_ | type_is<LanedTrack> ())
    {
      std::visit (
        [&] (auto &&t) {
          for (auto &lane : t->lanes_)
            {
              for (const auto &region : lane->regions_)
                {
                  if (!region->is_hit (playhead_pos_.frames_, true))
                    continue;

                  if constexpr (
                    std::is_same_v<
                      base_type<decltype (region.get ())>, MidiRegion>)
                    {
                      for (auto &midi_note : region->midi_notes_)
                        {
                          if (midi_note->is_hit (playhead_pos_.frames_))
                            {
                              t->processor_->piano_roll_->midi_events_
                                .queued_events_
                                .add_note_off (1, midi_note->val_, 0);
                            }
                        }
                    }
                }
            }
        },
        convert_to_variant<LanedTrackPtrVariant> (track));
    }

  /* move to new pos */
  playhead_pos_ = *target;

  if (set_cue_point)
    {
      /* move cue point */
      cue_pos_ = *target;
    }

  if (fire_events)
    {
      /* FIXME use another flag to decide when to do this */
      last_manual_playhead_change_ = g_get_monotonic_time ();

      EVENTS_PUSH (EventType::ET_PLAYHEAD_POS_CHANGED_MANUALLY, nullptr);
    }
}

void
Transport::set_metronome_enabled (bool enabled)
{
  metronome_enabled_ = enabled;
  g_settings_set_boolean (S_TRANSPORT, "metronome-enabled", enabled);
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
  playhead_pos_.update (update_from_ticks, 0.0);
  cue_pos_.update (update_from_ticks, 0.0);
  loop_start_pos_.update (update_from_ticks, 0.0);
  loop_end_pos_.update (update_from_ticks, 0.0);
  punch_in_pos_.update (update_from_ticks, 0.0);
  punch_out_pos_.update (update_from_ticks, 0.0);
}

void
Transport::foreach_arranger_handle_playhead_auto_scroll (
  ArrangerWidget * arranger)
{
  arranger_widget_handle_playhead_auto_scroll (arranger, true);
}

void
Transport::move_to_marker_or_pos_and_fire_events (
  const Marker *   marker,
  const Position * pos)
{
  move_playhead (marker ? &marker->pos_ : pos, true, true, true);

  if (ZRYTHM_HAVE_UI)
    {
      arranger_widget_foreach (foreach_arranger_handle_playhead_auto_scroll);
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
  move_to_marker_or_pos_and_fire_events (start_marker.get (), nullptr);
}

/**
 * Moves the playhead to the end Marker.
 */
void
Transport::goto_end_marker ()
{
  auto end_marker = P_MARKER_TRACK->get_end_marker ();
  z_return_if_fail (end_marker);
  move_to_marker_or_pos_and_fire_events (end_marker.get (), nullptr);
}

/**
 * Moves the playhead to the prev Marker.
 */
void
Transport::goto_prev_marker ()
{
  /* gather all markers */
  std::vector<Position> markers;
  for (auto &marker : P_MARKER_TRACK->markers_)
    {
      markers.push_back (marker->pos_);
    }
  markers.push_back (cue_pos_);
  markers.push_back (loop_start_pos_);
  markers.push_back (loop_end_pos_);
  markers.push_back (Position ());
  std::sort (markers.begin (), markers.end ());

  for (int i = markers.size () - 1; i >= 0; i--)
    {
      if (markers[i] >= playhead_pos_)
        continue;

      if (
        is_rolling () && i > 0
        && (playhead_pos_.to_ms () - markers[i].to_ms ()) < REPEATED_BACKWARD_MS)
        {
          continue;
        }
      else
        {
          move_to_marker_or_pos_and_fire_events (nullptr, &markers[i]);
          break;
        }
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
  for (auto &marker : P_MARKER_TRACK->markers_)
    {
      markers.push_back (marker->pos_);
    }
  markers.push_back (cue_pos_);
  markers.push_back (loop_start_pos_);
  markers.push_back (loop_end_pos_);
  markers.push_back (Position ());
  std::sort (markers.begin (), markers.end ());

  for (auto &marker : markers)
    {
      if (marker > playhead_pos_)
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
  /* can only be called from the gtk thread */
  z_return_if_fail (!audio_engine_->run_.load () || ZRYTHM_APP_IS_GTK_THREAD);

  std::optional<SemaphoreRAII<std::counting_semaphore<>>> sem;
  if (with_wait)
    {
      sem.emplace (audio_engine_->port_operation_lock_);
    }

  loop_ = enabled;

  if (gZrythm && !ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      g_settings_set_boolean (S_TRANSPORT, "loop", enabled);
    }

  EVENTS_PUSH (EventType::ET_LOOP_TOGGLED, nullptr);
}

void
Transport::position_add_frames (Position * pos, const signed_frame_t frames)
{
  Position pos_before_adding = *pos;
  pos->add_frames (frames);

  /* if start frames were before the loop-end point and the new frames are after
   * (loop crossed) */
  if (
    loop_ && pos_before_adding.frames_ < loop_end_pos_.frames_
    && pos->frames_ >= loop_end_pos_.frames_)
    {
      /* adjust the new frames */
      pos->add_ticks (loop_start_pos_.ticks_ - loop_end_pos_.ticks_);

      z_warn_if_fail (pos->frames_ < loop_end_pos_.frames_);
    }
}

void
Transport::set_has_range (bool has_range)
{
  has_range_ = has_range;

  EVENTS_PUSH (EventType::ET_RANGE_SELECTION_CHANGED, nullptr);
}

std::pair<Position, Position>
Transport::get_range_positions () const
{
  return range_1_ <= range_2_
           ? std::make_pair (range_1_, range_2_)
           : std::make_pair (range_2_, range_1_);
}

std::pair<Position, Position>
Transport::get_loop_range_positions () const
{
  return std::make_pair (loop_start_pos_, loop_end_pos_);
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
      pos_to_set->snap (start_pos, nullptr, nullptr, *SNAP_GRID_TIMELINE);
    }
}

void
Transport::set_loop_range (
  bool             range1,
  const Position * start_pos,
  const Position * pos,
  bool             snap)
{
  Position * pos_to_set = range1 ? &loop_start_pos_ : &loop_end_pos_;

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
      pos_to_set->snap (start_pos, nullptr, nullptr, *SNAP_GRID_TIMELINE);
    }
}

bool
Transport::position_is_inside_punch_range (const Position pos)
{
  return pos.frames_ >= punch_in_pos_.frames_
         && pos.frames_ < punch_out_pos_.frames_;
}

void
Transport::recalculate_total_bars (ArrangerSelections * sel)
{
  if (!ZRYTHM_HAVE_UI)
    return;

  int total_bars = total_bars_;
  if (sel)
    {
      for (const auto &obj : sel->objects_)
        {
          Position pos;
          if (ArrangerObject::type_has_length (obj->type_))
            {
              obj->get_position_from_type (
                &pos, ArrangerObject::PositionType::End);
            }
          else
            {
              obj->get_pos (&pos);
            }
          int pos_bars = pos.get_total_bars (true);
          if (pos_bars > total_bars - 3)
            {
              total_bars = pos_bars + BARS_END_BUFFER;
            }
        }
    }
  /* else no selections, calculate total bars for
   * every object */
  else
    {
      total_bars = TRACKLIST->get_total_bars (TRANSPORT_DEFAULT_TOTAL_BARS);

      total_bars += BARS_END_BUFFER;
    }

  update_total_bars (total_bars, true);
}

bool
Transport::is_in_active_project () const
{
  return audio_engine_ == AUDIO_ENGINE.get ();
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
      EVENTS_PUSH (EventType::ET_TRANSPORT_TOTAL_BARS_CHANGED, nullptr);
    }
}

void
Transport::move_backward (bool with_wait)
{
  /* can only be called from the gtk thread */
  z_return_if_fail (!AUDIO_ENGINE->run_.load () || ZRYTHM_APP_IS_GTK_THREAD);

  std::optional<SemaphoreRAII<std::counting_semaphore<>>> sem;
  if (with_wait)
    {
      sem.emplace (audio_engine_->port_operation_lock_);
    }

  Position pos;
  bool     ret =
    SNAP_GRID_TIMELINE->get_nearby_snap_point (pos, playhead_pos_, true);
  z_return_if_fail (ret);
  /* if prev snap point is exactly at the playhead or very close it, go back
   * more */
  if (
    pos.frames_ > 0
    && (pos == playhead_pos_ || (is_rolling () && (playhead_pos_.to_ms () - pos.to_ms ()) < REPEATED_BACKWARD_MS)))
    {
      Position tmp = pos;
      tmp.add_ticks (-1);
      ret = SNAP_GRID_TIMELINE->get_nearby_snap_point (pos, tmp, true);
      z_return_if_fail (ret);
    }
  move_playhead (&pos, true, true, true);
}

void
Transport::move_forward (bool with_wait)
{
  /* can only be called from the gtk thread */
  z_return_if_fail (!audio_engine_->run_.load () || ZRYTHM_APP_IS_GTK_THREAD);

  std::optional<SemaphoreRAII<std::counting_semaphore<>>> sem;
  if (with_wait)
    {
      sem.emplace (audio_engine_->port_operation_lock_);
    }

  Position pos;
  bool     ret =
    SNAP_GRID_TIMELINE->get_nearby_snap_point (pos, playhead_pos_, false);
  z_return_if_fail (ret);
  move_playhead (&pos, true, true, true);
}

/**
 * Sets recording on/off.
 */
void
Transport::set_recording (bool record, bool with_wait, bool fire_events)
{
  /* can only be called from the gtk thread */
  z_return_if_fail (!AUDIO_ENGINE->run_.load () || ZRYTHM_APP_IS_GTK_THREAD);

  std::optional<SemaphoreRAII<std::counting_semaphore<>>> sem;
  if (with_wait)
    {
      sem.emplace (audio_engine_->port_operation_lock_);
    }

  recording_ = record;

  if (fire_events)
    {
      EVENTS_PUSH (EventType::ET_TRANSPORT_RECORDING_ON_OFF_CHANGED, nullptr);
    }
}