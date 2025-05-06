// SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/logger.h"
#include "utils/progress_info.h"
#include "utils/types.h"

void
ProgressInfo::request_cancellation ()
{
  if (status_ == COMPLETED)
    {
      z_warning ("requested cancellation but task already completed");
      return;
    }

  std::scoped_lock guard (m_);

  if (status_ != RUNNING)
    {
      z_error ("invalid status {}", ENUM_NAME (status_));
      return;
    }

  status_ = PENDING_CANCELLATION;
}

/**
 * To be called by the task itself.
 */
void
ProgressInfo::mark_completed (CompletionType type, const utils::Utf8String &msg)
{
  std::scoped_lock guard (m_);

  status_ = COMPLETED;
  completion_type_ = type;
  if (!msg.empty ())
    completion_str_ = msg;

  if (type == HAS_WARNING)
    {
      z_return_if_fail (!msg.empty ());
      z_info ("progress warning: {}", msg);
    }
  else if (type == HAS_ERROR)
    {
      z_return_if_fail (!msg.empty ());
      z_info ("progress error: {}", msg);
    }
}

void
ProgressInfo::update_progress (double progress, const utils::Utf8String &msg)
{
  std::scoped_lock guard (m_);

  if (!msg.empty ())
    {
      progress_str_ = msg;
    }
  progress_ = progress;
  if (status_ == PENDING_START)
    {
      status_ = RUNNING;
    }
}
