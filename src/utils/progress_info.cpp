// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/objects.h"
#include "utils/progress_info.h"
#include "utils/types.h"

void
ProgressInfo::request_cancellation ()
{
  if (status_ == COMPLETED)
    {
      g_warning ("requested cancellation but task already completed");
      return;
    }

  std::scoped_lock guard (m_);

  if (status_ != RUNNING)
    {
      g_critical ("invalid status %s", ENUM_NAME (status_));
      return;
    }

  status_ = PENDING_CANCELLATION;
}

/**
 * To be called by the task itself.
 */
void
ProgressInfo::mark_completed (CompletionType type, const char * msg)
{
  std::scoped_lock guard (m_);

  status_ = COMPLETED;
  completion_type_ = type;
  completion_str_ = msg;

  if (type == HAS_WARNING)
    {
      g_message ("progress warning: %s", msg);
    }
  else if (type == HAS_ERROR)
    {
      g_message ("progress error: %s", msg);
    }
}

void
ProgressInfo::update_progress (double progress, const char * msg)
{
  std::scoped_lock guard (m_);

  progress_str_ = msg;
  progress_ = progress;
  if (status_ == PENDING_START)
    {
      status_ = RUNNING;
    }
}
