/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Bounce to audio dialog.
 */

#ifndef __GUI_WIDGETS_BOUNCE_DIALOG_H__
#define __GUI_WIDGETS_BOUNCE_DIALOG_H__

#include <gtk/gtk.h>

#define BOUNCE_DIALOG_WIDGET_TYPE \
  (bounce_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  BounceDialogWidget,
  bounce_dialog_widget,
  Z,
  BOUNCE_DIALOG_WIDGET,
  GtkDialog)

typedef struct _BounceStepSelectorWidget
  BounceStepSelectorWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Type of bounce.
 */
typedef enum BounceDialogWidgetType
{
  BOUNCE_DIALOG_REGIONS,
  BOUNCE_DIALOG_TRACKS,
} BounceDialogWidgetType;

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

  char * bounce_name;
} BounceDialogWidget;

/**
 * Creates a bounce dialog.
 */
BounceDialogWidget *
bounce_dialog_widget_new (
  BounceDialogWidgetType type,
  const char *           bounce_name);

/**
 * @}
 */

#endif
