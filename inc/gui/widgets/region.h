/*
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/** \file */

#ifndef __GUI_WIDGETS_REGION_H__
#define __GUI_WIDGETS_REGION_H__

#include <audio/region.h>

#include <gtk/gtk.h>

#define REGION_WIDGET_TYPE (region_widget_get_type ())
G_DECLARE_DERIVABLE_TYPE (RegionWidget,
                          region_widget,
                          REGION,
                          WIDGET,
                          GtkDrawingArea)
#define IS_REGION_WIDGET(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REGION_WIDGET_TYPE))
#define REGION_WIDGET_GET_PRIVATE(self) \
  RegionWidgetPrivate * rw_prv = \
    region_widget_get_private (REGION_WIDGET (self));

typedef struct _RegionWidgetPrivate
{
  Region                   * region;   ///< the region associated with this
} RegionWidgetPrivate;

typedef struct _RegionWidgetClass
{
  GtkDrawingAreaClass parent_class;
} RegionWidgetClass;

void
region_widget_setup (RegionWidget * self,
                     Region *       region);

void
region_widget_select (RegionWidget * self,
                      int            select);

RegionWidgetPrivate *
region_widget_get_private (RegionWidget * self);

#endif
