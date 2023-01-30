// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_LOG_H__
#define __UTILS_LOG_H__

#include <stdbool.h>

#include <gtk/gtk.h>

typedef struct _LogViewerWidget LogViewerWidget;
typedef struct MPMCQueue        MPMCQueue;
typedef struct ObjectPool       ObjectPool;

/**
 * @addtogroup utils
 *
 * @{
 */

#define LOG (zlog)

typedef struct Log
{
  FILE * logfile;

#if 0
  /* Buffers to fill in */
  GtkTextBuffer * messages_buf;
  GtkTextBuffer * warnings_buf;
  GtkTextBuffer * critical_buf;
#endif

  /** Current log file path. */
  char * log_filepath;

  /** Message queue, for when messages are sent
   * from a non-gtk thread. */
  MPMCQueue * mqueue;

  /** Object pool for the queue. */
  ObjectPool * obj_pool;

  /** Used by the writer func. */
  char * log_domains;

  bool initialized;

  /**
   * Whether to use structured log when writing
   * to the console.
   *
   * Used during tests.
   */
  bool use_structured_for_console;

  /**
   * Minimum log level for the console.
   *
   * Used during tests.
   */
  GLogLevelFlags min_log_level_for_test_console;

  /** Currently opened log viewer. */
  LogViewerWidget * viewer;

  /** ID of the source function. */
  guint writer_source_id;

  /**
   * Last timestamp a backtrace was obtained.
   *
   * This is used to avoid calculating too many
   * backtraces and showing too many error
   * popups at once.
   */
  gint64 last_bt_time;
} Log;

/** Global variable, available to all files. */
extern Log * zlog;

/**
 * Initializes logging to a file.
 *
 * This must be called from the GTK thread.
 *
 * @param secs Number of timeout seconds.
 */
NONNULL void
log_init_writer_idle (Log * self, unsigned int secs);

/**
 * Idle callback.
 */
NONNULL int
log_idle_cb (Log * self);

/**
 * Returns the last \ref n lines as a newly
 * allocated string.
 *
 * @note This must only be called from the GTK
 *   thread.
 *
 * @param n Number of lines.
 */
NONNULL char *
log_get_last_n_lines (Log * self, int n);

/**
 * Generates a compressed log file (for sending with
 * bug reports).
 *
 * @return Whether successful.
 */
NONNULL_ARGS (1, 2, 3)
bool log_generate_compressed_file (
  Log *     self,
  char **   ret_dir,
  char **   ret_path,
  GError ** error);

/**
 * Initializes logging to a file.
 *
 * This can be called from any thread.
 *
 * @param filepath If non-NULL, the given file will be used,
 *   otherwise the default file will be created.
 *
 * @return Whether successful.
 */
WARN_UNUSED_RESULT bool
log_init_with_file (
  Log *        self,
  const char * filepath,
  GError **    error);

/**
 * Returns a pointer to the global zlog.
 */
CONST
WARN_UNUSED_RESULT Log **
log_get (void);

/**
 * Creates the logger and sets the writer func.
 *
 * This can be called from any thread.
 */
Log *
log_new (void);

/**
 * Stops logging and frees any allocated memory.
 */
NONNULL void
log_free (Log * self);

/**
 * @}
 */

#endif
