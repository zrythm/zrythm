/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm.h"
#include "actions/actions.h"
#include "actions/duplicate_midi_arranger_selections_action.h"
#include "actions/move_midi_arranger_selections_action.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/midi_region.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_playhead.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_arranger_bg.h"
#include "gui/widgets/audio_clip_editor.h"
#include "gui/widgets/audio_ruler.h"
#include "gui/widgets/automation_curve.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/gtk/gtkeventcontrollermotion.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_modifier_arranger_bg.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "gui/widgets/velocity.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE_WITH_PRIVATE (
  ArrangerWidget,
  arranger_widget,
  GTK_TYPE_OVERLAY)

DEFINE_START_POS

#define GET_PRIVATE ARRANGER_WIDGET_GET_PRIVATE (self)

/**
 * Gets each specific arranger.
 *
 * Useful boilerplate
 */
#define GET_ARRANGER_ALIASES(self) \
  TimelineArrangerWidget *     timeline = NULL; \
  MidiArrangerWidget *         midi_arranger = NULL; \
  MidiModifierArrangerWidget * midi_modifier_arranger = NULL; \
  AudioArrangerWidget * audio_arranger = NULL; \
  if (ARRANGER_IS_MIDI (self)) \
    { \
      midi_arranger = Z_MIDI_ARRANGER_WIDGET (self); \
    } \
  else if (ARRANGER_IS_TIMELINE (self)) \
    { \
      timeline = Z_TIMELINE_ARRANGER_WIDGET (self); \
    } \
  else if (Z_IS_AUDIO_ARRANGER_WIDGET (self)) \
    { \
      audio_arranger = Z_AUDIO_ARRANGER_WIDGET (self); \
    } \
  else if (ARRANGER_IS_MIDI_MODIFIER (self)) \
    { \
      midi_modifier_arranger = \
        Z_MIDI_MODIFIER_ARRANGER_WIDGET (self); \
    }

ArrangerWidgetPrivate *
arranger_widget_get_private (ArrangerWidget * self)
{
  return arranger_widget_get_instance_private (self);
}

/**
 * Gets called to set the position/size of each overlayed widget.
 */
static gboolean
get_child_position (GtkOverlay   *self,
                    GtkWidget    *widget,
                    GdkRectangle *allocation,
                    gpointer      user_data)
{
  GET_ARRANGER_ALIASES (self);

  if (Z_IS_ARRANGER_PLAYHEAD_WIDGET (widget))
    {
      /* note: -1 (half the width of the playhead
       * widget */
      if (timeline)
        allocation->x =
          ui_pos_to_px_timeline (
            &TRANSPORT->playhead_pos,
            1) - 1;
      else if ((midi_arranger ||
               midi_modifier_arranger ||
               audio_arranger) &&
               CLIP_EDITOR->region)
        {
          allocation->x =
            ui_pos_to_px_piano_roll (
              &TRANSPORT->playhead_pos,
              1);
        }
      allocation->y = 0;
      allocation->width = 2;
      allocation->height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (self));
    }
  else if (midi_arranger)
    {
      midi_arranger_widget_set_allocation (
        midi_arranger,
        widget,
        allocation);
    }
  else if (timeline)
    {
      timeline_arranger_widget_set_allocation (
        timeline,
        widget,
        allocation);
    }
  else if (midi_modifier_arranger)
    {
      midi_modifier_arranger_widget_set_allocation (
        midi_modifier_arranger,
        widget,
        allocation);
    }
  else if (audio_arranger)
    {
      audio_arranger_widget_set_allocation (
        audio_arranger,
        widget,
        allocation);
    }
  return TRUE;
}

/**
 * Sets the cursor on the arranger and all of its
 * children.
 */
void
arranger_widget_set_cursor (
  ArrangerWidget * self,
  ArrangerCursor   cursor)
{
  GList *children, *iter;

#define SET_X_CURSOR(x) \
  ui_set_##x##_cursor (self); \
  children = \
    gtk_container_get_children ( \
      GTK_CONTAINER (self)); \
  for (iter = children; \
       iter != NULL; \
       iter = g_list_next (iter)) \
    { \
      ui_set_##x##_cursor ( \
        GTK_WIDGET (iter->data)); \
    } \
  g_list_free (children)


#define SET_CURSOR_FROM_NAME(name) \
  ui_set_cursor_from_name ( \
    GTK_WIDGET (self), name); \
  children = \
    gtk_container_get_children ( \
      GTK_CONTAINER (self)); \
  for (iter = children; \
       iter != NULL; \
       iter = g_list_next (iter)) \
    { \
      ui_set_cursor_from_name ( \
        GTK_WIDGET (iter->data), \
        name); \
    } \
  g_list_free (children)

  switch (cursor)
    {
    case ARRANGER_CURSOR_SELECT:
      SET_X_CURSOR (pointer);
      break;
    case ARRANGER_CURSOR_EDIT:
      SET_X_CURSOR (pencil);
      break;
    case ARRANGER_CURSOR_CUT:
      SET_X_CURSOR (cut_clip);
      break;
    case ARRANGER_CURSOR_RAMP:
      SET_X_CURSOR (line);
      break;
    case ARRANGER_CURSOR_ERASER:
      SET_X_CURSOR (eraser);
      break;
    case ARRANGER_CURSOR_AUDITION:
      SET_X_CURSOR (speaker);
      break;
    case ARRANGER_CURSOR_GRAB:
      SET_X_CURSOR (hand);
      break;
    case ARRANGER_CURSOR_GRABBING:
      SET_CURSOR_FROM_NAME ("grabbing");
      break;
    case ARRANGER_CURSOR_GRABBING_COPY:
      SET_CURSOR_FROM_NAME ("copy");
      break;
    case ARRANGER_CURSOR_GRABBING_LINK:
      SET_CURSOR_FROM_NAME ("link");
      break;
    case ARRANGER_CURSOR_RESIZING_L:
      SET_CURSOR_FROM_NAME ("w-resize");
      break;
    case ARRANGER_CURSOR_RESIZING_R:
      SET_CURSOR_FROM_NAME ("e-resize");
      break;
    case ARRANGER_CURSOR_RESIZING_UP:
      SET_CURSOR_FROM_NAME ("n-resize");
      break;
    case ARRANGER_CURSOR_RANGE:
      SET_CURSOR_FROM_NAME ("text");
      break;
    default:
      g_warn_if_reached ();
      break;
    }
}

