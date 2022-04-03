/*
 * Copyright (C) 2019, 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * Event viewer.
 */

#ifndef __GUI_WIDGETS_EVENT_VIEWER_H__
#define __GUI_WIDGETS_EVENT_VIEWER_H__

#include "audio/region_identifier.h"

#include <gtk/gtk.h>

#define EVENT_VIEWER_WIDGET_TYPE \
  (event_viewer_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  EventViewerWidget,
  event_viewer_widget,
  Z,
  EVENT_VIEWER_WIDGET,
  GtkBox)

typedef struct _ArrangerWidget    ArrangerWidget;
typedef struct ArrangerSelections ArrangerSelections;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TIMELINE_EVENT_VIEWER \
  MW_MAIN_NOTEBOOK->event_viewer
#define MW_EDITOR_EVENT_VIEWER_STACK \
  MW_BOT_DOCK_EDGE->event_viewer_stack
#define MW_MIDI_EVENT_VIEWER \
  MW_BOT_DOCK_EDGE->event_viewer_midi
#define MW_CHORD_EVENT_VIEWER \
  MW_BOT_DOCK_EDGE->event_viewer_chord
#define MW_AUDIO_EVENT_VIEWER \
  MW_BOT_DOCK_EDGE->event_viewer_audio
#define MW_AUTOMATION_EVENT_VIEWER \
  MW_BOT_DOCK_EDGE->event_viewer_automation

typedef enum EventViewerType
{
  EVENT_VIEWER_TYPE_TIMELINE,
  EVENT_VIEWER_TYPE_CHORD,
  EVENT_VIEWER_TYPE_MIDI,
  EVENT_VIEWER_TYPE_AUDIO,
  EVENT_VIEWER_TYPE_AUTOMATION,
} EventViewerType;

typedef struct _EventViewerWidget
{
  GtkBox parent_instance;

  /** The tree view. */
  GtkColumnView * column_view;

  /** Array of ItemFactory pointers for each
   * column. */
  GPtrArray * item_factories;

  /** Type. */
  EventViewerType type;

  /** Used by the editor EV to check if it should
   * readd the columns. */
  RegionType region_type;

  /** Clone of last selections used. */
  ArrangerSelections * last_selections;

  /** Temporary flag. */
  bool marking_selected_objs;
} EventViewerWidget;

/**
 * Called to update the models/selections.
 *
 * @param selections_only Only update the selection
 *   status of each item without repopulating the
 *   model.
 */
void
event_viewer_widget_refresh (
  EventViewerWidget * self,
  bool                selections_only);

/**
 * Convenience function.
 */
void
event_viewer_widget_refresh_for_selections (
  ArrangerSelections * sel);

/**
 * Convenience function.
 *
 * @param selections_only Only update the selection
 *   status of each item without repopulating the
 *   model.
 */
void
event_viewer_widget_refresh_for_arranger (
  const ArrangerWidget * arranger,
  bool                   selections_only);

EventViewerWidget *
event_viewer_widget_new (void);

/**
 * Sets up the event viewer.
 */
void
event_viewer_widget_setup (
  EventViewerWidget * self,
  EventViewerType     type);

/**
 * @}
 */

#endif
