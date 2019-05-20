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

#include "actions/create_midi_arranger_selections_action.h"
#include "actions/edit_midi_arranger_selections_action.h"
#include "actions/duplicate_midi_arranger_selections_action.h"
#include "actions/move_midi_arranger_selections_action.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/midi_region.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "audio/velocity.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_curve.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_lane.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/piano_roll_labels.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/gtk.h"
#include "utils/flags.h"
#include "utils/ui.h"
#include "utils/resources.h"
#include "settings/settings.h"
#include "zrythm.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MidiArrangerWidget,
               midi_arranger_widget,
               ARRANGER_WIDGET_TYPE)

/**
 * To be called from get_child_position in parent widget.
 *
 * Used to allocate the overlay children.
 */
void
midi_arranger_widget_set_allocation (
  MidiArrangerWidget * self,
  GtkWidget *          widget,
  GdkRectangle *       allocation)
{
  if (Z_IS_MIDI_NOTE_WIDGET (widget))
    {
      MidiNoteWidget * midi_note_widget =
        Z_MIDI_NOTE_WIDGET (widget);
      MidiNote * mn = midi_note_widget->midi_note;
      long region_start_ticks =
        mn->region->start_pos.total_ticks;
      Position tmp;

      /* use absolute position */
      position_from_ticks (
        &tmp,
        region_start_ticks +
        mn->start_pos.total_ticks);
      allocation->x =
        ui_pos_to_px_piano_roll (
          &tmp, 1);
      allocation->y =
        MW_PIANO_ROLL->piano_roll_labels->
          px_per_note *
        (127 - mn->val);

      /* use absolute position */
      position_from_ticks (
        &tmp,
        region_start_ticks +
        mn->end_pos.total_ticks);
      allocation->width =
        ui_pos_to_px_piano_roll (
          &tmp, 1) - allocation->x;
      allocation->height =
        MW_PIANO_ROLL->piano_roll_labels->px_per_note;
    }
}

int
midi_arranger_widget_get_note_at_y (double y)
{
  return 128 - y / PIANO_ROLL_LABELS->px_per_note;
}

MidiNoteWidget *
midi_arranger_widget_get_hit_midi_note (
  MidiArrangerWidget *  self,
  double                x,
  double                y)
{
  GtkWidget * widget =
    ui_get_hit_child (
      GTK_CONTAINER (self),
      x,
      y,
      MIDI_NOTE_WIDGET_TYPE);
  if (widget)
    {
      MidiNoteWidget * mn_w =
        Z_MIDI_NOTE_WIDGET (widget);
      self->start_midi_note = mn_w->midi_note;
      return mn_w;
    }
  return NULL;
}

void
midi_arranger_widget_select_all (
  MidiArrangerWidget *  self,
  int               select)
{
  if (!CLIP_EDITOR->region)
    return;

  /*midi_arranger_selections_clear (*/
    /*MA_SELECTIONS);*/

  /* select midi notes */
  MidiRegion * mr =
    (MidiRegion *) CLIP_EDITOR_SELECTED_REGION;
  for (int i = 0; i < mr->num_midi_notes; i++)
    {
      MidiNote * midi_note = mr->midi_notes[i];
      midi_note_widget_select (
        midi_note->widget, select,
        F_NO_TRANSIENTS);
    }
  /*arranger_widget_refresh(&self->parent_instance);*/
}

static int
on_motion (GtkWidget *      widget,
           GdkEventMotion * event,
           RegionWidget *   self)
{
  if (event->type == GDK_LEAVE_NOTIFY)
    MIDI_ARRANGER->hovered_note = -1;
  else
    MIDI_ARRANGER->hovered_note =
      midi_arranger_widget_get_note_at_y (
        event->y - 2); // for line boundaries

  ARRANGER_WIDGET_GET_PRIVATE (MIDI_ARRANGER);
  gtk_widget_queue_draw (
              GTK_WIDGET (ar_prv->bg));

  return FALSE;
}

