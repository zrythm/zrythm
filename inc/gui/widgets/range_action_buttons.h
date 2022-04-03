/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 */

#ifndef __GUI_WIDGETS_RANGE_ACTION_BUTTONS_H__
#define __GUI_WIDGETS_RANGE_ACTION_BUTTONS_H__

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define RANGE_ACTION_BUTTONS_WIDGET_TYPE \
  (range_action_buttons_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  RangeActionButtonsWidget,
  range_action_buttons_widget,
  Z,
  RANGE_ACTION_BUTTONS_WIDGET,
  GtkBox)

typedef struct _SnapGridWidget SnapGridWidget;
typedef struct SnapGrid        SnapGrid;

typedef struct _RangeActionButtonsWidget
{
  GtkBox      parent_instance;
  GtkButton * insert_silence;
  GtkButton * remove;
} RangeActionButtonsWidget;

/**
 * @}
 */

#endif
