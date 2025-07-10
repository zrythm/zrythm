// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <format>

#include "dsp/midi_event.h"
#include "structure/tracks/track.h"
#include "utils/types.h"

namespace zrythm::engine::session
{

/**
 * A recording event.
 *
 * During recording, a recording event must be sent in each cycle for all
 * record-enabled tracks.
 *
 * Recording events are queued for the recording thread to handle.
 */
class RecordingEvent
{
public:
  enum class Type
  {
    StartTrackRecording,
    StartAutomationRecording,

    /** These events are for processing any range. */
    Midi,
    Audio,
    Automation,

    /**
     * These events are for temporarily stopping recording (eg, when outside the
     * punch range or when looping).
     *
     * The nframes must always be 0 for these events.
     */
    PauseTrackRecording,
    PauseAutomationRecording,

    StopTrackRecording,
    StopAutomationRecording,
  };

public:
  void init (
    const Type                      type,
    const structure::tracks::Track &track,
    const EngineProcessTimeInfo    &time_nfo,
#if defined(__GNUC__) || defined(__clang__)
    const char * file = __builtin_FILE (),
    const char * func = __builtin_FUNCTION (),
    int          lineno = __builtin_LINE ())
#else
    const char * file = nullptr,
    const char * func = nullptr,
    int          lineno = 0)
#endif
  {
    type_ = type;
    track_uuid_ = track.get_uuid ();
    g_start_frame_w_offset_ = time_nfo.g_start_frame_w_offset_;
    local_offset_ = time_nfo.local_offset_;
    has_midi_event_ = false;
    midi_event_ = dsp::MidiEvent{};
    automation_track_idx_ = 0;
    nframes_ = time_nfo.nframes_;
    file_ = file;
    func_ = func;
    lineno_ = lineno;
  }

public:
  Type type_ = Type::Audio;

  bool has_midi_event_ = false;

  /** The identifier of the track this event is for. */
  structure::tracks::TrackUuid track_uuid_;

  /** Global start frames of the event (including offset). */
  unsigned_frame_t g_start_frame_w_offset_ = 0;

  /** Offset in current cycle that this event starts from. */
  nframes_t local_offset_ = 0;

  /** Number of frames processed in this event. */
  nframes_t nframes_ = 0;

  /** Index of automation track, if automation. */
  int automation_track_idx_ = 0;

  /**
   * The actual data (if audio).
   *
   * This will be @ref nframes_ times the number of channels in the track.
   */
  std::array<float, 8192> lbuf_{};
  std::array<float, 8192> rbuf_{};

  /**
   * MidiEvent, if midi.
   */
  dsp::MidiEvent midi_event_;

  /* debug info */
  const char * file_ = nullptr;
  const char * func_ = nullptr;
  int          lineno_ = 0;

  BOOST_DESCRIBE_CLASS (
    RecordingEvent,
    (),
    (type_,
     track_uuid_,
     g_start_frame_w_offset_,
     local_offset_,
     nframes_,
     automation_track_idx_,
     has_midi_event_,
     file_,
     func_,
     lineno_),
    (),
    ())
};
} // namespace zrythm::engine::session