void
midi_arranger_widget_setup (
  MidiArrangerWidget * self)
{
  // set the size
  int ww, hh;
  gtk_widget_get_size_request (
    GTK_WIDGET (PIANO_ROLL_LABELS),
    &ww,
    &hh);
  RULER_WIDGET_GET_PRIVATE (MIDI_RULER);
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    rw_prv->total_px,
    hh);

  ARRANGER_WIDGET_GET_PRIVATE (self);
  g_signal_connect (
    G_OBJECT(ar_prv->bg),
    "motion-notify-event",
    G_CALLBACK (on_motion),  self);
}

/**
 * Fills in the positions that the MidiArranger
 * remembers at the start of each drag.
 */
void
midi_arranger_widget_set_init_poses (
  MidiArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes;
       i++)
    {
      MidiNote * r =
        MA_SELECTIONS->midi_notes[i];
      if (position_compare (&r->start_pos,
                            &ar_prv->start_pos) <= 0)
        {
          position_set_to_pos (&ar_prv->start_pos,
                               &r->start_pos);
        }

      /* set start poses for midi_notes */
      position_set_to_pos (
        &self->midi_note_start_poses[i],
        &r->start_pos);

      /* set end poses for midi_notes */
      position_set_to_pos (
        &self->midi_note_end_poses[i],
        &r->end_pos);
    }
}

/**
 * Returns the appropriate cursor based on the
 * current hover_x and y.
 */
ArrangerCursor
midi_arranger_widget_get_cursor (
  MidiArrangerWidget * self,
  UiOverlayAction action,
  Tool            tool)
{
  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      if (P_TOOL == TOOL_SELECT_NORMAL ||
           P_TOOL == TOOL_SELECT_STRETCH ||
           P_TOOL == TOOL_EDIT)
        {
          MidiNoteWidget * mnw =
            midi_arranger_widget_get_hit_midi_note (
              self,
              ar_prv->hover_x,
              ar_prv->hover_y);

          int is_hit =
            mnw != NULL;
          int is_resize_l =
            mnw && mnw->resize_l;
          int is_resize_r =
            mnw && mnw->resize_r;

          if (is_hit && is_resize_l)
            {
              return ARRANGER_CURSOR_RESIZING_L;
            }
          else if (is_hit && is_resize_r)
            {
              return ARRANGER_CURSOR_RESIZING_R;
            }
          else if (is_hit)
            {
              return ARRANGER_CURSOR_GRAB;
            }
          else
            {
              /* set cursor to whatever it is */
              if (tool == TOOL_EDIT)
                return ARRANGER_CURSOR_EDIT;
              else
                return ARRANGER_CURSOR_SELECT;
            }
        }
      else if (P_TOOL == TOOL_EDIT)
        ac = ARRANGER_CURSOR_EDIT;
      else if (P_TOOL == TOOL_ERASER)
        ac = ARRANGER_CURSOR_ERASER;
      else if (P_TOOL == TOOL_RAMP)
        ac = ARRANGER_CURSOR_RAMP;
      else if (P_TOOL == TOOL_AUDITION)
        ac = ARRANGER_CURSOR_AUDITION;
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_ERASING:
      ac = ARRANGER_CURSOR_ERASER;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_COPY:
      ac = ARRANGER_CURSOR_GRABBING_COPY;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    case UI_OVERLAY_ACTION_MOVING:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      ac = ARRANGER_CURSOR_GRABBING_LINK;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      ac = ARRANGER_CURSOR_RESIZING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
      ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
    case UI_OVERLAY_ACTION_SELECTING:
      ac = ARRANGER_CURSOR_SELECT;
      /* TODO depends on tool */
      break;
    default:
      g_warn_if_reached ();
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
}

/**
 * Shows context menu.
 *
 * To be called from parent on right click.
 */
