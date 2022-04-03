/*
 * Copyright (C) 2019, 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_TOOLBOX_H__
#define __GUI_WIDGETS_TOOLBOX_H__

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define TOOLBOX_WIDGET_TYPE \
  (toolbox_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ToolboxWidget,
  toolbox_widget,
  Z,
  TOOLBOX_WIDGET,
  GtkBox)

#define MW_TOOLBOX MW_HOME_TOOLBAR->toolbox

typedef struct _ToolboxWidget
{
  GtkBox            parent_instance;
  GtkToggleButton * select_mode;
  GtkToggleButton * edit_mode;
  GtkToggleButton * cut_mode;
  GtkToggleButton * erase_mode;
  GtkToggleButton * ramp_mode;
  GtkToggleButton * audition_mode;
  GtkImage *        select_img;

  gulong select_handler_id;
  gulong edit_handler_id;
  gulong cut_handler_id;
  gulong erase_handler_id;
  gulong ramp_handler_id;
  gulong audition_handler_id;
} ToolboxWidget;

/**
 * Sets the toolbox toggled states after deactivating
 * the callbacks.
 */
void
toolbox_widget_refresh (ToolboxWidget * self);

/**
 * @}
 */

#endif
