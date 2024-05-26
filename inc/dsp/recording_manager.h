// SPDX-FileCopyrightText: © 2019, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Handles recording.
 */

#ifndef __AUDIO_RECORDING_MANAGER_H__
#define __AUDIO_RECORDING_MANAGER_H__

#include "dsp/region_identifier.h"
#include "utils/types.h"

#include "zix/sem.h"

typedef struct ObjectPool         ObjectPool;
typedef struct TrackProcessor     TrackProcessor;
typedef struct MPMCQueue          MPMCQueue;
typedef struct ArrangerSelections ArrangerSelections;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define RECORDING_MANAGER (gZrythm->recording_manager)

typedef struct RecordingManager
{
  /** Number of recordings currently in progress. */
  int num_active_recordings;

  /** Event queue. */
  MPMCQueue * event_queue;

  /**
   * Memory pool of event structs to avoid real time
   * allocation.
   */
  ObjectPool * event_obj_pool;

  /** Cloned selections before starting recording. */
  ArrangerSelections * selections_before_start;

  /** Source func ID. */
  guint source_id;

  /**
   * Recorded region identifiers, to be used for
   * creating the undoable actions.
   *
   * TODO use region pointers ?
   */
  RegionIdentifier recorded_ids[8000];
  int              num_recorded_ids;

  /** Pending recorded automation points. */
  GPtrArray * pending_aps;

  bool   currently_processing;
  ZixSem processing_sem;

  bool freeing;
} RecordingManager;

/**
 * Handles the recording logic inside the process
 * cycle.
 *
 * The MidiEvents are already dequeued at this
 * point.
 *
 * @param g_frames_start Global start frames.
 * @param nframes Number of frames to process. If
 *   this is zero, a pause will be added. See \ref
 *   RECORDING_EVENT_TYPE_PAUSE_TRACK_RECORDING and
 *   RECORDING_EVENT_TYPE_PAUSE_AUTOMATION_RECORDING.
 */
REALTIME
void
recording_manager_handle_recording (
  RecordingManager *            self,
  const TrackProcessor *        track_processor,
  const EngineProcessTimeInfo * time_nfo);

/**
 * GSourceFunc to be added using idle add.
 *
 * This will loop indefinintely.
 *
 * It can also be called to process the events
 * immediately. Should only be called from the
 * GTK thread.
 */
int
recording_manager_process_events (RecordingManager * self);

/**
 * Creates the event queue and starts the event loop.
 *
 * Must be called from a GTK thread.
 */
RecordingManager *
recording_manager_new ();

void
recording_manager_free (RecordingManager * self);

/**
 * @}
 */

#endif