void
midi_arranger_widget_show_context_menu (
  MidiArrangerWidget * self,
  gdouble              x,
  gdouble              y)
{
  GtkWidget *menu;
  GtkMenuItem * menu_item;

  MidiNoteWidget * clicked_note =
    midi_arranger_widget_get_hit_midi_note (
      self, x, y);

  if (clicked_note)
    {
      int selected =
        midi_note_is_selected (
          clicked_note->midi_note);
      if (!selected)
        ARRANGER_WIDGET_SELECT_MIDI_NOTE (
          self, clicked_note->midi_note,
          F_SELECT, F_NO_APPEND,
          F_NO_TRANSIENTS);
    }
  else
    {
      midi_arranger_widget_select_all (
        self, F_NO_SELECT);
      midi_arranger_selections_clear (
        MA_SELECTIONS);
    }

#define APPEND_TO_MENU \
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), \
                         GTK_WIDGET (menu_item))

  menu = gtk_menu_new();

  menu_item = CREATE_CUT_MENU_ITEM;
  APPEND_TO_MENU;
  menu_item = CREATE_COPY_MENU_ITEM;
  APPEND_TO_MENU;
  menu_item = CREATE_PASTE_MENU_ITEM;
  APPEND_TO_MENU;
  menu_item = CREATE_DELETE_MENU_ITEM;
  APPEND_TO_MENU;
  menu_item = CREATE_DUPLICATE_MENU_ITEM;
  APPEND_TO_MENU;
  menu_item =
    GTK_MENU_ITEM (gtk_separator_menu_item_new ());
  APPEND_TO_MENU;
  menu_item = CREATE_CLEAR_SELECTION_MENU_ITEM;
  APPEND_TO_MENU;
  menu_item = CREATE_SELECT_ALL_MENU_ITEM;
  APPEND_TO_MENU;

#undef APPEND_TO_MENU

  gtk_widget_show_all(menu);

  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);
}

/**
 * Sets transient notes and actual notes
 * visibility based on the current action.
 */
void
midi_arranger_widget_update_visibility (
  MidiArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  MidiNote * mn, * transient;
  ARRANGER_SET_SELECTION_VISIBILITY (
    MA_SELECTIONS->midi_notes,
    MA_SELECTIONS->transient_notes,
    MA_SELECTIONS->num_midi_notes,
    mn,
    transient);
}

