// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_GENERIC_PROGRESS_DIALOG_H__
#define __GUI_WIDGETS_GENERIC_PROGRESS_DIALOG_H__

#include "utils/types.h"

#include "libadwaita_wrapper.h"

#define GENERIC_PROGRESS_DIALOG_WIDGET_TYPE \
  (generic_progress_dialog_widget_get_type ())
G_DECLARE_DERIVABLE_TYPE (
  GenericProgressDialogWidget,
  generic_progress_dialog_widget,
  Z,
  GENERIC_PROGRESS_DIALOG_WIDGET,
  AdwAlertDialog)

class ProgressInfo;

/**
 * @addtogroup widgets
 *
 * @{
 */

using GenericProgressDialogButton = struct GenericProgressDialogButton
{
  char response[200];

  /**
   * Only show the button when the action is finished.
   */
  bool only_on_finish;

  /** Callback when this response is made. */
  GenericCallback cb;

  /** Callback object. */
  void * cb_obj;
};

/**
 * @brief A generic progress dialog.
 */
using GenericProgressDialogWidgetPrivate = struct
{
  GtkProgressBar * progress_bar;

  /**
   * Whether to automatically close the progress dialog when finished.
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

  ProgressInfo * progress_info;

  /**
   * Additional buttons.
   */
  GenericProgressDialogButton extra_buttons[10];
  size_t                      num_extra_buttons;

  /** Optional callback when autoclose is requested. */
  GenericCallback close_cb;

  /** Callback object. */
  void * close_cb_obj;
};

using GenericProgressDialogWidgetClass = struct _GenericProgressDialogWidgetClass
{
  AdwAlertDialogClass parent_class;
};

/**
 * Sets up a progress dialog widget.
 */
void
generic_progress_dialog_widget_setup (
  GenericProgressDialogWidget * self,
  const char *                  title,
  ProgressInfo *                progress_info,
  const char *                  initial_label,
  bool                          autoclose,
  GenericCallback               close_callback,
  void *                        close_callback_object,
  bool                          cancelable);

/**
 * Adds a response to the dialog.
 */
void
generic_progress_dialog_add_response (
  GenericProgressDialogWidget * self,
  const char *                  response,
  const char *                  response_label,
  GenericCallback               callback,
  void *                        callback_object,
  bool                          only_on_finish);

GenericProgressDialogWidget *
generic_progress_dialog_widget_new (void);

/**
 * @}
 */

#endif