void
arranger_widget_get_hit_widgets_in_range (
  ArrangerWidget *  self,
  GType             type,
  double            start_x,
  double            start_y,
  double            offset_x,
  double            offset_y,
  GtkWidget **      array, ///< array to fill
  int *             array_size) ///< array_size to fill
{
  GList *children, *iter;

  /* go through each overlay child */
  children =
    gtk_container_get_children (
      GTK_CONTAINER (self));
  for (iter = children;
       iter != NULL; iter = g_list_next (iter))
    {
      GtkWidget * widget = GTK_WIDGET (iter->data);

      GtkAllocation allocation;
      gtk_widget_get_allocation (widget,
                                 &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (self),
                GTK_WIDGET (widget),
                start_x,
                start_y,
                &wx,
                &wy);

      int x_hit, y_hit;
      if (offset_x < 0)
        {
          x_hit = wx + offset_x <= allocation.width &&
            wx > 0;
        }
      else
        {
          x_hit = wx <= allocation.width &&
            wx + offset_x > 0;
        }
      if (!x_hit) continue;
      if (offset_y < 0)
        {
          y_hit = wy + offset_y <= allocation.height &&
            wy > 0;
        }
      else
        {
          y_hit = wy <= allocation.height &&
            wy + offset_y > 0;
        }
      if (!y_hit) continue;

      if (type == MIDI_NOTE_WIDGET_TYPE &&
          Z_IS_MIDI_NOTE_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type == REGION_WIDGET_TYPE &&
               Z_IS_REGION_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type ==
                 AUTOMATION_POINT_WIDGET_TYPE &&
               Z_IS_AUTOMATION_POINT_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type == CHORD_OBJECT_WIDGET_TYPE &&
               Z_IS_CHORD_OBJECT_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type == SCALE_OBJECT_WIDGET_TYPE &&
               Z_IS_SCALE_OBJECT_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type == MARKER_WIDGET_TYPE &&
               Z_IS_MARKER_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type == VELOCITY_WIDGET_TYPE &&
               Z_IS_VELOCITY_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
    }
}

/**
 * Returns if the arranger is in a moving-related
 * operation or starting a moving-related operation.
 *
 * Useful to know if we need transient widgets or
 * not.
 */
int
arranger_widget_is_in_moving_operation (
  ArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING ||
      ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING_COPY ||
      ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING_LINK ||
      ar_prv->action ==
        UI_OVERLAY_ACTION_MOVING ||
      ar_prv->action ==
        UI_OVERLAY_ACTION_MOVING_COPY ||
      ar_prv->action ==
        UI_OVERLAY_ACTION_MOVING_LINK)
    return 1;

  return 0;
}

/**
 * Selects/deselects the object, optionally
 * appending it to
 * the selected items or making it the only
 * selected item.
 */
void
arranger_widget_select (
  ArrangerWidget * self,
  GType            type,
  void *           child,
  int              select,
  int              append)
{
  GET_ARRANGER_ALIASES (self);

  void ** array = NULL;
  int * num = NULL;

#define FIND_ARRAY_AND_NUM( \
  caps,sc,selections) \
  if (type == caps##_WIDGET_TYPE) \
    { \
      array = \
        (void **) selections->sc##s; \
      num = &selections->num_##sc##s; \
    }
#define SELECT_WIDGET( \
  caps,cc,sc,select) \
  if (type == caps##_WIDGET_TYPE) \
    sc##_widget_select ( \
      ((cc *)child)->widget, select);

  if (timeline)
    {
      FIND_ARRAY_AND_NUM (
        REGION, region, TL_SELECTIONS);
      FIND_ARRAY_AND_NUM (
        CHORD_OBJECT, chord_object, TL_SELECTIONS);
      FIND_ARRAY_AND_NUM (
        SCALE_OBJECT, scale_object, TL_SELECTIONS);
      FIND_ARRAY_AND_NUM (
        MARKER, marker, TL_SELECTIONS);
      FIND_ARRAY_AND_NUM (
        AUTOMATION_POINT, automation_point,
        TL_SELECTIONS);
    }
  if (midi_arranger)
    {
      FIND_ARRAY_AND_NUM (
        MIDI_NOTE, midi_note, MA_SELECTIONS);
    }
  if (audio_arranger)
    {
      /* TODO */

    }
  if (midi_modifier_arranger)
    {
      FIND_ARRAY_AND_NUM (
        MIDI_NOTE, midi_note, MA_SELECTIONS);
    }

  if (select && !append)
    {
      /* deselect existing selections */
      if (timeline)
        timeline_selections_clear (
          TL_SELECTIONS);
      else if (midi_arranger)
        midi_arranger_selections_clear (
          MA_SELECTIONS);
      else if (audio_arranger)
        {
          /* TODO */
        }
      else if (midi_modifier_arranger)
        {
          midi_arranger_selections_clear (
            MA_SELECTIONS);
        }
    }

  /* if we are deselecting and the item is
   * selected */
  if (!select &&
      array_contains (array, *num, child))
    {
      /* deselect */
      SELECT_WIDGET (
        REGION, Region, region, F_NO_SELECT);
      SELECT_WIDGET (
        CHORD_OBJECT, ChordObject, chord_object,
        F_NO_SELECT);
      SELECT_WIDGET (
        SCALE_OBJECT, ScaleObject, scale_object,
        F_NO_SELECT);
      SELECT_WIDGET (
        MARKER, Marker, marker,  F_NO_SELECT);
      SELECT_WIDGET (
        AUTOMATION_POINT, AutomationPoint,
        automation_point,  F_NO_SELECT);
      SELECT_WIDGET (
        MIDI_NOTE, MidiNote, midi_note,
        F_NO_SELECT);
    }
  /* if selecting */
  else if (select &&
           !array_contains (array, *num, child))
    {
      /* select */
      SELECT_WIDGET (
        REGION, Region, region,
        F_SELECT);
      SELECT_WIDGET (
        CHORD_OBJECT, ChordObject, chord_object,
        F_SELECT);
      SELECT_WIDGET (
        SCALE_OBJECT, ScaleObject, scale_object,
        F_SELECT);
      SELECT_WIDGET (
        MARKER, Marker, marker,
        F_SELECT);
      SELECT_WIDGET (
        AUTOMATION_POINT, AutomationPoint,
        automation_point,
        F_SELECT);
      SELECT_WIDGET (
        MIDI_NOTE, MidiNote, midi_note,
        F_SELECT);
    }

#undef FIND_ARRAY_AND_NUM
#undef SELECT_WIDGET
}

/**
 * Refreshes all arranger backgrounds.
 */
void
arranger_widget_refresh_all_backgrounds ()
{
  ArrangerWidgetPrivate * ar_prv;

  ar_prv =
    arranger_widget_get_private (
      Z_ARRANGER_WIDGET (MW_TIMELINE));
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
  ar_prv =
    arranger_widget_get_private (
      Z_ARRANGER_WIDGET (MIDI_ARRANGER));
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
  ar_prv =
    arranger_widget_get_private (
      Z_ARRANGER_WIDGET (MIDI_MODIFIER_ARRANGER));
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
  ar_prv =
    arranger_widget_get_private (
      Z_ARRANGER_WIDGET (AUDIO_ARRANGER));
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
}