void
midi_arranger_widget_on_drag_begin_note_hit (
  MidiArrangerWidget * self,
  double               start_x,
  MidiNoteWidget *     mnw)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* get x as local to the midi note */
  gint wx, wy;
  gtk_widget_translate_coordinates(
            GTK_WIDGET (self),
            GTK_WIDGET (mnw),
            start_x,
            0,
            &wx,
            &wy);

  MidiNote * mn = mnw->midi_note;
  self->start_midi_note = mn;
  self->start_midi_note_clone =
    midi_note_clone (mn);

  /* update arranger action */
  switch (P_TOOL)
    {
    case TOOL_ERASER:
      ar_prv->action =
        UI_OVERLAY_ACTION_ERASING;
      break;
    case TOOL_AUDITION:
      ar_prv->action =
        UI_OVERLAY_ACTION_AUDITIONING;
      break;
    case TOOL_SELECT_NORMAL:
      if (midi_note_widget_is_resize_l (mnw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_RESIZING_L;
      else if (midi_note_widget_is_resize_r (mnw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_RESIZING_R;
      else
        ar_prv->action =
          UI_OVERLAY_ACTION_STARTING_MOVING;
      break;
    case TOOL_SELECT_STRETCH:
      if (midi_note_widget_is_resize_l (mnw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_STRETCHING_L;
      else if (midi_note_widget_is_resize_r (mnw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_STRETCHING_R;
      else
        ar_prv->action =
          UI_OVERLAY_ACTION_STARTING_MOVING;
      break;
    case TOOL_EDIT:
      if (midi_note_widget_is_resize_l (mnw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_RESIZING_L;
      else if (midi_note_widget_is_resize_r (mnw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_RESIZING_R;
      else
        ar_prv->action =
          UI_OVERLAY_ACTION_STARTING_MOVING;
      break;
    case TOOL_RAMP:
      break;
    }

  int selected = midi_note_is_selected (mn);

  /* select midi note if unselected */
  if (P_TOOL == TOOL_EDIT ||
      P_TOOL == TOOL_SELECT_NORMAL ||
      P_TOOL == TOOL_SELECT_STRETCH)
    {
      int transients =
        arranger_widget_is_in_moving_operation (
          Z_ARRANGER_WIDGET (self)) ||
        ar_prv->action ==
          UI_OVERLAY_ACTION_RESIZING_R ||
        ar_prv->action ==
          UI_OVERLAY_ACTION_RESIZING_L;
      g_message ("transients %d", transients);

      /* if ctrl held & not selected, add to
       * selections */
      if (ar_prv->ctrl_held &&
          !selected)
        {
          ARRANGER_WIDGET_SELECT_MIDI_NOTE (
            self, mn, F_SELECT, F_APPEND,
            transients);
        }
      /* if ctrl not held & not selected, make it
       * the only
       * selection */
      else if (!ar_prv->ctrl_held &&
               !selected)
        {
          ARRANGER_WIDGET_SELECT_MIDI_NOTE (
            self, mn, F_SELECT, F_NO_APPEND,
            transients);
        }
      /* if selected, create transients */
      else if (selected)
        {
          midi_arranger_selections_create_missing_transients (
            MA_SELECTIONS);
          g_warn_if_fail (
            MA_SELECTIONS->
              transient_notes[0] &&
            GTK_IS_WIDGET (
              MA_SELECTIONS->
                transient_notes[0]->widget));
        }
    }
}

/**
 * Called on drag begin in parent when background is double
 * clicked (i.e., a note is created).
 * @param pos The absolute position in the piano
 *   roll.
 */
void
midi_arranger_widget_create_note (
  MidiArrangerWidget * self,
  Position * pos,
  int                  note,
  MidiRegion * region)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid) &&
      !ar_prv->shift_held)
    {
      position_snap (NULL,
                     pos,
                     NULL,
                     NULL,
                     ar_prv->snap_grid);
    }
  Velocity * vel = velocity_default ();

  /* get local pos */
  Position local_pos;
  position_from_ticks (
    &local_pos,
    pos->total_ticks -
    region->start_pos.total_ticks);

  MidiNote * midi_note =
    midi_note_new (
      region, &local_pos, &local_pos, note, vel);
  position_set_min_size (&midi_note->start_pos,
                         &midi_note->end_pos,
                         ar_prv->snap_grid);
  midi_region_add_midi_note (region,
                        midi_note);
  ar_prv->action =
    UI_OVERLAY_ACTION_CREATING_RESIZING_R;
  position_set_to_pos (
    &self->midi_note_end_poses[0],
    &midi_note->end_pos);
  ARRANGER_WIDGET_SELECT_MIDI_NOTE (
    self, midi_note, F_SELECT, F_NO_APPEND,
    F_TRANSIENTS);
}

/**
 * Called when in selection mode.
 *
 * Called by arranger widget during drag_update to find and
 * select the midi notes enclosed in the selection area.
 *
 * @param[in] delete If this is a select-delete
 *   operation
 */
void
midi_arranger_widget_select (
  MidiArrangerWidget * self,
  double               offset_x,
  double               offset_y,
  int                  delete)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (!delete)
    /* deselect all */
    arranger_widget_select_all (
      Z_ARRANGER_WIDGET (self), 0);

  /* find enclosed midi notes */
  GtkWidget *    midi_note_widgets[800];
  int            num_midi_note_widgets = 0;
  arranger_widget_get_hit_widgets_in_range (
    Z_ARRANGER_WIDGET (self),
    MIDI_NOTE_WIDGET_TYPE,
    ar_prv->start_x,
    ar_prv->start_y,
    offset_x,
    offset_y,
    midi_note_widgets,
    &num_midi_note_widgets);

  MidiNoteWidget * midi_note_widget;
  MidiNote * midi_note;
  if (delete)
    {
      /* delete the enclosed midi notes */
      for (int i = 0; i < num_midi_note_widgets; i++)
        {
          midi_note_widget =
            Z_MIDI_NOTE_WIDGET (midi_note_widgets[i]);
          midi_note =
            midi_note_widget->midi_note;

          midi_region_remove_midi_note (
            midi_note->region,
            midi_note,
            F_NO_FREE,
            F_PUBLISH_EVENTS);
        }
    }
  else
    {
      /* select the enclosed midi_notes */
      for (int i = 0; i < num_midi_note_widgets; i++)
        {
          midi_note_widget =
            Z_MIDI_NOTE_WIDGET (
              midi_note_widgets[i]);
          midi_note =
            midi_note_widget->midi_note;
          ARRANGER_WIDGET_SELECT_MIDI_NOTE (
            self,
            midi_note,
            F_SELECT,
            F_APPEND,
            F_NO_TRANSIENTS);
        }
    }
}

/**
 * Called during drag_update in parent when resizing the
 * selection. It sets the start pos of the selected MIDI
 * notes.
 */
void
midi_arranger_widget_snap_midi_notes_l (
  MidiArrangerWidget *self,
  Position *          pos)
{
  int i;
  MidiNote * mn;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* snap */
  if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid) &&
        !ar_prv->shift_held)
    position_snap (NULL,
                   pos,
                   NULL,
                   CLIP_EDITOR->region,
                   ar_prv->snap_grid);

  /* get local pos */
  Position local_pos;
  position_from_ticks (
    &local_pos,
    pos->total_ticks -
    self->start_midi_note->
      region->start_pos.total_ticks);

  /* get delta with first clicked note's start
   * pos */
  long delta;
  g_warn_if_fail (self->start_midi_note_clone);
  delta =
    local_pos.total_ticks -
    self->start_midi_note_clone->
      start_pos.total_ticks;

  /* new start pos for each midi note, calculated by
   * adding delta to the midi note's original start
   * pos */
  Position new_start_pos;

#define CALC_NEW_START_POS \
  position_set_to_pos ( \
    &new_start_pos, \
    &self->midi_note_start_poses[i]); \
  position_add_ticks ( \
    &new_start_pos, delta);

  /* transient midi notes */
  for (i = 0;
       i < MA_SELECTIONS->num_midi_notes;
       i++)
    {
      mn =
        MA_SELECTIONS->
          transient_notes[i];
      if (mn)
        {
          CALC_NEW_START_POS;

          midi_note_set_start_pos (
            mn, &new_start_pos);
        }

    }

#undef CALC_NEW_START_POS
}

/**
 * Called during drag_update in parent when resizing
 * the selection. It sets the end pos of the
 * selected MIDI notes.
 */
void
midi_arranger_widget_snap_midi_notes_r (
  MidiArrangerWidget *self,
  Position *          pos)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid) &&
        !ar_prv->shift_held)
    position_snap (NULL,
                   pos,
                   NULL,
                   CLIP_EDITOR->region,
                   ar_prv->snap_grid);


  /* get local pos */
  Position local_pos;
  position_from_ticks (
    &local_pos,
    pos->total_ticks -
    CLIP_EDITOR->region->start_pos.total_ticks);

  /* get delta with first clicked region's end
   * pos */
  long delta;
  if (ar_prv->action ==
        UI_OVERLAY_ACTION_CREATING_RESIZING_R)
    {
      delta =
        local_pos.total_ticks -
        self->midi_note_end_poses[0].total_ticks;
    }
  else
    {
      g_warn_if_fail (self->start_midi_note_clone);
      delta =
        local_pos.total_ticks -
        self->start_midi_note_clone->
          end_pos.total_ticks;
    }

  /* new end pos for each MidiNote, calculated by
   * adding delta to the MidiNote's original end
   * pos */
  Position new_end_pos;

#define CALC_NEW_END_POS \
  position_set_to_pos ( \
    &new_end_pos, \
    &self->midi_note_end_poses[i]); \
  position_add_ticks ( \
    &new_end_pos, delta);

  /* actual midi notes */
  if (ar_prv->action ==
        UI_OVERLAY_ACTION_CREATING_RESIZING_R)
    {
      for (int i = 0;
           i < MA_SELECTIONS->num_midi_notes;
           i++)
        {
          MidiNote * mn =
            MA_SELECTIONS->midi_notes[i];

          CALC_NEW_END_POS;

          midi_note_set_end_pos (mn, &new_end_pos);
        }
    }

  /* transients */
  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes;
       i++)
    {
      MidiNote * mn =
        MA_SELECTIONS->transient_notes[i];

      if (mn)
        {
          CALC_NEW_END_POS;

          midi_note_set_end_pos (mn, &new_end_pos);
        }
    }
}

/**
 * Called when moving midi notes in drag update in arranger
 * widget.
 */
void
midi_arranger_widget_move_items_x (
  MidiArrangerWidget * self,
  long                 ticks_diff)
{
  /* update region positions */
  MidiNote * mn;
  Position tmp;
  long length_ticks;
  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes;
       i++)
    {
      mn =
        MA_SELECTIONS->
          transient_notes[i];

      ARRANGER_MOVE_OBJ_BY_TICKS_W_LENGTH (
        mn, midi_note,
        &self->midi_note_start_poses[i],
        ticks_diff, &tmp, length_ticks);

      if (mn->widget)
        midi_note_widget_update_tooltip (
          mn->widget, 1);
    }
}


