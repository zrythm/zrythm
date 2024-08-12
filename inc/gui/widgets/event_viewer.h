// SPDX-FileCopyrightText: © 2019, 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Event viewer.
 */

#ifndef __GUI_WIDGETS_EVENT_VIEWER_H__
#define __GUI_WIDGETS_EVENT_VIEWER_H__

#include "dsp/region_identifier.h"
#include "gui/widgets/item_factory.h"
#include "utils/types.h"

#include "gtk_wrapper.h"

#define EVENT_VIEWER_WIDGET_TYPE (event_viewer_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  EventViewerWidget,
  event_viewer_widget,
  Z,
  EVENT_VIEWER_WIDGET,
  GtkBox)

TYPEDEF_STRUCT_UNDERSCORED (ArrangerWidget);
class ArrangerSelections;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TIMELINE_EVENT_VIEWER MW_MAIN_NOTEBOOK->event_viewer
#define MW_EDITOR_EVENT_VIEWER_STACK MW_BOT_DOCK_EDGE->event_viewer_stack
#define MW_MIDI_EVENT_VIEWER MW_BOT_DOCK_EDGE->event_viewer_midi
#define MW_CHORD_EVENT_VIEWER MW_BOT_DOCK_EDGE->event_viewer_chord
#define MW_AUDIO_EVENT_VIEWER MW_BOT_DOCK_EDGE->event_viewer_audio
#define MW_AUTOMATION_EVENT_VIEWER MW_BOT_DOCK_EDGE->event_viewer_automation

enum class EventViewerType
{
  EVENT_VIEWER_TYPE_TIMELINE,
  EVENT_VIEWER_TYPE_CHORD,
  EVENT_VIEWER_TYPE_MIDI,
  EVENT_VIEWER_TYPE_AUDIO,
  EVENT_VIEWER_TYPE_AUTOMATION,
};

using EventViewerWidget = struct _EventViewerWidget
{
  GtkBox parent_instance;

  /** The tree view. */
  GtkColumnView * column_view;

  /** Array of ItemFactory pointers for each column. */
  ItemFactoryPtrVector item_factories;

  /** Type. */
  EventViewerType type;

  /** Used by the editor EV to check if it should
   * readd the columns. */
  // RegionType region_type;

  /** Clone of last selections used. */
  std::unique_ptr<ArrangerSelections> last_selections;

  /** Temporary flag. */
  bool marking_selected_objs;
};

/**
 * Called to update the models/selections.
 *
 * @param selections_only Only update the selection
 *   status of each item without repopulating the
 *   model.
 */
void
event_viewer_widget_refresh (EventViewerWidget * self, bool selections_only);

/**
 * Convenience function.
 */
void
event_viewer_widget_refresh_for_selections (ArrangerSelections * sel);

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
event_viewer_widget_setup (EventViewerWidget * self, EventViewerType type);

/**
 * @}
 */

#endif
