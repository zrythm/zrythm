/*
 * Copyright (C) 2018-2019, 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Bot part of timeline panel (with timeline minimap).
 */

#ifndef __GUI_WIDGETS_TIMELINE_BOT_BOX_H__
#define __GUI_WIDGETS_TIMELINE_BOT_BOX_H__

#include <gtk/gtk.h>

#define TIMELINE_BOT_BOX_WIDGET_TYPE \
  (timeline_bot_box_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TimelineBotBoxWidget,
  timeline_bot_box_widget,
  Z,
  TIMELINE_BOT_BOX_WIDGET,
  GtkBox)

typedef struct _TimelineMinimapWidget
  TimelineMinimapWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TIMELINE_BOT_BOX \
  MW_TIMELINE_PANEL->bot_box

typedef struct _TimelineBotBoxWidget
{
  GtkBox                  parent_instance;
  GtkBox *                left_tb;
  GtkButton *             instrument_add;
  TimelineMinimapWidget * timeline_minimap;
} TimelineBotBoxWidget;

/**
 * @}
 */

#endif
