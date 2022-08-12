// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Bug report/backtrace dialog.
 */

#ifndef __GUI_WIDGETS_BUG_REPORT_DIALOG_H__
#define __GUI_WIDGETS_BUG_REPORT_DIALOG_H__

#include <gtk/gtk.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtksourceview/gtksource.h>
#pragma GCC diagnostic pop

/**
 * @addtogroup widgets
 *
 * @{
 */

#define BUG_REPORT_DIALOG_WIDGET_TYPE \
  (bug_report_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  BugReportDialogWidget,
  bug_report_dialog_widget,
  Z,
  BUG_REPORT_DIALOG_WIDGET,
  GtkDialog)

typedef struct _BugReportDialogWidget
{
  GtkDialog parent_instance;

  GtkLabel * top_lbl;

  GtkLabel * backtrace_lbl;
  GtkLabel * log_lbl;
  GtkLabel * system_info_lbl;

  GtkTextView * user_input_text_view;

  char * log;
  char * log_long;
  char * undo_stack;
  char * undo_stack_long;
  char * backtrace;
  char * system_nfo;
  bool   fatal;

} BugReportDialogWidget;

/**
 * Creates and displays the about dialog.
 */
BugReportDialogWidget *
bug_report_dialog_new (
  GtkWindow *  parent,
  const char * msg_prefix,
  const char * backtrace,
  bool         fatal);

/**
 * @}
 */

#endif
