/*
 * gui/widgets/inspector.h - A inspector widget
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#define INSPECTOR_WIDGET_TYPE \
  (inspector_widget_get_type ())
G_DECLARE_FINAL_TYPE (InspectorWidget,
                      inspector_widget,
                      Z,
                      INSPECTOR_WIDGET,
                      GtkBox)

#define MW_INSPECTOR MW_LEFT_DOCK_EDGE->inspector

typedef struct Region Region;
typedef struct _InspectorRegionWidget InspectorRegionWidget;
typedef struct _InspectorApWidget InspectorApWidget;
typedef struct _InspectorTrackWidget InspectorTrackWidget;
typedef struct _InspectorMidiWidget InspectorMidiWidget;
typedef struct _InspectorChordWidget InspectorChordWidget;

typedef enum InspectorWidgetChildType
{
  INSPECTOR_CHILD_REGION,
  INSPECTOR_CHILD_MIDI,
  INSPECTOR_CHILD_TRACK,
  INSPECTOR_CHILD_AP,
  INSPECTOR_CHILD_CHORD,
} InspectorWidgetChildType;

typedef struct _InspectorWidget
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
  InspectorChordWidget *    chord;
  GtkBox *                  chord_box;
  GtkLabel *                no_item_label; ///< no item selected label
} InspectorWidget;

/**
 * Creates the inspector widget.
 *
 * Only once per project.
 */
InspectorWidget *
inspector_widget_new ();

/**
 * Refreshes the inspector widget (shows current
 * selections.
 */
void
inspector_widget_refresh ();

/**
 * Displays info about the regions.
 *
 * If num_regions < 1, it hides the regions box.
 */
void
inspector_widget_show_selections (
  InspectorWidgetChildType type,
  void **                  selections,
  int                      num_selections);

#endif

