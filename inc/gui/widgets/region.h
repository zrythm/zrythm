/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/region.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#define REGION_WIDGET_TYPE \
  (region_widget_get_type ())
G_DECLARE_DERIVABLE_TYPE (RegionWidget,
                          region_widget,
                          Z,
                          REGION_WIDGET,
                          GtkBox)

#define REGION_WIDGET_GET_PRIVATE(self) \
  RegionWidgetPrivate * rw_prv = \
    region_widget_get_private (Z_REGION_WIDGET (self));

/**
* Region widget base private.
*/
typedef struct _RegionWidgetPrivate
{
  /** Region associated with this widget. */
  Region *           region;

  /** If cursor is at resizing L. */
  int                resize_l;

  /** If this widget is a TrackLane Region or main
   * Track Region. */
  int                is_lane;

  /** If cursor is at resizing R. */
  int                      resize_r;
  GtkDrawingArea *   drawing_area;
} RegionWidgetPrivate;

typedef struct _RegionWidgetClass
{
  GtkBoxClass parent_class;
} RegionWidgetClass;

/**
 * Sets up the RegionWidget.
 *
 * @param is_lane Whether this RegionWidget will be
 *   showin inside a TrackLane or not.
 */
void
region_widget_setup (
  RegionWidget * self,
  Region *       region,
  int            is_lane);

/**
 * Mark the Region as selected.
 *
 * @param with_transients Create transient Region's
 *   for the selection.
 */
void
region_widget_select (
  RegionWidget * self,
  int            select,
  int            with_transients);

/**
 * Returns if the current position is for resizing
 * L.
 */
int
region_widget_is_resize_l (
  RegionWidget * self,
  int             x);

/**
 * Returns if the current position is for resizing
 * L.
 */
int
region_widget_is_resize_r (
  RegionWidget * self,
  int             x);

/**
 * Destroys the widget completely.
 */
void
region_widget_delete (RegionWidget *self);

RegionWidgetPrivate *
region_widget_get_private (RegionWidget * self);

#endif
