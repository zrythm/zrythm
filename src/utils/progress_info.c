// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/objects.h"
#include "utils/progress_info.h"

/**
 * Generic progress info.
 *
 * @note Not realtime-safe.
 */
typedef struct ProgressInfo
{
  /** Progress done (0.0 to 1.0). */
  double progress;

  /** Current running status. */
  ProgressStatus status;

  /** Status to be checked after completion. */
  ProgressCompletionType completion_type;

  /** Message to show after completion (error or warning or
   * success message). */
  char * completion_str;

  /** String to show during the action (can be updated multiple
   * times until completion). */
  char * progress_str;

  /** String to show in the label when the action is complete
   * (progress == 1.0). */
  //char * label_done_str;

  /** Mutex to prevent concurrent access/edits. */
  GMutex mutex;
} ProgressInfo;

ProgressInfo *
progress_info_new (void)
{
  ProgressInfo * self = object_new_unresizable (ProgressInfo);

  g_mutex_init (&self->mutex);

  return self;
}

ProgressStatus
progress_info_get_status (ProgressInfo * self)
{
  return self->status;
}

ProgressCompletionType
progress_info_get_completion_type (ProgressInfo * self)
{
  g_return_val_if_fail (
    self->status == PROGRESS_STATUS_COMPLETED, PROGRESS_COMPLETED_HAS_ERROR);
  return self->completion_type;
}

/**
 * To be called by the task caller.
 */
void
progress_info_request_cancellation (ProgressInfo * self)
{
  if (self->status == PROGRESS_STATUS_COMPLETED)
    {
      g_warning ("requested cancellation but task already completed");
      return;
    }

  g_mutex_lock (&self->mutex);

  if (self->status != PROGRESS_STATUS_RUNNING)
    {
      g_critical ("invalid status %d", self->status);
      g_mutex_unlock (&self->mutex);
      return;
    }

  self->status = PROGRESS_STATUS_PENDING_CANCELLATION;

  g_mutex_unlock (&self->mutex);
}

bool
progress_info_pending_cancellation (ProgressInfo * self)
{
  return self->status == PROGRESS_STATUS_PENDING_CANCELLATION;
}

/**
 * To be called by the task itself.
 */
void
progress_info_mark_completed (
  ProgressInfo *         self,
  ProgressCompletionType type,
  const char *           msg)
{
  g_mutex_lock (&self->mutex);

  self->status = PROGRESS_STATUS_COMPLETED;
  self->completion_type = type;
  self->completion_str = g_strdup (msg);

  if (type == PROGRESS_COMPLETED_HAS_WARNING)
    {
      g_message ("progress warning: %s", msg);
    }
  else if (type == PROGRESS_COMPLETED_HAS_ERROR)
    {
      g_message ("progress error: %s", msg);
    }

  g_mutex_unlock (&self->mutex);
}

/**
 * Returns a newly allocated string.
 */
char *
progress_info_get_message (ProgressInfo * self)
{
  return g_strdup (self->completion_str);
}

/**
 * To be called by the task caller.
 */
void
progress_info_get_progress (ProgressInfo * self, double * progress, char ** str)
{
  g_mutex_lock (&self->mutex);

  if (str)
    {
      *str = g_strdup (self->progress_str);
    }
  *progress = self->progress;

  g_mutex_unlock (&self->mutex);
}

/**
 * To be called by the task itself.
 */
void
progress_info_update_progress (
  ProgressInfo * self,
  double         progress,
  const char *   msg)
{
  g_mutex_lock (&self->mutex);

  g_free_and_null (self->progress_str);
  self->progress_str = g_strdup (msg);
  self->progress = progress;
  if (self->status == PROGRESS_STATUS_PENDING_START)
    {
      self->status = PROGRESS_STATUS_RUNNING;
    }

  g_mutex_unlock (&self->mutex);
}

void
progress_info_free (ProgressInfo * self)
{
  g_return_if_fail (self->status == PROGRESS_STATUS_COMPLETED);
  g_mutex_clear (&self->mutex);
  g_free_and_null (self->completion_str);
  g_free_and_null (self->progress_str);
  object_zero_and_free_unresizable (ProgressInfo, self);
}