/**
 * Called on move items_y setup.
 *
 * calculates the max possible y movement
 */
static int
calc_deltamax_for_note_movement (int y_delta)
{
  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes;
       i++)
    {
      MidiNote * midi_note =
      MA_SELECTIONS->transient_notes[i];
      /*g_message ("midi note val %d, y delta %d",*/
                 /*midi_note->val, y_delta);*/
      if (midi_note->val + y_delta < 0)
        {
          y_delta = 0;
        }
      else if (midi_note->val + y_delta >= 127)
        {
          y_delta = 127 - midi_note->val;
        }
    }
  /*g_message ("y delta %d", y_delta);*/
  return y_delta;
  /*return y_delta < 0 ? -1 : 1;*/
}
/**
 * Called when moving midi notes in drag update in arranger
 * widget for moving up/down (changing note).
 */
void
midi_arranger_widget_move_items_y (
  MidiArrangerWidget *self,
  double              offset_y)
{
  ARRANGER_WIDGET_GET_PRIVATE(self);

  int y_delta;
  int ar_start_val =
    MA_SELECTIONS->
      transient_notes[0]->val;
  int ar_end_val =
    midi_arranger_widget_get_note_at_y (
      ar_prv->start_y + offset_y);

  y_delta = ar_end_val - ar_start_val;
  y_delta =
    calc_deltamax_for_note_movement (y_delta);
  if (ar_end_val != ar_start_val)
    {
      for (int i = 0;
           i < MA_SELECTIONS->
                 num_midi_notes;
           i++)
        {
          MidiNote * midi_note =
            MA_SELECTIONS->
              transient_notes[i];
          midi_note->val = midi_note->val + y_delta;
          if (midi_note->widget)
            midi_note_widget_update_tooltip (
              midi_note->widget, 0);

        }
    }
}


