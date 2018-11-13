/*
 * inc/gui/widgets/arranger.h - MIDI arranger widget
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_ARRANGER_H__
#define __GUI_WIDGETS_ARRANGER_H__

#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_editor.h"
#include "audio/position.h"

#include <gtk/gtk.h>

#define ARRANGER_WIDGET_TYPE                  (arranger_widget_get_type ())
#define ARRANGER_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARRANGER_WIDGET_TYPE, ArrangerWidget))
#define ARRANGER_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), ARRANGER_WIDGET, ArrangerWidgetClass))
#define IS_ARRANGER_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARRANGER_WIDGET_TYPE))
#define IS_ARRANGER_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), ARRANGER_WIDGET_TYPE))
#define ARRANGER_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), ARRANGER_WIDGET_TYPE, ArrangerWidgetClass))

#define MW_TIMELINE MAIN_WINDOW->timeline
#define MIDI_ARRANGER MIDI_EDITOR->midi_arranger
#define FOREACH_TL_AP for (int i = 0; i < self->num_tl_automation_points; i++)
#define FOREACH_TL_R for (int i = 0; i < self->num_tl_regions; i++)
#define FOREACH_ME_MN for (int i = 0; i < self->num_me_midi_notes; i++)

typedef struct ArrangerBgWidget ArrangerBgWidget;
typedef struct MidiNote MidiNote;
typedef struct SnapGrid SnapGrid;
typedef struct AutomationPoint AutomationPoint;

typedef enum ArrangerWidgetType
{
  ARRANGER_TYPE_TIMELINE,
  ARRANGER_TYPE_MIDI
} ArrangerWidgetType;

typedef enum ArrangerWidgetAction
{
  ARRANGER_ACTION_NONE,
  ARRANGER_ACTION_RESIZING_L,
  ARRANGER_ACTION_RESIZING_R,
  ARRANGER_ACTION_STARTING_MOVING, ///< in drag_start
  ARRANGER_ACTION_MOVING,
  ARRANGER_ACTION_STARTING_CHANGING_CURVE,
  ARRANGER_ACTION_CHANGING_CURVE,
  ARRANGER_ACTION_STARTING_SELECTION, ///< used in drag_start, useful to check If
                                    ///< nothing was clicked
  ARRANGER_ACTION_SELECTING
} ArrangerWidgetAction;

/**
 * Used internally.
 */
typedef enum ArrangerChildType
{
  ARRANGER_CHILD_TYPE_MIDI_NOTE,
  ARRANGER_CHILD_TYPE_REGION,
  ARRANGER_CHILD_TYPE_AP,
  ARRANGER_CHILD_TYPE_AC
} ArrangerChildType;

typedef struct ArrangerWidget
{
  GtkOverlay               parent_instance;
  ArrangerWidgetType       type;
  GtkDrawingArea           * bg;
  GtkGestureDrag           * drag;
  GtkGestureMultiPress     * multipress;
  GtkGestureMultiPress     * right_mouse_mp;
  double                   last_offset_x;  ///< for dragging regions, selections
  double                   last_offset_y;  ///< for selections
  ArrangerWidgetAction     action;
  Region *                 tl_regions[600];  ///< regions doing action upon (timeline)
  Region *                 tl_start_region; ///< clicked region
  Region *                 tl_top_region; ///< highest selected region
  Region *                 tl_bot_region; ///< lowest selected region
  int                      num_tl_regions;
  AutomationPoint *        tl_automation_points[600];  ///< automation points doing action upon (timeline)
  AutomationPoint *        tl_start_ap;
  int                      num_tl_automation_points;
  AutomationCurve *        tl_start_ac;
  Region                   * me_selected_region; ///< selected region for editing MIDI
  Channel                  * me_selected_channel; ///< Midi EDitor selected chan
  MidiNote *               me_midi_notes[600];  ///< MIDI notes doing action upon (midi)
  MidiNote *               me_start_midi_note; ///< original MIdi note doing action
                                              ///< upon. this is the note that was
                                              ///< clicked, even though there could
                                              ///< be more selected
  int                      num_me_midi_notes;
  double                   start_x; ///< for dragging
  double                   start_y; ///< for dragging

  double                   start_pos_px; ///< for moving regions

  /* temporary start positions, set on drag_begin, and used in drag_update
   * to move the objects accordingly */
  Position                 tl_region_start_poses[600]; ///< region initial start positions, for moving regions
  Position                 tl_ap_poses[600]; ///< for moving regions
  Position                 me_midi_note_start_poses[600]; ///< for moving regions
  Position                 start_pos; ///< useful for moving
  Position                 end_pos; ///< for moving regions
  /* end */

  int                      n_press; ///< for multipress
  SnapGrid                 * snap_grid; ///< associated snap grid
} ArrangerWidget;

typedef struct ArrangerWidgetClass
{
  GtkOverlayClass       parent_class;
} ArrangerWidgetClass;

/**
 * Creates a timeline widget using the given timeline data.
 */
ArrangerWidget *
arranger_widget_new (ArrangerWidgetType type, SnapGrid * snap_grid);

/**
 * Sets up the MIDI editor for the given region.
 */
void
arranger_widget_set_channel (ArrangerWidget * arranger, Channel * channel);

/**
 * Gets x position in pixels
 */
int
arranger_get_x_pos_in_px (Position * pos);

/**
 * Draws the selection in its background.
 *
 * Should only be called by the bg widgets when drawing.
 */
void
arranger_bg_draw_selections (ArrangerWidget * arranger,
                             cairo_t *        cr);

/**
 * Selects the region.
 *
 * NOTE for timeline.
 */
void
arranger_widget_toggle_select_region (ArrangerWidget * self,
                                      Region         * region,
                                      int              append);

void
arranger_widget_toggle_select_automation_point (ArrangerWidget * self,
                                      AutomationPoint         * ap,
                                      int              append);

void
arranger_widget_toggle_select_midi_note (ArrangerWidget * self,
                                      MidiNote         * midi_note,
                                      int              append);

void
arranger_widget_select_all (ArrangerWidget *  self,
                                   int               select);

GType arranger_widget_get_type(void);

#endif