void
arranger_widget_select_all (
  ArrangerWidget *  self,
  int               select)
{
  GET_ARRANGER_ALIASES (self);

  if (midi_arranger)
    {
      midi_arranger_widget_select_all (
        midi_arranger, select);
    }
  else if (timeline)
    {
      timeline_arranger_widget_select_all (
        timeline, select);
    }
  else if (midi_modifier_arranger)
    {
      midi_modifier_arranger_widget_select_all (
        midi_modifier_arranger, select);
    }
  else if (audio_arranger)
    {

    }
}

static void
show_context_menu (
  ArrangerWidget * self,
  gdouble          x,
  gdouble          y)
{
  GET_ARRANGER_ALIASES (self);

  if (midi_arranger)
    {
      midi_arranger_widget_show_context_menu (
        midi_arranger, x, y);
    }
  else if (timeline)
    {
      timeline_arranger_widget_show_context_menu (
        timeline, x, y);
    }
  else if (audio_arranger)
    {
      /*audio_arranger_widget_show_context_menu (*/
        /*Z_AUDIO_ARRANGER_WIDGET (self));*/
    }
  else if (midi_modifier_arranger)
    {

    }
}

static void
on_right_click (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               ArrangerWidget *      self)
{
  if (n_press != 1)
    return;

  show_context_menu (self, x, y);
}

static void
auto_scroll (ArrangerWidget * self)
{
  return;
  g_message ("AUTO SCROLL");
  ARRANGER_WIDGET_GET_PRIVATE (self);
	GET_ARRANGER_ALIASES(self);
	GtkScrolledWindow *scrolled =
		arranger_widget_get_scrolled_window (self);
	if (midi_arranger)
	{
		midi_arranger_widget_auto_scroll(
      midi_arranger, scrolled,
      ar_prv->action ==
        UI_OVERLAY_ACTION_MOVING ||
      ar_prv->action ==
        UI_OVERLAY_ACTION_MOVING_COPY ||
      ar_prv->action ==
        UI_OVERLAY_ACTION_MOVING_LINK);
	};
  if (timeline)
    {

    }
  if (midi_modifier_arranger)
    {

    }
  if (audio_arranger)
    {

    }
}

static gboolean
on_key_release_action (
	GtkWidget *widget,
	GdkEventKey *event,
	ArrangerWidget * self)
{
  GET_PRIVATE;
  GET_ARRANGER_ALIASES (self);
  ar_prv->key_is_pressed = 0;

  if (event->keyval == GDK_KEY_Control_L ||
      event->keyval == GDK_KEY_Control_R)
    {
      ar_prv->ctrl_held = 0;
    }

  if (ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      if (ar_prv->ctrl_held)
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING_COPY;
      else
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING;
    }
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING &&
           ar_prv->ctrl_held)
    ar_prv->action =
      UI_OVERLAY_ACTION_MOVING_COPY;
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING_COPY &&
           !ar_prv->ctrl_held)
    ar_prv->action =
      UI_OVERLAY_ACTION_MOVING;

  if (midi_arranger)
    midi_arranger_widget_update_visibility (
      midi_arranger);
  if (timeline)
    timeline_arranger_widget_update_visibility (
      timeline);
  if (midi_modifier_arranger)
    {
      midi_modifier_arranger_widget_update_visibility (
        midi_modifier_arranger);
    }
  if (audio_arranger)
    {

    }

  arranger_widget_refresh_cursor (
    self);

  return TRUE;
}

static gboolean
on_key_action (
  GtkWidget *widget,
  GdkEventKey *event,
  ArrangerWidget * self)
{
  GET_PRIVATE;
  GET_ARRANGER_ALIASES(self);

  int num = 0;

  if (event->type == GDK_KEY_PRESS &&
      (event->keyval = GDK_KEY_Control_L ||
       event->keyval == GDK_KEY_Control_R))
    {
      ar_prv->ctrl_held = 1;
    }

  if (midi_arranger)
    {
      num =
        MA_SELECTIONS->num_midi_notes;
      if (event->state & GDK_CONTROL_MASK &&
          event->type == GDK_KEY_PRESS &&
          event->keyval == GDK_KEY_d &&
          num > 0)
        {
          /*UndoableAction * duplicate_action =*/
            /*duplicate_midi_arranger_selections_action_new ();*/
          /*undo_manager_perform (*/
            /*UNDO_MANAGER, duplicate_action);*/
          return FALSE;
        }
      if (event->type == GDK_KEY_PRESS &&
          event->keyval == GDK_KEY_Up &&
          num > 0)
        {
          UndoableAction * shift_up_action =
            move_midi_arranger_selections_action_new (
              MA_SELECTIONS, 0, 1);
          undo_manager_perform (
          UNDO_MANAGER, shift_up_action);
          return TRUE;
        }
      if (event->type == GDK_KEY_PRESS &&
          event->keyval == GDK_KEY_Down &&
          num > 0)
        {
          UndoableAction * shift_down_action =
            move_midi_arranger_selections_action_new (
              MA_SELECTIONS, 0, -1);
          undo_manager_perform (
          UNDO_MANAGER, shift_down_action);
          return TRUE;
        }
      if (event->type == GDK_KEY_PRESS &&
          event->keyval == GDK_KEY_Left &&
          num > 0)
        {
          /*UndoableAction * shift_left_action =*/
            /*move_midi_arranger_selections_action_new (-1);*/
          /*undo_manager_perform (*/
          /*UNDO_MANAGER, shift_left_action);*/
          return TRUE;
        }
      if (event->type == GDK_KEY_PRESS &&
          event->keyval == GDK_KEY_Right &&
          num > 0)
        {
          /*UndoableAction * shift_right_action =*/
            /*move_midi_arranger_selections_pos_action_new (1);*/
          /*undo_manager_perform (*/
          /*UNDO_MANAGER, shift_right_action);*/
        }
    }
  if (timeline)
    {
      /* TODO */

    }
  if (midi_modifier_arranger)
    {

    }
  if (audio_arranger)
    {

    }

  if (ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      if (ar_prv->ctrl_held)
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING_COPY;
      else
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING;
    }
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING &&
           ar_prv->ctrl_held)
    ar_prv->action =
      UI_OVERLAY_ACTION_MOVING_COPY;
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING_COPY &&
           !ar_prv->ctrl_held)
    ar_prv->action =
      UI_OVERLAY_ACTION_MOVING;

  if (midi_arranger)
    midi_arranger_widget_update_visibility (
      midi_arranger);
  if (midi_modifier_arranger)
    midi_modifier_arranger_widget_update_visibility (
      midi_modifier_arranger);
  else if (timeline)
    timeline_arranger_widget_update_visibility (
      timeline);

  arranger_widget_refresh_cursor (
    self);

  if (num > 0)
    auto_scroll (self);

  return FALSE;
}

