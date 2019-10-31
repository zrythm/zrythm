/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Automation Point.
 */

#ifndef __GUI_WIDGETS_AUTOMATION_POINT_H__
#define __GUI_WIDGETS_AUTOMATION_POINT_H__

#include "audio/automation_point.h"
#include "gui/widgets/arranger_object.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#define AUTOMATION_POINT_WIDGET_TYPE \
  (automation_point_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  AutomationPointWidget,
  automation_point_widget,
  Z, AUTOMATION_POINT_WIDGET,
  ArrangerObjectWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define AP_WIDGET_POINT_SIZE 6
#define AP_WIDGET_CURVE_H 2
#define AP_WIDGET_CURVE_W 8
#define AP_WIDGET_PADDING 1

typedef struct _AutomationPointWidget
{
  ArrangerObjectWidget   parent_instance;

  /** The AutomationPoint associated with the
   * widget. */
  AutomationPoint *      ap;
} AutomationPointWidget;

/**
 * Creates a automation_point.
 */
AutomationPointWidget *
automation_point_widget_new (
  AutomationPoint * automation_point);

/**
 * @}
 */

#endif
