/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Handles recording.
 */

#ifndef __AUDIO_RECORDING_MANAGER_H__
#define __AUDIO_RECORDING_MANAGER_H__

#include "utils/types.h"

#include "zix/sem.h"

typedef struct ObjectPool     ObjectPool;
typedef struct TrackProcessor TrackProcessor;
typedef struct MPMCQueue      MPMCQueue;

/**
 * @addtogroup audio
 *
 * @{
 */

#define RECORDING_MANAGER \
  (ZRYTHM->recording_manager)

typedef struct RecordingManager
{
  /** Number of recordings currently in progress. */
  int num_active_recordings;

  /** Event queue. */
  MPMCQueue * event_queue;

  /**
   * Object pool of event structs to avoid real time
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
  RecordingManager *     self,
  const TrackProcessor * track_processor,
  const EngineProcessTimeInfo * const time_nfo);

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
recording_manager_process_events (
  RecordingManager * self);

/**
 * Creates the event queue and starts the event loop.
 *
 * Must be called from a GTK thread.
 */
RecordingManager *
recording_manager_new (void);

void
recording_manager_free (RecordingManager * self);

/**
 * @}
 */

#endif
