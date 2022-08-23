// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Generic progress dialog.
 */

#ifndef __GUI_WIDGETS_GENERIC_PROGRESS_DIALOG_H__
#define __GUI_WIDGETS_GENERIC_PROGRESS_DIALOG_H__

#include <stdbool.h>

#include <gtk/gtk.h>

#define GENERIC_PROGRESS_DIALOG_WIDGET_TYPE \
  (generic_progress_dialog_widget_get_type ())
G_DECLARE_DERIVABLE_TYPE (
  GenericProgressDialogWidget,
  generic_progress_dialog_widget,
  Z,
  GENERIC_PROGRESS_DIALOG_WIDGET,
  GtkDialog)

typedef struct GenericProgressInfo GenericProgressInfo;

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct GenericProgressDialogButton
{
  GtkButton * btn;

  /**
   * Only show the button when the action is finished.
   */
  bool only_on_finish;
} GenericProgressDialogButton;

/**
 * A generic progress dialog.
 */
typedef struct
{
  GtkLabel *       label;
  GtkProgressBar * progress_bar;
  GtkButton *      ok;
  GtkButton *      cancel;

  GtkBox * action_btn_box;

  /**
   * Whether to automatically close the progress
   * dialog when finished.
   *
   * This will hide the OK button.
   */
  bool autoclose;

  /**
   * Whether to allow canceling the action.
   *
   * Will show a Cancel button if true.
   */
  bool cancelable;

  GenericProgressInfo * progress_info;

  /**
   * Additional buttons.
   */
  GenericProgressDialogButton extra_buttons[10];
  size_t                      num_extra_buttons;

} GenericProgressDialogWidgetPrivate;

typedef struct _GenericProgressDialogWidgetClass
{
  GtkDialogClass parent_class;
} GenericProgressDialogWidgetClass;

GenericProgressDialogWidget *
generic_progress_dialog_widget_new (void);

/**
 * Sets up a progress dialog widget.
 */
void
generic_progress_dialog_widget_setup (
  GenericProgressDialogWidget * self,
  const char *                  title,
  GenericProgressInfo *         progress_info,
  bool                          autoclose,
  bool                          cancelable);

/**
 * Adds a button at the start or end of the button box.
 *
 * @param start Whether to add the button at the
 *   start of the button box, otherwise the button
 *   will be added at the end.
 */
void
generic_progress_dialog_add_button (
  GenericProgressDialogWidget * self,
  GtkButton *                   btn,
  bool                          start,
  bool                          only_on_finish);

/**
 * @}
 */

#endif