/**
 * On button press.
 *
 * This merely sets the number of clicks and
 * objects clicked on. It is always called
 * before drag_begin, so the logic is done in drag_begin.
 */
static void
multipress_pressed (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  ArrangerWidget *      self)
{
  GET_PRIVATE;

  /* set number of presses */
  ar_prv->n_press = n_press;

  /* set modifier button states */
  UI_GET_STATE_MASK (gesture);
  if (state_mask & GDK_SHIFT_MASK)
    ar_prv->shift_held = 1;
  if (state_mask & GDK_CONTROL_MASK)
    ar_prv->ctrl_held = 1;
}

/**
 * Called when an item needs to be created at the
 * given position.
 */
static void
create_item (ArrangerWidget * self,
             double           start_x,
             double           start_y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  GET_ARRANGER_ALIASES (self);

  /* something will be created */
  Position pos;
  Track * track = NULL;
  AutomationTrack * at = NULL;
  int note;
  Region * region = NULL;

  /* get the position */
  arranger_widget_px_to_pos (
    self, start_x, &pos, 1);

  /* snap it */
  if (!ar_prv->shift_held &&
      SNAP_GRID_ANY_SNAP (ar_prv->snap_grid))
    position_snap (
      NULL, &pos, NULL, NULL,
      ar_prv->snap_grid);

  if (timeline)
    {
      /* figure out if we are creating a region or
       * automation point */
      at =
        timeline_arranger_widget_get_automation_track_at_y (timeline, start_y);
      if (!at)
        track =
          timeline_arranger_widget_get_track_at_y (
            timeline, start_y);

      /* creating automation point */
      if (at)
        {
          timeline_arranger_widget_create_ap (
            timeline,
            at,
            &pos,
            start_y);
        }
      /* double click inside a track */
      else if (track)
        {
          TrackLane * lane =
            timeline_arranger_widget_get_track_lane_at_y (
              timeline, start_y);
          switch (track->type)
            {
            case TRACK_TYPE_INSTRUMENT:
            case TRACK_TYPE_AUDIO:
              timeline_arranger_widget_create_region (
                timeline,
                track,
                lane,
                &pos);
              break;
            case TRACK_TYPE_MASTER:
              break;
            case TRACK_TYPE_CHORD:
              timeline_arranger_widget_create_chord_or_scale (
                timeline,
                track,
                start_y,
                &pos);
            case TRACK_TYPE_BUS:
              break;
            case TRACK_TYPE_GROUP:
              break;
            case TRACK_TYPE_MARKER:
              timeline_arranger_widget_create_marker (
                timeline,
                track,
                &pos);
              break;
            default:
              /* TODO */
              break;
            }
        }
    }
  if (midi_arranger)
    {
      /* find the note and region at x,y */
      note =
        midi_arranger_widget_get_note_at_y (start_y);
      region =
        CLIP_EDITOR->region;

      /* create a note */
      if (region)
        {
          midi_arranger_widget_create_note (
            midi_arranger,
            &pos,
            note,
            (MidiRegion *) region);
        }
    }
  if (midi_modifier_arranger)
    {

    }
  if (audio_arranger)
    {

    }

  /* something is (likely) added so reallocate */
  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
drag_cancel (
  GtkGesture *       gesture,
  GdkEventSequence * sequence,
  ArrangerWidget *   self)
{
  g_message ("drag cancelled");
}

static void
drag_begin (GtkGestureDrag *   gesture,
            gdouble            start_x,
            gdouble            start_y,
            ArrangerWidget *   self)
{
  GET_PRIVATE;

  ar_prv->start_x = start_x;
  arranger_widget_px_to_pos (
    self, start_x, &ar_prv->start_pos, 1);
  ar_prv->start_y = start_y;

  if (!gtk_widget_has_focus (GTK_WIDGET (self)))
    gtk_widget_grab_focus (GTK_WIDGET (self));

  GET_ARRANGER_ALIASES (self);

  /* get hit arranger objects */
  /* FIXME move this part to multipress, makes
   * more sense and makes this function smaller/
   * easier to understand.
   */
  MidiNoteWidget *        midi_note_widget = NULL;
  RegionWidget *          rw = NULL;
  AutomationCurveWidget * ac_widget = NULL;
  AutomationPointWidget * ap_widget = NULL;
  ChordObjectWidget *     chord_widget = NULL;
  ScaleObjectWidget *     scale_widget = NULL;
  MarkerWidget *          mw = NULL;
  VelocityWidget *        vel_widget = NULL;
  if (midi_arranger)
    {
      midi_note_widget =
        midi_arranger_widget_get_hit_midi_note (
          midi_arranger, start_x, start_y);
    }
  else if (audio_arranger)
    {
    }
  else if (midi_modifier_arranger)
    {
      vel_widget =
        midi_modifier_arranger_widget_get_hit_velocity (
          midi_modifier_arranger, start_x, start_y);
    }
  else if (timeline)
    {
      rw =
        timeline_arranger_widget_get_hit_region (
          timeline, start_x, start_y);
      ac_widget =
        timeline_arranger_widget_get_hit_curve (
          timeline, start_x, start_y);
      ap_widget =
        timeline_arranger_widget_get_hit_ap (
          timeline, start_x, start_y);
      chord_widget =
        timeline_arranger_widget_get_hit_chord (
          timeline, start_x, start_y);
      scale_widget =
        timeline_arranger_widget_get_hit_scale (
          timeline, start_x, start_y);
      mw =
        timeline_arranger_widget_get_hit_marker (
          timeline, start_x, start_y);
    }

  /* if something is hit */
  int is_hit =
    midi_note_widget || rw || ac_widget ||
    ap_widget || chord_widget || vel_widget ||
    scale_widget || mw;
  if (is_hit)
    {
      /* set selections, positions, actions, cursor */
      if (midi_note_widget)
        {
          midi_arranger_widget_on_drag_begin_note_hit (
            midi_arranger, start_x, midi_note_widget);
        }
      else if (rw)
        {
          timeline_arranger_widget_on_drag_begin_region_hit (
            timeline, start_x, rw);
        }
      else if (chord_widget)
        {
          timeline_arranger_widget_on_drag_begin_chord_hit (
            timeline, start_x, chord_widget);
        }
      else if (scale_widget)
        {
          timeline_arranger_widget_on_drag_begin_scale_hit (
            timeline, start_x, scale_widget);
        }
      else if (ap_widget)
        {
          timeline_arranger_widget_on_drag_begin_ap_hit (
            timeline, start_x, ap_widget);
        }
      else if (ac_widget)
        {
          timeline->start_ac =
            ac_widget->ac;
        }
      else if (vel_widget)
        {
          midi_modifier_arranger_on_drag_begin_vel_hit (
            midi_modifier_arranger,
            vel_widget, start_y);
        }
      else if (mw)
        {
          timeline_arranger_widget_on_drag_begin_marker_hit (
            timeline, start_x, mw);
        }
    }
  else /* nothing hit */
    {
      /* single click */
      if (ar_prv->n_press == 1)
        {
          if (P_TOOL == TOOL_SELECT_NORMAL ||
              P_TOOL == TOOL_SELECT_STRETCH)
            {
              /* selection */
              ar_prv->action =
                UI_OVERLAY_ACTION_STARTING_SELECTION;

              /* deselect all */
              arranger_widget_select_all (self, 0);

              /* if timeline, set either selecting
               * objects or selecting range */
              if (timeline)
                {
                  timeline_arranger_widget_set_select_type (
                    timeline,
                    start_y);
                }
            }
          else if (P_TOOL == TOOL_EDIT)
            {
              if (!ar_prv->ctrl_held)
                {
                  /* something is created */
                  create_item (
                    self, start_x, start_y);
                }
              else
                {
                  /* autofill */
                }
            }
          else if (P_TOOL == TOOL_ERASER)
            {
              /* delete selection */
              ar_prv->action =
                UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION;
            }
          else if (P_TOOL == TOOL_RAMP)
            {
              ar_prv->action =
                UI_OVERLAY_ACTION_STARTING_RAMP;
            }
        }
      /* double click */
      else if (ar_prv->n_press == 2)
        {
          if (P_TOOL == TOOL_SELECT_NORMAL ||
              P_TOOL == TOOL_SELECT_STRETCH ||
              P_TOOL == TOOL_EDIT)
            {
              create_item (
                self, start_x, start_y);
            }
          else if (P_TOOL == TOOL_ERASER)
            {
              /* delete selection */
              ar_prv->action =
                UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION;
            }
        }
    }

  /* set start pos */
  position_set_to_bar (
    &ar_prv->earliest_obj_start_pos, 2000);
  if (timeline &&
      timeline_selections_has_any (
        TL_SELECTIONS))
    {
      timeline_selections_get_start_pos (
        TL_SELECTIONS,
        &ar_prv->earliest_obj_start_pos,
        0);
      timeline_selections_set_cache_poses (
        TL_SELECTIONS);
      ar_prv->earliest_obj_exists = 1;
    }
  else if (midi_arranger &&
           midi_arranger_selections_has_any (
             MA_SELECTIONS))
    {
      midi_arranger_selections_get_start_pos (
        MA_SELECTIONS,
        &ar_prv->earliest_obj_start_pos,
        0, 1);
      midi_arranger_selections_set_cache_poses (
        MA_SELECTIONS);
      ar_prv->earliest_obj_exists = 1;
    }
  else if (midi_modifier_arranger &&
           midi_arranger_selections_has_any (
             MA_SELECTIONS))
    {
      midi_arranger_selections_get_start_pos (
        MA_SELECTIONS,
        &ar_prv->earliest_obj_start_pos,
        0, 1);
      midi_arranger_selections_set_cache_poses (
        MA_SELECTIONS);
      ar_prv->earliest_obj_exists = 1;
    }
  else
    {
      ar_prv->earliest_obj_exists = 0;
    }

  arranger_widget_refresh_cursor (self);
}


static void
drag_update (
  GtkGestureDrag * gesture,
  gdouble         offset_x,
  gdouble         offset_y,
  ArrangerWidget * self)
{
  GET_PRIVATE;

  /* state mask needs to be updated */
  UI_GET_STATE_MASK (gesture);
  if (state_mask & GDK_SHIFT_MASK)
    ar_prv->shift_held = 1;
  else
    ar_prv->shift_held = 0;
  if (state_mask & GDK_CONTROL_MASK)
    ar_prv->ctrl_held = 1;
  else
    ar_prv->ctrl_held = 0;

  GET_ARRANGER_ALIASES (self);

  /* get current pos */
  arranger_widget_px_to_pos (
    self, ar_prv->start_x + offset_x,
    &ar_prv->curr_pos, 1);

  /* get difference with drag start pos */
  ar_prv->curr_ticks_diff_from_start =
    position_get_ticks_diff (
      &ar_prv->curr_pos,
      &ar_prv->start_pos,
      NULL);

  if (ar_prv->earliest_obj_exists)
    {
      /* add diff to the earliest object's start pos
       * and snap it, then get the diff ticks */
      Position earliest_obj_new_pos;
      position_set_to_pos (
        &earliest_obj_new_pos,
        &ar_prv->earliest_obj_start_pos);
      position_add_ticks (
        &earliest_obj_new_pos,
        ar_prv->curr_ticks_diff_from_start);

      if (position_is_before (
            &earliest_obj_new_pos, START_POS))
        {
          /* stop at 0.0.0.0 */
          position_set_to_pos (
            &earliest_obj_new_pos, START_POS);
        }
      else if (!ar_prv->shift_held &&
          SNAP_GRID_ANY_SNAP (ar_prv->snap_grid))
        {
          position_snap (
            NULL, &earliest_obj_new_pos, NULL,
            NULL, ar_prv->snap_grid);
        }
      ar_prv->adj_ticks_diff =
        position_get_ticks_diff (
          &earliest_obj_new_pos,
          &ar_prv->earliest_obj_start_pos,
          NULL);
    }

  /* set action to selecting if starting selection.
   * this
   * is because drag_update never gets called if
   * it's just
   * a click, so we can check at drag_end and see if
   * anything was selected */
  if (ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_SELECTION)
    ar_prv->action =
      UI_OVERLAY_ACTION_SELECTING;
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION)
    ar_prv->action =
      UI_OVERLAY_ACTION_DELETE_SELECTING;
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      if (ar_prv->ctrl_held)
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING_COPY;
      else
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING;
    }
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING &&
           ar_prv->ctrl_held)
    ar_prv->action =
      UI_OVERLAY_ACTION_MOVING_COPY;
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING_COPY &&
           !ar_prv->ctrl_held)
    ar_prv->action =
      UI_OVERLAY_ACTION_MOVING;
  else if (ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_RAMP)
    ar_prv->action =
      UI_OVERLAY_ACTION_RAMPING;

  /* if drawing a selection */
  switch (ar_prv->action)
    {
    case UI_OVERLAY_ACTION_SELECTING:
      /* find and select objects inside selection */
      if (timeline)
        {
          timeline_arranger_widget_select (
            timeline,
            offset_x,
            offset_y,
            F_NO_DELETE);
        }
      else if (midi_arranger)
        {
          midi_arranger_widget_select (
            midi_arranger,
            offset_x,
            offset_y,
            F_NO_DELETE);
        }
      else if (midi_modifier_arranger)
        {
          midi_modifier_arranger_widget_select (
            midi_modifier_arranger,
            offset_x,
            offset_y,
            F_NO_DELETE);
        }
      else if (audio_arranger)
        {
          /* TODO */
        }
      break;
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
      /* find and delete objects inside
       * selection */
      if (timeline)
        {
          timeline_arranger_widget_select (
            timeline,
            offset_x,
            offset_y,
            1);
        }
      else if (midi_arranger)
        {
          midi_arranger_widget_select (
            midi_arranger,
            offset_x,
            offset_y,
            1);
        }
      else if (midi_modifier_arranger)
        {
          midi_modifier_arranger_widget_select (
            midi_modifier_arranger,
            offset_x,
            offset_y,
            1);
        }
      else if (audio_arranger)
        {
          /* TODO */
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      /* snap selections based on new pos */
      if (timeline)
        {
          timeline_arranger_widget_update_visibility (
            timeline);
          int ret =
            timeline_arranger_widget_snap_regions_l (
              timeline,
              &ar_prv->curr_pos, 1);
          if (!ret)
            timeline_arranger_widget_snap_regions_l (
              timeline,
              &ar_prv->curr_pos, 0);
        }
      else if (midi_arranger)
        {
          midi_arranger_widget_update_visibility (
            midi_arranger);
          midi_modifier_arranger_widget_update_visibility (
            midi_modifier_arranger);
          int ret =
            midi_arranger_widget_snap_midi_notes_l (
              midi_arranger,
              &ar_prv->curr_pos, 1);
          if (!ret)
            midi_arranger_widget_snap_midi_notes_l (
              midi_arranger,
              &ar_prv->curr_pos, 0);
        }
      break;
    case UI_OVERLAY_ACTION_STRETCHING_L:
      /* TODO */
      g_message ("stretching L");
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
      if (timeline)
        {
          if (timeline->resizing_range)
            timeline_arranger_widget_snap_range_r (
              &ar_prv->curr_pos);
          else
            {
              timeline_arranger_widget_update_visibility (
                timeline);
              int ret =
                timeline_arranger_widget_snap_regions_r (
                  timeline,
                  &ar_prv->curr_pos, 1);
              if (!ret)
                timeline_arranger_widget_snap_regions_r (
                  timeline,
                  &ar_prv->curr_pos, 0);
            }
        }
      else if (midi_arranger)
        {
          midi_arranger_widget_update_visibility (
            midi_arranger);

          int ret =
            midi_arranger_widget_snap_midi_notes_r (
              midi_arranger,
              &ar_prv->curr_pos, 1);
          if (!ret)
            midi_arranger_widget_snap_midi_notes_r (
              midi_arranger,
              &ar_prv->curr_pos, 0);
        }
      break;
    case UI_OVERLAY_ACTION_STRETCHING_R:
      g_message ("stretching R");
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP:
      if (midi_modifier_arranger)
        {
          midi_modifier_arranger_widget_update_visibility (
            midi_modifier_arranger);
          midi_modifier_arranger_widget_resize_velocities (
            midi_modifier_arranger,
            offset_y);
        }
    case UI_OVERLAY_ACTION_MOVING:
    case UI_OVERLAY_ACTION_CREATING_MOVING:
      if (timeline)
        {
          timeline_arranger_widget_update_visibility (
            timeline);
          timeline_arranger_widget_move_items_x (
            timeline,
            ar_prv->adj_ticks_diff,
            F_NOT_COPY_MOVING);
          timeline_arranger_widget_move_items_y (
            timeline,
            offset_y);
        }
      else if (midi_arranger)
        {
          midi_arranger_widget_update_visibility (
            midi_arranger);
          midi_modifier_arranger_widget_update_visibility (
            midi_modifier_arranger);
          midi_arranger_widget_move_items_x (
            midi_arranger,
            ar_prv->adj_ticks_diff,
            F_NOT_COPY_MOVING);
          midi_arranger_widget_move_items_y (
            midi_arranger,
            offset_y);
          auto_scroll (self);
        }
      break;
    case UI_OVERLAY_ACTION_MOVING_COPY:
      if (timeline)
        {
          timeline_arranger_widget_update_visibility (
            timeline);
          timeline_arranger_widget_move_items_x (
            timeline,
            ar_prv->adj_ticks_diff,
            F_COPY_MOVING);
          timeline_arranger_widget_move_items_y (
            timeline,
            offset_y);
        }
      else if (midi_arranger)
        {
          midi_arranger_widget_update_visibility (
            midi_arranger);
          midi_modifier_arranger_widget_update_visibility (
            midi_modifier_arranger);
          midi_arranger_widget_move_items_x (
            midi_arranger,
            ar_prv->adj_ticks_diff,
            F_COPY_MOVING);
          midi_arranger_widget_move_items_y (
            midi_arranger,
            offset_y);
          /*auto_scroll(self);*/
        }
      break;
    case UI_OVERLAY_ACTION_AUTOFILLING:
      /* TODO */
      g_message ("autofilling");
      break;
    case UI_OVERLAY_ACTION_AUDITIONING:
      /* TODO */
      g_message ("auditioning");
      break;
    case UI_OVERLAY_ACTION_RAMPING:
      /* find and select objects inside selection */
      if (timeline)
        {
          /* TODO */
        }
      else if (midi_arranger)
        {
          /* TODO */
        }
      else if (midi_modifier_arranger)
        {
          midi_modifier_arranger_widget_ramp (
            midi_modifier_arranger,
            offset_x,
            offset_y);
        }
      else if (audio_arranger)
        {
          /* TODO */
        }
      break;
    default:
      /* TODO */
      break;
    }

  /* update last offsets */
  ar_prv->last_offset_x = offset_x;
  ar_prv->last_offset_y = offset_y;

  arranger_widget_refresh_cursor (self);
}
static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  ArrangerWidget * self =
    (ArrangerWidget *) user_data;
  GET_PRIVATE;

  GET_ARRANGER_ALIASES (self);

  if (midi_arranger)
    {
      midi_arranger_widget_on_drag_end (
        midi_arranger);
    }
  else if (timeline)
    {
      timeline_arranger_widget_on_drag_end (
        timeline);
    }
  else if (midi_modifier_arranger)
    {
      midi_modifier_arranger_widget_on_drag_end (
        midi_modifier_arranger);
    }
  else if (audio_arranger)
    {
      /* TODO */
    }

  /* if clicked on nothing */
  if (ar_prv->action ==
           UI_OVERLAY_ACTION_STARTING_SELECTION)
    {
      /* deselect all */
      arranger_widget_select_all (self, 0);
    }

  /* reset start coordinates and offsets */
  ar_prv->start_x = 0;
  ar_prv->start_y = 0;
  ar_prv->last_offset_x = 0;
  ar_prv->last_offset_y = 0;

  ar_prv->shift_held = 0;
  ar_prv->ctrl_held = 0;

  /* reset action */
  ar_prv->action = UI_OVERLAY_ACTION_NONE;

  /* queue redraw to hide the selection */
  gtk_widget_queue_draw (GTK_WIDGET (ar_prv->bg));

  arranger_widget_refresh_cursor (self);
}

