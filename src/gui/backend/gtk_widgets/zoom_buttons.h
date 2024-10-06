// SPDX-FileCopyrightText: © 2022-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 */

#ifndef __GUI_WIDGETS_ZOOM_BUTTONS_H__
#define __GUI_WIDGETS_ZOOM_BUTTONS_H__

#include "gui/backend/gtk_widgets/gtk_wrapper.h"

#define ZOOM_BUTTONS_WIDGET_TYPE (zoom_buttons_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ZoomButtonsWidget,
  zoom_buttons_widget,
  Z,
  ZOOM_BUTTONS_WIDGET,
  GtkBox)

/**
 * Zoom buttons for toolbars.
 */
typedef struct _ZoomButtonsWidget
{
  GtkBox      parent_instance;
  GtkButton * zoom_in;
  GtkButton * zoom_out;
  GtkButton * original_size;
  GtkButton * best_fit;
} ZoomButtonsWidget;

/**
 * @param orientation Orientation the zoom buttons will zoom
 *   in.
 */
void
zoom_buttons_widget_setup (
  ZoomButtonsWidget * self,
  bool                timeline,
  GtkOrientation      orientation);

#endif