/**
 * Called on drag end.
 *
 * Sets default cursors back and sets the start midi note
 * to NULL if necessary.
 */
void
midi_arranger_widget_on_drag_end (
  MidiArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  MidiNote * midi_note;
  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes;
       i++)
    {
      midi_note =
        MA_SELECTIONS->midi_notes[i];

      if (midi_note->widget)
        midi_note_widget_update_tooltip (
          midi_note->widget, 0);

      EVENTS_PUSH (ET_MIDI_NOTE_CHANGED,
                   midi_note);
    }

  if (ar_prv->action ==
        UI_OVERLAY_ACTION_RESIZING_L)
    {
      UndoableAction * ua =
        (UndoableAction *)
        edit_midi_arranger_selections_action_new (
          MA_SELECTIONS,
          EMAS_TYPE_RESIZE_L,
          MA_SELECTIONS->
            transient_notes[0]->
              start_pos.total_ticks -
          MA_SELECTIONS->
            midi_notes[0]->start_pos.total_ticks);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
  else if (ar_prv->action ==
        UI_OVERLAY_ACTION_RESIZING_R)
    {
      UndoableAction * ua =
        (UndoableAction *)
        edit_midi_arranger_selections_action_new (
          MA_SELECTIONS,
          EMAS_TYPE_RESIZE_R,
          MA_SELECTIONS->
            transient_notes[0]->end_pos.total_ticks -
          MA_SELECTIONS->
            midi_notes[0]->end_pos.total_ticks);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
  else if (ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      /* if something was clicked with ctrl without
       * moving*/
      if (ar_prv->ctrl_held)
        if (self->start_midi_note_clone &&
            midi_note_is_selected (
              self->start_midi_note_clone))
          {
            /* deselect it */
            ARRANGER_WIDGET_SELECT_MIDI_NOTE (
              self, self->start_midi_note,
              F_NO_SELECT, F_APPEND,
              F_NO_TRANSIENTS);
          }
    }
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING)
    {
      UndoableAction * ua =
        move_midi_arranger_selections_action_new (
          MA_SELECTIONS,
          MA_SELECTIONS->
            transient_notes[0]->start_pos.
              total_ticks -
          MA_SELECTIONS->
            midi_notes[0]->start_pos.total_ticks,
          MA_SELECTIONS->
            transient_notes[0]->val -
          MA_SELECTIONS->
            midi_notes[0]->val);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
  /* if copy/link-moved */
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING_COPY ||
           ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING_LINK)
    {
      UndoableAction * ua =
        (UndoableAction *)
        duplicate_midi_arranger_selections_action_new (
          MA_SELECTIONS,
          MA_SELECTIONS->
            transient_notes[0]->start_pos.
            total_ticks -
          MA_SELECTIONS->
            midi_notes[0]->start_pos.total_ticks,
          MA_SELECTIONS->
            transient_notes[0]->val -
          MA_SELECTIONS->
            midi_notes[0]->val);
      midi_arranger_selections_clear (
        MA_SELECTIONS);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_NONE)
    {
      midi_arranger_selections_clear (
        MA_SELECTIONS);
    }
  /* if something was created */
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_CREATING_RESIZING_R)
    {
      g_message ("ATTEMPTING TO CREATE");
      UndoableAction * ua =
        (UndoableAction *)
        create_midi_arranger_selections_action_new (
          MA_SELECTIONS);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
  /* if didn't click on something */
  else
    {
      self->start_midi_note = NULL;
    }
  midi_arranger_selections_remove_transients (
    MA_SELECTIONS);
  ar_prv->action = UI_OVERLAY_ACTION_NONE;
  midi_arranger_widget_update_visibility (
    self);

  /* FIXME temporary, should be removed */
  /*for (int i = 0;*/
       /*i < CLIP_EDITOR->region->num_midi_notes;*/
       /*i++)*/
    /*{*/
      /*midi_note =*/
        /*CLIP_EDITOR->region->midi_notes[i];*/

      /*if (!midi_note_is_selected (midi_note))*/
        /*EVENTS_PUSH (*/
          /*ET_MIDI_NOTE_CHANGED,*/
          /*CLIP_EDITOR->region->midi_notes[i]);*/
    /*}*/

  if (self->start_midi_note_clone)
    {
      midi_note_free (self->start_midi_note_clone);
      self->start_midi_note_clone = NULL;
    }

  EVENTS_PUSH (ET_MA_SELECTIONS_CHANGED,
               NULL);
}

