/*
 * gui/widgets/inspector.h - A inspector widget
 *
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

#ifndef __GUI_WIDGETS_INSPECTOR_H__
#define __GUI_WIDGETS_INSPECTOR_H__

#include <gtk/gtk.h>

#define INSPECTOR_WIDGET_TYPE                  (inspector_widget_get_type ())
#define INSPECTOR_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), INSPECTOR_WIDGET_TYPE, InspectorWidget))
#define INSPECTOR_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), INSPECTOR_WIDGET, InspectorWidgetClass))
#define IS_INSPECTOR_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INSPECTOR_WIDGET_TYPE))
#define IS_INSPECTOR_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), INSPECTOR_WIDGET_TYPE))
#define INSPECTOR_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), INSPECTOR_WIDGET_TYPE, InspectorWidgetClass))

#define MW_INSPECTOR MAIN_WINDOW->inspector

typedef struct Region Region;
typedef struct InspectorRegionWidget InspectorRegionWidget;
typedef struct InspectorApWidget InspectorApWidget;
typedef struct InspectorTrackWidget InspectorTrackWidget;
typedef struct InspectorMidiWidget InspectorMidiWidget;

typedef enum InspectorWidgetChildType
{
  INSPECTOR_CHILD_REGION,
  INSPECTOR_CHILD_MIDI,
  INSPECTOR_CHILD_TRACK,
  INSPECTOR_CHILD_AP
} InspectorWidgetChildType;

typedef struct InspectorWidget
{
  GtkBox                    parent_instance;
  GtkSizeGroup *            size_group;
  GtkBox *                  top_box;
  // TODO
  //GtkBox *                  chord_track_box;
  //InspectorChordTrackWidget * chord_track;
  GtkBox *                  track_box;
  InspectorTrackWidget *    track;
  GtkBox *                  region_box;
  InspectorRegionWidget *   region;
  GtkBox *                  ap_box;
  InspectorApWidget *       ap;
  GtkBox *                  midi_box;
  InspectorMidiWidget *     midi;
  GtkBox *                  bot_box;
  GtkLabel *                no_item_label; ///< no item selected label
} InspectorWidget;

typedef struct InspectorWidgetClass
{
  GtkBoxClass       parent_class;
} InspectorWidgetClass;

/**
 * Creates the inspector widget.
 *
 * Only once per project.
 */
InspectorWidget *
inspector_widget_new ();

/**
 * Displays info about the regions.
 *
 * If num_regions < 1, it hides the regions box.
 */
void
inspector_widget_show_selections (InspectorWidgetChildType type,
                                  void **                  selections,
                                  int                      num_selections);

#endif

