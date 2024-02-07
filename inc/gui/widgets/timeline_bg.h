/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
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

#    define TIMELINE_BG_WIDGET_TYPE (timeline_bg_widget_get_type ())
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
        arranger_widget_get_private (Z_ARRANGER_WIDGET (MW_TIMELINE))->bg)

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
