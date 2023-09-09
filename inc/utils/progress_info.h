// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Progress info.
 */

#ifndef __UTILS_PROGRESS_INFO_H__
#define __UTILS_PROGRESS_INFO_H__

#include <stdbool.h>

/**
 * @addtogroup utils
 *
 * @{
 */

typedef enum ProgressStatus
{
  PROGRESS_STATUS_PENDING_START,
  PROGRESS_STATUS_PENDING_CANCELLATION,
  PROGRESS_STATUS_RUNNING,
  PROGRESS_STATUS_COMPLETED,
} ProgressStatus;

typedef enum ProgressCompletionType
{
  PROGRESS_COMPLETED_CANCELLED,
  PROGRESS_COMPLETED_HAS_WARNING,
  PROGRESS_COMPLETED_HAS_ERROR,
  PROGRESS_COMPLETED_SUCCESS,
} ProgressCompletionType;

typedef struct ProgressInfo ProgressInfo;

ProgressInfo *
progress_info_new (void);

ProgressStatus
progress_info_get_status (ProgressInfo * self);

ProgressCompletionType
progress_info_get_completion_type (ProgressInfo * self);

/**
 * To be called by the task caller.
 */
void
progress_info_request_cancellation (ProgressInfo * self);

/**
 * To be called by the task itself.
 */
void
progress_info_mark_completed (
  ProgressInfo *         self,
  ProgressCompletionType type,
  const char *           msg);

/**
 * Returns a newly allocated string.
 */
char *
progress_info_get_message (ProgressInfo * self);

/**
 * To be called by the task caller.
 */
void
progress_info_get_progress (ProgressInfo * self, double * progress, char ** str);

/**
 * To be called by the task itself.
 */
void
progress_info_update_progress (
  ProgressInfo * self,
  double         progress,
  const char *   msg);

bool
progress_info_pending_cancellation (ProgressInfo * self);

void
progress_info_free (ProgressInfo * self);

/**
 * @}
 */

#endif