int
arranger_widget_pos_to_px (
  ArrangerWidget * self,
  Position * pos,
  int        use_padding)
{
  GET_ARRANGER_ALIASES (self);

  if (timeline)
    return ui_pos_to_px_timeline (
             pos, use_padding);
  else if (midi_arranger || midi_modifier_arranger)
    return ui_pos_to_px_piano_roll (
             pos, use_padding);
  else if (audio_arranger)
    return ui_pos_to_px_audio_clip_editor (
             pos, use_padding);

  return -1;
}

/**
 * Gets the corresponding scrolled window.
 */
GtkScrolledWindow *
arranger_widget_get_scrolled_window (
  ArrangerWidget * self)
{
  GET_ARRANGER_ALIASES (self);

  if (timeline)
    return MW_CENTER_DOCK->timeline_scroll;
  else if (midi_arranger)
    return MW_PIANO_ROLL->arranger_scroll;
  else if (midi_modifier_arranger)
    return MW_PIANO_ROLL->modifier_arranger_scroll;
  else if (audio_arranger)
    return MW_AUDIO_CLIP_EDITOR->arranger_scroll;

  return NULL;
}

static gboolean
tick_cb (GtkWidget *widget,
         GdkFrameClock *frame_clock,
         gpointer user_data)
{
  ARRANGER_WIDGET_GET_PRIVATE (widget);

  gint64 frame_time =
    gdk_frame_clock_get_frame_time (frame_clock);

  if (gtk_widget_get_visible (widget) &&
      (frame_time - ar_prv->last_frame_time) >
        15000)
    {
      gtk_widget_queue_allocate (widget);
    }
  ar_prv->last_frame_time = frame_time;

  return G_SOURCE_CONTINUE;
}
#include "gui/widgets/center_dock_bot_box.h"
#include "gui/widgets/timeline_minimap.h"

