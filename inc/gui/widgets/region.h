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

#define REGION_WIDGET_TYPE                  (region_widget_get_type ())
#define REGION_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), REGION_WIDGET_TYPE, RegionWidget))
#define REGION_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), REGION_WIDGET, RegionWidgetClass))
#define IS_REGION_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REGION_WIDGET_TYPE))
#define IS_REGION_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), REGION_WIDGET_TYPE))
#define REGION_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), REGION_WIDGET_TYPE, RegionWidgetClass))

typedef enum RegionWidgetState
{
  RW_STATE_NONE,
  RW_STATE_SELECTED,
  RW_STATE_RESIZE_L,
  RW_STATE_RESIZE_R,
  RW_STATE_HOVER
} RegionWidgetState;

typedef struct RegionWidget
{
  GtkDrawingArea           parent_instance;
  Region                   * region;   ///< the region associated with this
  RegionWidgetState        state;
} RegionWidget;

typedef struct RegionWidgetClass
{
  GtkDrawingAreaClass       parent_class;
} RegionWidgetClass;

/**
 * Creates a region.
 */
RegionWidget *
region_widget_new (Region * region);

/**
 * Sets hover state and queues draw.
 */
void
region_widget_set_state_and_queue_draw (RegionWidget *    self,
                                        RegionWidgetState state);

GType region_widget_get_type(void);

#endif

