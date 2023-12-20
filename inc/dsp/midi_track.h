// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MIDI_TRACK_H__
#define __AUDIO_MIDI_TRACK_H__

typedef struct Position   Position;
typedef struct MidiEvents MidiEvents;
typedef struct Track      Track;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Initializes an midi track.
 */
void
midi_track_init (Track * track);

void
midi_track_setup (Track * self);

/**
 * Fills MIDI event queue from track.
 *
 * The events are dequeued right after the call to
 * this function.
 *
 * @note The engine splits the cycle so transport
 *   loop related logic is not needed.
 *
 * Caveats:
 * - This will not work properly if the loop sizes
 *   (region or transport) are smaller than nframes,
 *   so small sizes should not be allowed.
 *
 * @param g_start_frames Global start frame.
 * @param local_start_frame The start frame offset
 *   from 0 in this cycle.
 * @param nframes Number of frames at start
 *   Position.
 * @param midi_events MidiEvents to fill (from
 *   Piano Roll Port for example).
 */
REALTIME
void
midi_track_fill_midi_events (
  Track *         track,
  const long      g_start_frames,
  const nframes_t local_start_frame,
  nframes_t       nframes,
  MidiEvents *    midi_events);

/**
 * Set MIDI CC's to Touch mode temporarily.
 *
 * Used when starting recording on a MIDI-based track.
 */
void
midi_track_enable_automation_auto_record (Track * self);

/**
 * Unset MIDI CC Touch mode if set temporarily.
 *
 * Used when stopping recording on a MIDI-based track.
 */
void
midi_track_disable_automation_auto_record (Track * self);

/**
 * Frees the track.
 *
 * TODO
 */
void
midi_track_free (Track * track);

/**
 * @}
 */

#endif // __AUDIO_MIDI_TRACK_H__
