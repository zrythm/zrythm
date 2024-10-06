// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "common/dsp/midi_event.h"
#include "common/dsp/midi_track.h"
#include "common/utils/midi.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

constexpr auto BUFFER_SIZE = 20;

class TrackFixture : public ZrythmFixture
{
public:
  void SetUp () override
  {
    ZrythmFixture::SetUp ();
    midi_track_ = Track::create_empty_with_action<MidiTrack> ();
  };

  /**
   * Prepares a MIDI region with a note starting at the Region start position
   * and ending at the region end position.
   */
  static std::shared_ptr<MidiRegion> prepare_region_with_note_at_start_to_end (
    Track *     track,
    midi_byte_t pitch,
    midi_byte_t velocity)
  {
    Position start_pos;
    Position end_pos;
    end_pos.set_to_bar (3);
    end_pos.add_beats (1);
    end_pos.add_ticks (3);
    start_pos.update_frames_from_ticks (0.0);
    end_pos.update_frames_from_ticks (0.0);
    auto r = std::make_shared<MidiRegion> (
      start_pos, end_pos, track->get_name_hash (), 0, 0);
    auto mn1 =
      std::make_shared<MidiNote> (r->id_, start_pos, end_pos, pitch, velocity);
    r->append_object (mn1);
    EXPECT_NONEMPTY (r->midi_notes_);

    return r;
  }

protected:
  MidiTrack *     midi_track_;
  MidiEventVector events_;
};

TEST_F (ZrythmFixture, FillMidiEventsFromEngine)
{
  /* deactivate audio engine processing so we can process
   * manually */
  test_project_stop_dummy_engine ();

  /* create an instrument track for testing */
  test_plugin_manager_create_tracks_from_plugin (
    TRIPLE_SYNTH_BUNDLE, TRIPLE_SYNTH_URI, true, false, 1);
  auto ins_track = TRACKLIST->get_last_track<InstrumentTrack> ();

  auto r =
    TrackFixture::prepare_region_with_note_at_start_to_end (ins_track, 35, 60);
  ins_track->add_region (r, nullptr, 0, true, false);
  AUDIO_ENGINE->run_.store (false);
  TRACKLIST->set_caches (CacheType::PlaybackSnapshots);
  AUDIO_ENGINE->run_.store (true);

  TRANSPORT->set_loop (true, true);
  TRANSPORT->loop_end_pos_ = r->end_pos_;
  Position pos;
  pos = r->end_pos_;
  pos.add_frames (-20);
  TRANSPORT->set_playhead_pos (pos);
  TRANSPORT->request_roll (true);

  /* run the engine for 1 cycle */
  z_info ("--- processing engine...");
  AUDIO_ENGINE->process (40);
  z_info ("--- processing recording events...");
  auto &pl = ins_track->channel_->instrument_;
  auto  event_in = dynamic_cast<MidiPort *> (pl->in_ports_[2].get ());
  auto &midi_events = event_in->midi_events_;
  ASSERT_SIZE_EQ (midi_events.active_events_, 3);
  auto ev = midi_events.active_events_.at (0);
  ASSERT_TRUE (midi_is_note_off (ev.raw_buffer_.data ()));
  ASSERT_EQ (ev.time_, 19);
  ASSERT_EQ (midi_get_note_number (ev.raw_buffer_.data ()), 35);
  ev = midi_events.active_events_.at (1);
  ASSERT_TRUE (midi_is_all_notes_off (ev.raw_buffer_.data ()));
  ASSERT_EQ (ev.time_, 19);
  ev = midi_events.active_events_.at (2);
  ASSERT_TRUE (midi_is_note_on (ev.raw_buffer_.data ()));
  ASSERT_EQ (ev.time_, 20);
  ASSERT_EQ (midi_get_note_number (ev.raw_buffer_.data ()), 35);
  ASSERT_EQ (midi_get_velocity (ev.raw_buffer_.data ()), 60);

  /* process again and check events are 0 */
  z_info ("--- processing engine...");
  AUDIO_ENGINE->process (40);
  z_info ("--- processing recording events...");
  ASSERT_EMPTY (midi_events.active_events_);
}