static gboolean
on_scroll (GtkWidget *widget,
           GdkEventScroll  *event,
           ArrangerWidget * self)
{
  GET_ARRANGER_ALIASES (widget);
  g_message ("dx %f dy %f", event->delta_x,
             event->delta_y);
  if (!(event->state & GDK_CONTROL_MASK))
    return FALSE;

  double x = event->x,
         /*y = event->y,*/
         adj_val,
         diff;
  Position cursor_pos;
   GtkScrolledWindow * scroll =
    arranger_widget_get_scrolled_window (self);
  GtkAdjustment * adj;
  int new_x;
  RulerWidget * ruler = NULL;
  RulerWidgetPrivate * rw_prv;

  if (timeline)
    ruler = Z_RULER_WIDGET (MW_RULER);
  else if (midi_modifier_arranger ||
           midi_arranger)
    ruler = Z_RULER_WIDGET (MIDI_RULER);
  else if (audio_arranger)
    ruler = Z_RULER_WIDGET (AUDIO_RULER);

  rw_prv = ruler_widget_get_private (ruler);

  /* get current adjustment so we can get the
   * difference from the cursor */
  adj = gtk_scrolled_window_get_hadjustment (
    scroll);
  adj_val = gtk_adjustment_get_value (adj);

  /* get positions of cursor */
  arranger_widget_px_to_pos (
    self, x, &cursor_pos, 1);

  /* get px diff so we can calculate the new
   * adjustment later */
  diff = x - adj_val;

  /* scroll down, zoom out */
  if (event->delta_y > 0)
    {
      ruler_widget_set_zoom_level (
        ruler,
        rw_prv->zoom_level / 1.3f);
    }
  else /* scroll up, zoom in */
    {
      ruler_widget_set_zoom_level (
        ruler,
        rw_prv->zoom_level * 1.3f);
    }

  new_x = arranger_widget_pos_to_px (
    self, &cursor_pos, 1);

  /* refresh relevant widgets */
  if (timeline)
    timeline_minimap_widget_refresh (
      MW_TIMELINE_MINIMAP);

  /* get updated adjustment and set its value
   at the same offset as before */
  adj = gtk_scrolled_window_get_hadjustment (
    scroll);
  gtk_adjustment_set_value (adj,
                            new_x - diff);

  return TRUE;
}

