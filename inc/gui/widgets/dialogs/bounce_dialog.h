/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Bounce to audio dialog.
 */

#ifndef __GUI_WIDGETS_BOUNCE_DIALOG_H__
#define __GUI_WIDGETS_BOUNCE_DIALOG_H__

#include <gtk/gtk.h>

#define BOUNCE_DIALOG_WIDGET_TYPE (bounce_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  BounceDialogWidget,
  bounce_dialog_widget,
  Z,
  BOUNCE_DIALOG_WIDGET,
  GtkDialog)

typedef struct _BounceStepSelectorWidget BounceStepSelectorWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Type of bounce.
 */
enum class BounceDialogWidgetType
{
  BOUNCE_DIALOG_REGIONS,
  BOUNCE_DIALOG_TRACKS,
};

/**
 * The export dialog.
 */
typedef struct _BounceDialogWidget
{
  GtkDialog                  parent_instance;
  GtkButton *                cancel_btn;
  GtkButton *                bounce_btn;
  GtkCheckButton *           bounce_with_parents;
  GtkCheckButton *           disable_after_bounce;
  GtkBox *                   bounce_step_box;
  BounceStepSelectorWidget * bounce_step_selector;
  GtkSpinButton *            tail_spin;

  BounceDialogWidgetType type;

  /** Whether to bounce to file instead of on a new
   * audio track (TODO, or make it a gsetting). */
  int bounce_to_file;

  Position start_pos;

  char * bounce_name;
} BounceDialogWidget;

/**
 * Creates a bounce dialog.
 */
BounceDialogWidget *
bounce_dialog_widget_new (BounceDialogWidgetType type, const char * bounce_name);

/**
 * @}
 */

#endif