TEST_F (TrackFixture, FillMidiEvents)
{
  auto     track = midi_track_;
  auto    &events = events_;
  Position pos;

  midi_byte_t pitch1 = 35;
  midi_byte_t vel1 = 91;
  auto r = prepare_region_with_note_at_start_to_end (track, pitch1, vel1);
  track->add_region (r, nullptr, 0, true, false);
  auto mn = r->midi_notes_[0];

  /* -- BASIC TESTS (no loops) -- */

  /* deactivate audio engine processing so we can process
   * manually */
  AUDIO_ENGINE->activate (false);
  TRANSPORT->play_state_ = Transport::PlayState::Rolling;

  /* try basic fill */
  EngineProcessTimeInfo time_nfo = {
    .g_start_frame_ = (unsigned_frame_t) pos.frames_,
    .g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_,
    .local_offset_ = 0,
    .nframes_ = BUFFER_SIZE,
  };

  auto set_caches_and_fill = [&] () {
    TRACKLIST->set_caches (CacheType::PlaybackSnapshots);
    track->fill_midi_events (time_nfo, events);
  };

  {
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 1);
    auto ev = events.at (0);
    ASSERT_EQ (
      midi_get_channel_1_to_16 (ev.raw_buffer_.data ()), r->get_midi_ch ());
    ASSERT_EQ (midi_get_note_number (ev.raw_buffer_.data ()), pitch1);
    ASSERT_EQ (midi_get_velocity (ev.raw_buffer_.data ()), vel1);
    ASSERT_EQ ((long) ev.time_, pos.frames_);
    events.clear ();
  }

  /*
   * Start: region start + 1
   * End: region start + 1 + BUFFER_SIZE
   * Local offset (included above): 1
   *
   * Expected result:
   * MIDI note start is skipped (no events).
   */
  {
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_ + 1;
    time_nfo.local_offset_ = 1;
    time_nfo.nframes_ = BUFFER_SIZE;
    set_caches_and_fill ();
    ASSERT_EMPTY (events);
  }

  /*
   * Start: region start
   * End: region start + 1
   *
   * Expected result:
   * MIDI note is added even though the cycle is
   * only 1 sample.
   */
  {
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = 1;
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 1);
    events.clear ();
  }

  /*
   * Start: region start + BUFFER_SIZE
   * End: region start + BUFFER_SIZE * 2
   *
   * Expected result:
   * No MIDI notes in range.
   */
  {
    pos.add_frames (BUFFER_SIZE);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = BUFFER_SIZE;
    set_caches_and_fill ();
    ASSERT_EMPTY (events);
  }

  /*
   * Start: region end - (BUFFER_SIZE + 1)
   * End: region end - 1
   *
   * Expected result:
   * No MIDI notes in range just 1 sample before
   * the region end.
   */
  {
    pos = r->end_pos_;
    pos.add_frames ((-BUFFER_SIZE) - 1);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = BUFFER_SIZE;
    set_caches_and_fill ();
    ASSERT_EMPTY (events);
  }

  /*
   * Start: region end - BUFFER_SIZE
   * End: region end
   *
   * Expected result:
   * MIDI note off event ends 1 sample before the region end.
   */
  {
    pos = r->end_pos_;
    pos.add_frames (-BUFFER_SIZE);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = BUFFER_SIZE;
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 1);
    auto ev = events.front ();
    ASSERT_TRUE (midi_is_note_off (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, BUFFER_SIZE - 1);
    events.clear ();
  }

  /*
   * Start: midi end - BUFFER_SIZE
   * End: midi end
   *
   * Expected result:
   * Cycle ends on MIDI end so note off is fired
   * at the last sample because the actual MIDI end
   * is exclusive.
   */
  {
    mn->end_pos_.add_ticks (-5);
    mn->end_pos_.update_frames_from_ticks (0.0);
    pos = mn->end_pos_;
    pos.frames_ = mn->end_pos_.frames_;
    pos.add_frames (-BUFFER_SIZE);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = BUFFER_SIZE;
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 1);
    auto ev = events.at (0);
    ASSERT_EQ (ev.time_, BUFFER_SIZE - 1);
    mn->end_pos_ = r->end_pos_;
    events.clear ();
  }

  /*
   * Start: (midi end - BUFFER_SIZE) + 1
   * End: midi end + 1
   *
   * Expected result:
   * MIDI note off event is fired at the frame before
   * its actual end.
   */
  {
    mn->end_pos_.add_ticks (-5);
    mn->end_pos_.update_frames_from_ticks (0.0);
    pos = mn->end_pos_;
    pos.frames_ = mn->end_pos_.frames_;
    pos.add_frames ((-BUFFER_SIZE) + 1);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = BUFFER_SIZE;
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 1);
    auto ev = events.front ();
    ASSERT_EQ (ev.time_, BUFFER_SIZE - 2);
    mn->end_pos_ = r->end_pos_;
    events.clear ();
  }

  /*
   * Start: region end - (BUFFER_SIZE - 1)
   * End: region end + 1
   *
   * Expected result:
   * MIDI note off event is fired at the frame before
   * the region's actual end.
   */
  {
    pos = r->end_pos_;
    pos.add_frames ((-BUFFER_SIZE) + 1);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = BUFFER_SIZE;
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 1);
    auto ev = events.front ();
    ASSERT_TRUE (midi_is_note_off (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, BUFFER_SIZE - 2);
    events.clear ();
  }

  /*
   * Initialization
   *
   * Region <2.1.1.0 ~ 4.1.1.0>
   *   loop end 2.1.1.0
   * MidiNote <-1.1.1.1>
   */
  r->pos_.set_to_bar (2);
  r->end_pos_.set_to_bar (4);
  r->loop_end_pos_.set_to_bar (2);
  r->pos_.update_frames_from_ticks (0.0);
  r->end_pos_.update_frames_from_ticks (0.0);
  r->loop_end_pos_.update_frames_from_ticks (0.0);
  mn->pos_.set_to_bar (1);
  mn->pos_.add_ticks (-1);
  mn->pos_.update_frames_from_ticks (0.0);
  mn->end_pos_ = mn->pos_;
  mn->end_pos_.add_ticks (2000);
  mn->end_pos_.update_frames_from_ticks (0.0);

  /*
   * Premise: note starts 1 tick before region.
   * Start: before region start
   * End: after region start
   *
   * Expected result:
   * No MIDI note event.
   */
  {
    pos = r->pos_;
    pos.add_frames (-365);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = 512;
    set_caches_and_fill ();
    ASSERT_EMPTY (events);
  }

  /*
   * Initialization
   *
   * Region <2.1.1.0 ~ 4.1.1.0>
   *   loop start 1.1.1.7
   *   loop end 2.1.1.0
   * MidiNote <1.1.1.7 ~ 2.1.1.7>
   */
  r->pos_.set_to_bar (2);
  r->end_pos_.set_to_bar (4);
  r->loop_end_pos_.set_to_bar (2);
  r->loop_start_pos_.add_ticks (7);
  r->pos_.update_frames_from_ticks (0.0);
  r->end_pos_.update_frames_from_ticks (0.0);
  r->loop_start_pos_.update_frames_from_ticks (0.0);
  r->loop_end_pos_.update_frames_from_ticks (0.0);
  mn->pos_.set_to_bar (1);
  mn->pos_.add_ticks (7);
  mn->end_pos_.set_to_bar (2);
  mn->end_pos_.add_ticks (7);
  mn->pos_.update_frames_from_ticks (0.0);
  mn->end_pos_.update_frames_from_ticks (0.0);

  /*
   * Premise: note starts at the loop_start point after clip start.
   * Start: before first loop
   * End: after first loop
   *
   * Expected result:
   * MIDI note off at loop point - 1 sample.
   * MIDI note on at loop point.
   */
  {
    pos.set_to_bar (3);
    pos.add_frames (-365);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = 2000;
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 2);
    auto ev = events.front ();
    ASSERT_TRUE (midi_is_note_off (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, 364);
    ev = events.at (1);
    ASSERT_TRUE (midi_is_note_on (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, 365);
    events.clear ();
  }

  /* -- REGION LOOP TESTS -- */

  /*
   * Initialization
   *
   * Region <2.1.1.0 ~ 3.1.1.0>
   *   loop end (same as midi note end)
   * MidiNote <1.1.1.0 ~ (2000 ticks later)>
   */
  r->pos_.set_to_bar (2);
  r->end_pos_.set_to_bar (3);
  r->pos_.update_frames_from_ticks (0.0);
  r->end_pos_.update_frames_from_ticks (0.0);
  mn->pos_.set_to_bar (1);
  mn->pos_.update_frames_from_ticks (0.0);
  mn->end_pos_ = mn->pos_;
  mn->end_pos_.add_ticks (2000);
  mn->end_pos_.update_frames_from_ticks (0.0);
  r->loop_end_pos_ = mn->end_pos_;

  /*
   * Premise: note starts 1 tick before region.
   * Start: before region start
   * End: after region start
   *
   * Expected result:
   * No MIDI note event.
   */
  {
    mn->pos_.add_ticks (-1);
    mn->pos_.update_frames_from_ticks (0.0);
    pos = r->pos_;
    pos.add_frames (-6);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = BUFFER_SIZE;
    set_caches_and_fill ();
    ASSERT_EMPTY (events);
    mn->pos_.add_ticks (1);
    mn->pos_.update_frames_from_ticks (0.0);
  }

  /*
   * Start: way before region start
   * End: way before region start
   *
   * Expected result:
   * No MIDI notes.
   */
  {
    pos.set_to_bar (1);
    pos.update_frames_from_ticks (0.0);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = BUFFER_SIZE;
    set_caches_and_fill ();
    ASSERT_EMPTY (events);
  }

  /**
   * Start: frame after region end
   * End: BUFFER_SIZE after that way before transport
   * loop.
   *
   * Expected result:
   * No MIDI notes.
   */
  {
    pos.set_to_bar (3);
    pos.update_frames_from_ticks (0.0);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = BUFFER_SIZE;
    set_caches_and_fill ();
    ASSERT_EMPTY (events);
  }

  /**
   * Start: before region start
   * End: after region start
   *
   * Expected result:
   * MIDI note at correct time.
   */
  {
    pos = r->pos_;
    pos.add_frames (-10);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = BUFFER_SIZE;
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 1);
    auto ev = events.front ();
    ASSERT_EQ (ev.time_, 10);
    events.clear ();
  }

  /**
   * Start: before region start
   * End: after region start
   *
   * Expected result:
   * MIDI note at correct time.
   */
  {
    pos = r->pos_;
    pos.add_frames (-10);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = BUFFER_SIZE;
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 1);
    auto ev = events.front ();
    ASSERT_EQ (ev.time_, 10);
    events.clear ();
  }

  /*
   * Initialization
   *
   * Region <2.1.1.0 ~ 3.1.1.0>
   *   loop start <1.1.1.0>
   *   loop end (same as midi note end)
   * MidiNote <1.1.1.0 ~ (2000 ticks later)>
   */
  r->pos_.set_to_bar (2);
  r->end_pos_.set_to_bar (3);
  r->loop_start_pos_.set_to_bar (1);
  r->pos_.update_frames_from_ticks (0.0);
  r->end_pos_.update_frames_from_ticks (0.0);
  mn->pos_.set_to_bar (1);
  mn->pos_.update_frames_from_ticks (0.0);
  mn->end_pos_ = mn->pos_;
  mn->end_pos_.add_ticks (2000);
  mn->end_pos_.update_frames_from_ticks (0.0);
  r->loop_end_pos_ = mn->end_pos_;

  /**
   * Start: before region loop point
   * End: after region loop point
   *
   * Expected result:
   * MIDI note off at 1 sample before the loop point.
   * MIDI note on at the loop point.
   */
  {
    pos = r->loop_end_pos_;
    pos.add_frames (r->pos_.frames_);
    pos.add_frames (-10);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = BUFFER_SIZE;
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 2);
    auto ev = events.front ();
    ASSERT_TRUE (midi_is_note_off (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, 9);
    ev = events.at (1);
    ASSERT_TRUE (midi_is_note_on (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, 10);
    events.clear ();
  }

  /**
   * Premise: note ends on region end (no loops).
   * Start: before region end
   * End: after region end
   *
   * Expected result:
   * 1 MIDI note off, no notes on.
   */
  {
    r->pos_.set_to_bar (2);
    r->end_pos_.set_to_bar (3);
    r->loop_end_pos_.set_to_bar (2);
    mn->pos_.set_to_bar (1);
    mn->end_pos_.set_to_bar (2);
    pos = r->end_pos_;
    pos.add_frames (-10);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = BUFFER_SIZE;
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 1);
    auto ev = events.front ();
    ASSERT_TRUE (midi_is_note_off (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, 9);
    events.clear ();
  }

  /**
   * Premise:
   * Region <2.1.1.0 ~ 4.1.1.0>
   *   loop start <1.1.1.0>
   *   clip start <1.3.1.0>
   *   loop end <2.1.1.0>
   * MidiNote <1.1.1.0 ~ (400 frames)>
   * Playhead <3.1.1.0>
   *
   * Expected result:
   * 1 MIDI note on.
   */
  {
    r->pos_.set_to_bar (2);
    r->end_pos_.set_to_bar (4);
    r->loop_start_pos_.set_to_bar (1);
    r->loop_end_pos_.set_to_bar (2);
    mn->pos_.set_to_bar (1);
    pos.set_to_bar (1);
    pos.add_frames (400);
    mn->end_pos_ = pos;
    pos.set_to_bar (3);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = BUFFER_SIZE;
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 1);
    auto ev = events.front ();
    ASSERT_TRUE (midi_is_note_on (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, 0);
    events.clear ();
  }

  /**
   * Premise: note starts at 1.1.1.0 and ends right before transport loop end.
   *
   * Expected result:
   * Note off before loop end and note on at
   * 1.1.1.0.
   */
  {
    r->pos_.set_to_bar (1);
    r->end_pos_ = TRANSPORT->loop_end_pos_;
    r->loop_start_pos_.set_to_bar (1);
    r->loop_end_pos_ = TRANSPORT->loop_end_pos_;
    mn->pos_.set_to_bar (1);
    pos = TRANSPORT->loop_end_pos_;
    pos.add_frames (-20);
    mn->end_pos_ = pos;
    pos = TRANSPORT->loop_end_pos_;
    pos.add_frames (-30);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = 30;
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 2);
    auto ev = events.front ();
    ASSERT_TRUE (midi_is_note_off (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, 9);
    ev = events.at (1);
    ASSERT_TRUE (midi_is_all_notes_off (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, 29);
    events.clear ();
  }

  {
    pos = TRANSPORT->loop_start_pos_;
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = 10;
    set_caches_and_fill ();
    auto ev = events.front ();
    ASSERT_TRUE (midi_is_note_on (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, 0);
    events.clear ();
  }

  /**
   * Premise (bug #3317):
   * Region: <1.2.1.0 ~ 5.4.1.0> (loop end 4.2.1.0)
   * Note: <2.4.3.37.2 ~ 4.2.3.37.2>
   *
   * Expected result:
   * Note on at 2.4.3.37.2 and note off at 4.2.1.0.
   */
  {
    r->pos_ = Position ("1.2.1.0.0");
    r->end_pos_ = Position ("5.4.1.0.0");
    r->loop_end_pos_ = Position ("4.2.1.0.0");
    TRANSPORT->loop_start_pos_.set_to_bar (1);
    TRANSPORT->loop_end_pos_.set_to_bar (8);
    mn->pos_ = Position ("2.4.3.37.2");
    mn->end_pos_ = Position ("4.2.3.37.2");

    /* check note on */
    time_nfo.g_start_frame_ =
      (unsigned_frame_t) r->pos_.frames_ + (unsigned_frame_t) mn->pos_.frames_
      - 4;
    time_nfo.g_start_frame_w_offset_ =
      (unsigned_frame_t) r->pos_.frames_ + (unsigned_frame_t) mn->pos_.frames_
      - 4;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = 30;
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 1);
    auto ev = events.front ();
    ASSERT_TRUE (midi_is_note_on (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, 4);
    events.clear ();

    /* check note off at region loop */
    time_nfo.g_start_frame_ =
      (unsigned_frame_t) r->pos_.frames_
      + (unsigned_frame_t) r->loop_end_pos_.frames_ - 4;
    time_nfo.g_start_frame_w_offset_ =
      (unsigned_frame_t) r->pos_.frames_
      + (unsigned_frame_t) r->loop_end_pos_.frames_ - 4;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = 30;
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 1);
    ev = events.front ();
    ASSERT_TRUE (midi_is_note_off (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, 3);
    events.clear ();
  }

  /**
   *
   * Premise: note starts inside region and ends outside it.
   *
   * Expected result:
   * Note on inside region and note off at region end.
   */
  {
    r->pos_.set_to_bar (1);
    r->end_pos_.set_to_bar (2);
    r->loop_start_pos_.set_to_bar (1);
    r->loop_end_pos_.set_to_bar (2);
    mn->pos_.set_to_bar (2);
    mn->pos_.add_frames (-30);
    mn->end_pos_.set_to_bar (3);
    pos = mn->pos_;
    pos.add_frames (-10);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = 50;
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 2);
    auto ev = events.front ();
    ASSERT_TRUE (midi_is_note_on (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, 10);
    ev = events.at (1);
    ASSERT_TRUE (midi_is_note_off (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, 39);
    events.clear ();
  }

  /**
   * Premise: region loops back near the end.
   *
   * Expected result:
   * Note on for a few samples then all notes off.
   */
  {
    r->pos_.set_to_bar (1);
    r->end_pos_.set_to_bar (2);
    r->loop_start_pos_.set_to_bar (1);
    r->loop_end_pos_.set_to_bar (2);
    r->loop_end_pos_.add_frames (-10);
    mn->pos_.set_to_bar (1);
    mn->end_pos_.set_to_bar (1);
    mn->end_pos_.add_beats (1);
    pos = r->loop_end_pos_;
    pos.add_frames (-5);
    time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
    time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
    time_nfo.local_offset_ = 0;
    time_nfo.nframes_ = 50;
    set_caches_and_fill ();
    ASSERT_SIZE_EQ (events, 2);
    auto ev = events.front ();
    ASSERT_TRUE (midi_is_note_on (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, 5);
    ev = events.at (1);
    ASSERT_TRUE (midi_is_all_notes_off (ev.raw_buffer_.data ()));
    ASSERT_EQ (ev.time_, 14);
    events.clear ();
  }
}
