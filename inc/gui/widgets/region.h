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

typedef struct _RegionWidgetPrivate
{
  Region                   * region;   ///< the region associated with this
  //UiCursorState            cursor_state;
  //
  /** If cursor is at resizing L. */
  int                      resize_l;

  /** If cursor is at resizing R. */
  int                      resize_r;
  GtkDrawingArea *         drawing_area;
} RegionWidgetPrivate;

typedef struct _RegionWidgetClass
{
  GtkBoxClass parent_class;
} RegionWidgetClass;

void
region_widget_setup (RegionWidget * self,
                     Region *       region);

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

RegionWidgetPrivate *
region_widget_get_private (RegionWidget * self);

#endif
