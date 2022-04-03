/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#if 0

/**
 * \file
 *
 * The background (main overlay child) of the
 * TimelineArrangerWidget.
 */

#  ifndef __GUI_WIDGETS_TIMELINE_BG_H__
#    define __GUI_WIDGETS_TIMELINE_BG_H__

#    include "gui/widgets/arranger_bg.h"

#    include <gtk/gtk.h>

#    define TIMELINE_BG_WIDGET_TYPE \
      (timeline_bg_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TimelineBgWidget,
  timeline_bg_widget,
  Z, TIMELINE_BG_WIDGET,
  ArrangerBgWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

#    define TIMELINE_BG \
      Z_TIMELINE_BG_WIDGET ( \
        arranger_widget_get_private ( \
          Z_ARRANGER_WIDGET (MW_TIMELINE)) \
          ->bg)

typedef struct _TimelineBgWidget
{
  ArrangerBgWidget         parent_instance;
} TimelineBgWidget;

/**
 * Creates a TimelineBgWidget for the given
 * arranger and ruler.
 */
TimelineBgWidget *
timeline_bg_widget_new (
  RulerWidget *    ruler,
  ArrangerWidget * arranger);

/**
 * To be called by the arranger bg during drawing.
 */
void
timeline_bg_widget_draw (
  TimelineBgWidget * self,
  cairo_t *          cr,
  GdkRectangle *     rect);

/**
 * @}
 */

#  endif
#endif
