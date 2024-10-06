// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Bot dock.
 */

#ifndef __GUI_WIDGETS_BOT_DOCK_EDGE_H__
#define __GUI_WIDGETS_BOT_DOCK_EDGE_H__

#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/libpanel_wrapper.h"

#define BOT_DOCK_EDGE_WIDGET_TYPE (bot_dock_edge_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  BotDockEdgeWidget,
  bot_dock_edge_widget,
  Z,
  BOT_DOCK_EDGE_WIDGET,
  GtkWidget)

#define MW_BOT_DOCK_EDGE MW_CENTER_DOCK->bot_dock_edge

typedef struct _MixerWidget            MixerWidget;
typedef struct _ClipEditorWidget       ClipEditorWidget;
typedef struct _ModulatorViewWidget    ModulatorViewWidget;
typedef struct _FoldableNotebookWidget FoldableNotebookWidget;
typedef struct _EventViewerWidget      EventViewerWidget;
typedef struct _ChordPadPanelWidget    ChordPadPanelWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Bot dock widget.
 */
typedef struct _BotDockEdgeWidget
{
  GtkWidget    parent_instance;
  PanelFrame * panel_frame;

  /** Wrapper. */
  GtkBox *              modulator_view_box;
  ModulatorViewWidget * modulator_view;

  GtkPaned * clip_editor_plus_event_viewer_paned;

  /** Event viewers. */
  GtkStack *          event_viewer_stack;
  EventViewerWidget * event_viewer_midi;
  EventViewerWidget * event_viewer_chord;
  EventViewerWidget * event_viewer_automation;
  EventViewerWidget * event_viewer_audio;

  /** Wrapper. */
  GtkBox *           clip_editor_box;
  ClipEditorWidget * clip_editor;

  /** Wrapper. */
  GtkBox *      mixer_box;
  MixerWidget * mixer;

  /** Chord pads. */
  GtkBox *              chord_pad_panel_box;
  ChordPadPanelWidget * chord_pad_panel;

  GtkButton * toggle_top_panel;
} BotDockEdgeWidget;

void
bot_dock_edge_widget_setup (BotDockEdgeWidget * self);

/**
 * Sets the appropriate stack page.
 */
void
bot_dock_edge_widget_update_event_viewer_stack_page (BotDockEdgeWidget * self);

/**
 * Brings up the clip editor.
 *
 * @param navigate_to_region_start Whether to adjust
 *   the view to start at the region start.
 */
void
bot_dock_edge_widget_show_clip_editor (
  BotDockEdgeWidget * self,
  bool                navigate_to_region_start);

/**
 * @}
 */

#endif
