// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Recording events to queue for the recording
 * thread to handle.
 */

#ifndef __AUDIO_RECORDING_EVENTS_H__
#define __AUDIO_RECORDING_EVENTS_H__

#include "dsp/midi_event.h"
#include "dsp/port_identifier.h"
#include "utils/types.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

#define recording_event_queue_push_back_event(q, x) \
  mpmc_queue_push_back (q, (void *) x)

#define recording_event_queue_dequeue_event(q, x) \
  mpmc_queue_dequeue (q, (void **) x)

/**
 * Push events.
 */
#define RECORDING_EVENTS_PUSH_AUDIO(et, _arg) \
  if (RECORDING_MANAGER->event_queue) \
    { \
      RecordingEvent * ev = (RecordingEvent *) object_pool_get ( \
        RECORDING_MANAGER->event_obj_pool); \
      ev->type = et; \
      ev->arg = (void *) _arg; \
      event_queue_push_back_event (RECORDING_MANAGER->event_queue, _ev); \
    }

enum class RecordingEventType
{
  RECORDING_EVENT_TYPE_START_TRACK_RECORDING,
  RECORDING_EVENT_TYPE_START_AUTOMATION_RECORDING,

  /** These events are for processing any range. */
  RECORDING_EVENT_TYPE_MIDI,
  RECORDING_EVENT_TYPE_AUDIO,
  RECORDING_EVENT_TYPE_AUTOMATION,

  /**
   * These events are for temporarily stopping
   * recording (eg, when outside the punch range or
   * when looping).
   *
   * The nframes must always be 0 for these events.
   */
  RECORDING_EVENT_TYPE_PAUSE_TRACK_RECORDING,
  RECORDING_EVENT_TYPE_PAUSE_AUTOMATION_RECORDING,

  RECORDING_EVENT_TYPE_STOP_TRACK_RECORDING,
  RECORDING_EVENT_TYPE_STOP_AUTOMATION_RECORDING,
};

/**
 * A recording event.
 *
 * During recording, a recording event must be sent
 * in each cycle for all record-enabled tracks.
 */
typedef struct RecordingEvent
{
  RecordingEventType type;

  /** The name of the track this event is for. */
  unsigned int track_name_hash;

  /** Region name, if applicable. */
  char region_name[200];

  /** Global start frames of the event (including offset). */
  unsigned_frame_t g_start_frame_w_offset;

  /** Offset in current cycle that this event starts from. */
  nframes_t local_offset;

  /**
   * The actual data (if audio).
   *
   * This will be \ref RecordingEvent.nframes times
   * the number of channels in the track.
   */
  float lbuf[9000];
  float rbuf[9000];

  int has_midi_event;

  /**
   * MidiEvent, if midi.
   */
  MidiEvent midi_event;

  /** Index of automation track, if automation. */
  int automation_track_idx;

  /** Automation value, if automation. */
  // float             control_val;

  /** Number of frames processed in this event. */
  nframes_t nframes;

  /* debug info */
  const char * file;
  const char * func;
  int          lineno;
} RecordingEvent;

/**
 * Inits an already allocated recording event.
 */
#define recording_event_init(re) \
  re->type = ENUM_INT_TO_VALUE (RecordingEventType, 0); \
  re->track_name_hash = 0; \
  re->region_name[0] = '\0'; \
  re->g_start_frame_w_offset = 0; \
  re->local_offset = 0; \
  re->has_midi_event = 0; \
  memset (&re->midi_event, 0, sizeof (MidiEvent)); \
  re->automation_track_idx = 0; \
  re->nframes = 0; \
  re->file = __FILE__; \
  re->func = __func__; \
  re->lineno = __LINE__

COLD MALLOC RecordingEvent *
recording_event_new (void);

void
recording_event_print (RecordingEvent * self);

void
recording_event_free (RecordingEvent * self);

/**
 * @}
 */

#endif