/**
 * Readd children.
 */
void
midi_arranger_widget_refresh_children (
  MidiArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* remove all children except bg */
  GList *children, *iter;

  children =
    gtk_container_get_children (
      GTK_CONTAINER (self));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      GtkWidget * widget = GTK_WIDGET (iter->data);
      if (widget != (GtkWidget *) ar_prv->bg &&
          widget != (GtkWidget *) ar_prv->playhead)
        {
          g_object_ref (widget);
          gtk_container_remove (
            GTK_CONTAINER (self),
            widget);
        }
    }
  g_list_free (children);

  if (CLIP_EDITOR->region &&
      CLIP_EDITOR->region->type == REGION_TYPE_MIDI)
    {
      MidiNote * mn;

      /* add notes of main region */
      Region * mr = CLIP_EDITOR->region;
      for (int j = 0; j < mr->num_midi_notes; j++)
        {
          mn = mr->midi_notes[j];
          gtk_overlay_add_overlay (
            GTK_OVERLAY (self),
            GTK_WIDGET (mn->widget));
        }

      /* add notes of all other regions */
      Region * other_r;
      for (int i = 0; i < mr->track->num_regions; i++)
        {
          other_r = mr->track->regions[i];
          if (!g_strcmp0 (mr->name, other_r->name))
            continue;

          for (int j = 0;
               j < other_r->num_midi_notes; j++)
            {
              mn = other_r->midi_notes[j];
              gtk_overlay_add_overlay (
                GTK_OVERLAY (self),
                GTK_WIDGET (mn->widget));
            }
        }
    }
}

static gboolean
on_focus (GtkWidget       *widget,
          gpointer         user_data)
{
  /*g_message ("timeline focused");*/
  MAIN_WINDOW->last_focused = widget;

  return FALSE;
}

