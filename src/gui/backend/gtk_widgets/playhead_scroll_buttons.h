/*
 * SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 */

#ifndef __GUI_WIDGETS_PLAYHEAD_SCROLL_BUTTONS_H__
#define __GUI_WIDGETS_PLAYHEAD_SCROLL_BUTTONS_H__

#include "gui/backend/gtk_widgets/gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

#define PLAYHEAD_SCROLL_BUTTONS_WIDGET_TYPE \
  (playhead_scroll_buttons_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PlayheadScrollButtonsWidget,
  playhead_scroll_buttons_widget,
  Z,
  PLAYHEAD_SCROLL_BUTTONS_WIDGET,
  GtkWidget)

#define MW_PLAYHEAD_SCROLL_BUTTONS MW_HOME_TOOLBAR->snap_box

typedef struct _PlayheadScrollButtonsWidget
{
  GtkWidget         parent_instance;
  GtkToggleButton * scroll_edges;
  GtkToggleButton * follow;
} PlayheadScrollButtonsWidget;

/**
 * @}
 */

#endif