static gboolean
on_motion (GtkEventControllerMotion * event,
           gdouble                    x,
           gdouble                    y,
           ArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  /*g_message ("on motion");*/

  ar_prv->hover_x = x;
  ar_prv->hover_y = y;

  arranger_widget_refresh_cursor (self);

  return FALSE;
}

static gboolean
on_focus_out (GtkWidget *widget,
               GdkEvent  *event,
               gpointer   user_data)
{
    g_message ("arranger focus out");

  return FALSE;
}

static gboolean
on_grab_broken (GtkWidget *widget,
               GdkEvent  *event,
               gpointer   user_data)
{
  GdkEventGrabBroken * ev =
    (GdkEventGrabBroken *) event;
  g_message ("arranger grab broken");
  return FALSE;
}

void
arranger_widget_setup (
  ArrangerWidget *   self,
  SnapGrid *         snap_grid)
{
  GET_PRIVATE;

  GET_ARRANGER_ALIASES (self);

  ar_prv->snap_grid = snap_grid;

  /* create and set background widgets, setup */
  if (midi_arranger)
    {
      ar_prv->bg = Z_ARRANGER_BG_WIDGET (
        midi_arranger_bg_widget_new (
          Z_RULER_WIDGET (MIDI_RULER),
          self));
      midi_arranger_widget_setup (
        midi_arranger);
    }
  else if (timeline)
    {
      ar_prv->bg = Z_ARRANGER_BG_WIDGET (
        timeline_bg_widget_new (
          Z_RULER_WIDGET (MW_RULER),
          self));
      timeline_arranger_widget_setup (
        timeline);
    }
  else if (midi_modifier_arranger)
    {
      ar_prv->bg = Z_ARRANGER_BG_WIDGET (
        midi_modifier_arranger_bg_widget_new (
          Z_RULER_WIDGET (MIDI_RULER),
          self));
      midi_modifier_arranger_widget_setup (
        midi_modifier_arranger);
    }
  else if (audio_arranger)
    {
      ar_prv->bg = Z_ARRANGER_BG_WIDGET (
        audio_arranger_bg_widget_new (
          Z_RULER_WIDGET (AUDIO_RULER),
          self));
      audio_arranger_widget_setup (
        audio_arranger);
    }
  gtk_container_add (
    GTK_CONTAINER (self),
    GTK_WIDGET (ar_prv->bg));
  gtk_widget_add_events (
    GTK_WIDGET (ar_prv->bg),
    GDK_ALL_EVENTS_MASK);


  /* add the playhead */
  ar_prv->playhead =
    arranger_playhead_widget_new ();
  gtk_overlay_add_overlay (
    GTK_OVERLAY (self),
    GTK_WIDGET (ar_prv->playhead));

  /* make the arranger able to notify */
  gtk_widget_add_events (GTK_WIDGET (self),
                         GDK_ALL_EVENTS_MASK);
  gtk_widget_set_can_focus (GTK_WIDGET (self),
                           1);
  gtk_widget_set_focus_on_click (GTK_WIDGET (self),
                                 1);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self), "get-child-position",
    G_CALLBACK (get_child_position), NULL);
  g_signal_connect (
    G_OBJECT(ar_prv->drag), "drag-begin",
    G_CALLBACK (drag_begin),  self);
  g_signal_connect (
    G_OBJECT(ar_prv->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(ar_prv->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  g_signal_connect (
    G_OBJECT (ar_prv->drag), "cancel",
    G_CALLBACK (drag_cancel), self);
  g_signal_connect (
    G_OBJECT (ar_prv->multipress), "pressed",
    G_CALLBACK (multipress_pressed), self);
  g_signal_connect (
    G_OBJECT (ar_prv->right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);
  g_signal_connect (
    G_OBJECT (self), "scroll-event",
    G_CALLBACK (on_scroll), self);
  g_signal_connect (
    G_OBJECT (self), "key-press-event",
    G_CALLBACK (on_key_action), self);
  g_signal_connect (
    G_OBJECT (self), "key-release-event",
    G_CALLBACK (on_key_release_action), self);
  g_signal_connect (
    G_OBJECT (ar_prv->motion_controller), "motion",
    G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT (self), "focus-out-event",
    G_CALLBACK (on_focus_out), self);
  g_signal_connect (
    G_OBJECT (self), "grab-broken-event",
    G_CALLBACK (on_grab_broken), self);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), tick_cb,
    NULL, NULL);
}

/**
 * Wrapper for ui_px_to_pos depending on the arranger
 * type.
 */
void
arranger_widget_px_to_pos (
  ArrangerWidget * self,
  double           px,
  Position *       pos,
  int              has_padding)
{
  GET_ARRANGER_ALIASES (self);

  if (timeline)
    ui_px_to_pos_timeline (
      px, pos, has_padding);
  else if (midi_arranger)
    ui_px_to_pos_piano_roll (
      px, pos, has_padding);
  else if (midi_modifier_arranger)
    ui_px_to_pos_piano_roll (
      px, pos, has_padding);
  else if (audio_arranger)
    ui_px_to_pos_audio_clip_editor (
      px, pos, has_padding);
}

void
arranger_widget_refresh_cursor (
  ArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  GET_ARRANGER_ALIASES (self);

  if (!gtk_widget_get_realized (
        GTK_WIDGET (self)))
    return;

  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;

  if (timeline)
    {
      ac =
        timeline_arranger_widget_get_cursor (
          timeline,
          ar_prv->action,
          P_TOOL);
    }
  if (midi_arranger)
    {
      ac =
        midi_arranger_widget_get_cursor (
          midi_arranger,
          ar_prv->action,
          P_TOOL);
    }
  if (audio_arranger)
    {
      ac =
        audio_arranger_widget_get_cursor (
          audio_arranger,
          ar_prv->action,
          P_TOOL);
    }
  if (midi_modifier_arranger)
    {
      ac =
        midi_modifier_arranger_widget_get_cursor (
          midi_modifier_arranger,
          ar_prv->action,
          P_TOOL);
    }

  arranger_widget_set_cursor (
    self, ac);
}

/**
 * Readd children.
 */
int
arranger_widget_refresh (
  ArrangerWidget * self)
{
  arranger_widget_set_cursor (
    self, ARRANGER_CURSOR_SELECT);
  ARRANGER_WIDGET_GET_PRIVATE (self);

  GET_ARRANGER_ALIASES (self);

  if (midi_arranger)
    {
      /*RULER_WIDGET_GET_PRIVATE (MIDI_RULER);*/
      /*gtk_widget_set_size_request (*/
        /*GTK_WIDGET (self),*/
        /*rw_prv->total_px,*/
        /*gtk_widget_get_allocated_height (*/
          /*GTK_WIDGET (self)));*/
      midi_arranger_widget_refresh_children (
        midi_arranger);
    }
  else if (timeline)
    {
      timeline_arranger_widget_set_size (
        timeline);
      timeline_arranger_widget_refresh_children (
        timeline);
    }
  else if (midi_modifier_arranger)
    {
      RULER_WIDGET_GET_PRIVATE (MIDI_RULER);
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        rw_prv->total_px,
        -1);
      midi_modifier_arranger_widget_refresh_children (
        midi_modifier_arranger);
    }
  else if (audio_arranger)
    {
      RULER_WIDGET_GET_PRIVATE (AUDIO_RULER);
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        rw_prv->total_px,
        -1);
      audio_arranger_widget_refresh_children (
        audio_arranger);

    }

	if (ar_prv->bg)
	{
		arranger_bg_widget_refresh (ar_prv->bg);
		arranger_widget_refresh_cursor (self);
	}

	return FALSE;
}

static void
arranger_widget_class_init (ArrangerWidgetClass * _klass)
{
}

static void
arranger_widget_init (ArrangerWidget *self)
{
  GET_PRIVATE;

  ar_prv->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new (GTK_WIDGET (self)));
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (ar_prv->drag),
    GTK_PHASE_CAPTURE);
  ar_prv->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (ar_prv->multipress),
    GTK_PHASE_CAPTURE);
  ar_prv->right_mouse_mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (
      ar_prv->right_mouse_mp),
      GDK_BUTTON_SECONDARY);
  ar_prv->motion_controller =
    gtk_event_controller_motion_new (
      GTK_WIDGET (self));
}