void
midi_arranger_widget_auto_scroll (
  MidiArrangerWidget * self,
  GtkScrolledWindow *  scrolled_window,
  int                  transient)
{
  Region * region = CLIP_EDITOR->region;
  int scroll_speed = 20;
  int border_distance = 10;
  g_message ("midi auto scrolling");
  if (region != 0)
  {
    MidiNote * first_note =
      midi_arranger_selections_get_first_midi_note (
        MA_SELECTIONS, transient);
    MidiNote * last_note =
      midi_arranger_selections_get_last_midi_note (
        MA_SELECTIONS, transient);
    MidiNote * lowest_note =
      midi_arranger_selections_get_lowest_note (
        MA_SELECTIONS, transient);
    MidiNote * highest_note =
      midi_arranger_selections_get_highest_note (
        MA_SELECTIONS, transient);
    g_warn_if_fail (first_note && last_note &&
              lowest_note && highest_note &&
              first_note->widget &&
              last_note->widget &&
              lowest_note->widget &&
              highest_note->widget);
    int arranger_width =
      gtk_widget_get_allocated_width (
        GTK_WIDGET (scrolled_window));
    int arranger_height =
      gtk_widget_get_allocated_height (
        GTK_WIDGET (scrolled_window));
    GtkAdjustment *hadj =
      gtk_scrolled_window_get_hadjustment (
        GTK_SCROLLED_WINDOW (scrolled_window));
    GtkAdjustment *vadj =
      gtk_scrolled_window_get_vadjustment (
        GTK_SCROLLED_WINDOW (scrolled_window));
    int v_delta = 0;
    int h_delta = 0;
    if (lowest_note != 0)
    {
      GtkWidget *focused =
        GTK_WIDGET (lowest_note->widget);
      gint note_x, note_y;
      gtk_widget_translate_coordinates (
        focused,
        GTK_WIDGET (scrolled_window),
        0,
        0,
        &note_x,
        &note_y);
      int note_height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (focused));
      if (note_y + note_height + border_distance
        >= arranger_height)
      {
        v_delta = scroll_speed;
      }
    }
    if (highest_note != 0)
    {
      GtkWidget *focused = GTK_WIDGET (
        highest_note->widget);
      gint note_x, note_y;
      gtk_widget_translate_coordinates (
        focused,
        GTK_WIDGET (scrolled_window),
        0,
        0,
        &note_x,
        &note_y);
      /*int note_height = gtk_widget_get_allocated_height (*/
        /*GTK_WIDGET (focused));*/
      if (note_y - border_distance <= 1)
      {
        v_delta = scroll_speed * -1;
      }
    }
    if (first_note != 0)
    {
      gint note_x, note_y;
      GtkWidget *focused =
        GTK_WIDGET (first_note->widget);
      gtk_widget_translate_coordinates (
        focused,
        GTK_WIDGET (scrolled_window),
        0,
        0,
        &note_x,
        &note_y);
      /*int note_width = gtk_widget_get_allocated_width (*/
        /*GTK_WIDGET (focused));*/
      if (note_x - border_distance <= 10)
      {
        h_delta = scroll_speed * -1;
      }
    }
    if (last_note != 0)
    {
      gint note_x, note_y;
      GtkWidget *focused = GTK_WIDGET (last_note->widget);
      gtk_widget_translate_coordinates (
        focused,
        GTK_WIDGET (scrolled_window),
        0,
        0,
        &note_x,
        &note_y);
      int note_width = gtk_widget_get_allocated_width (
        GTK_WIDGET (focused));
      if (note_x + note_width + border_distance
        > arranger_width)
      {
        h_delta = scroll_speed;
      }
    }
    if (h_delta != 0)
    {
      gtk_adjustment_set_value (
        hadj,
        gtk_adjustment_get_value (hadj) + h_delta);
    }
    if (v_delta != 0)
    {
      gtk_adjustment_set_value (
        vadj,
        gtk_adjustment_get_value (vadj) + v_delta);
    }
  }
}

static void
midi_arranger_widget_class_init (
  MidiArrangerWidgetClass * klass)
{
}

static void
midi_arranger_widget_init (MidiArrangerWidget *self)
{
  g_signal_connect (
    self, "grab-focus",
    G_CALLBACK (on_focus), self);
}
