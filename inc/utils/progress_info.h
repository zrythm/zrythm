// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Progress info.
 */

#ifndef __UTILS_PROGRESS_INFO_H__
#define __UTILS_PROGRESS_INFO_H__

#include <atomic>
#include <mutex>
#include <string>
#include <utility>

#include <glib.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Generic progress info.
 *
 * @note Not realtime-safe.
 */
class ProgressInfo
{

public:
  enum Status
  {
    PENDING_START,
    PENDING_CANCELLATION,
    RUNNING,
    COMPLETED,
  };

  enum CompletionType
  {
    CANCELLED,
    HAS_WARNING,
    HAS_ERROR,
    SUCCESS,
  };

  Status get_status () const { return status_; };

  CompletionType get_completion_type () const
  {
    g_return_val_if_fail (status_ == COMPLETED, HAS_ERROR);
    return completion_type_;
  }

  /**
   * To be called by the task caller.
   */
  void request_cancellation ();

  /**
   * To be called by the task itself.
   */
  void mark_completed (CompletionType type, const char * msg);

  /**
   * Returns a newly allocated string.
   */
  char * get_message () const { return g_strdup (completion_str_.c_str ()); }

  /**
   * To be called by the task caller.
   */
  std::tuple<double, std::string> get_progress ()
  {
    std::scoped_lock guard (m_);

    return std::make_tuple (progress_, progress_str_);
  }

  /**
   * To be called by the task itself.
   */
  void update_progress (double progress, const char * msg);

  bool pending_cancellation () const
  {
    return status_ == PENDING_CANCELLATION;
  };

private:
  /** Progress done (0.0 to 1.0). */
  double progress_ = 0;

  /** Current running status. */
  Status status_ = (Status) 0;

  /** Status to be checked after completion. */
  CompletionType completion_type_ = (CompletionType) 0;

  /** Message to show after completion (error or warning or
   * success message). */
  std::string completion_str_;

  /** String to show during the action (can be updated multiple
   * times until completion). */
  std::string progress_str_;

  /** String to show in the label when the action is complete
   * (progress == 1.0). */
  // std::string label_done_str;

  /** Mutex to prevent concurrent access/edits. */
  std::mutex m_;
};

/**
 * @}
 */

#endif
