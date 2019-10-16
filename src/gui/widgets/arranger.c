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
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_arranger_bg.h"
#include "gui/widgets/automation_curve.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_arranger_bg.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_modifier_arranger_bg.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_bot_box.h"
#include "gui/widgets/timeline_minimap.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "gui/widgets/velocity.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/gtk.h"
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
  TimelineArrangerWidget * timeline_arranger = \
    NULL; \
  MidiArrangerWidget * midi_arranger = NULL; \
  MidiModifierArrangerWidget * \
    midi_modifier_arranger = NULL; \
  AudioArrangerWidget * audio_arranger = NULL; \
  ChordArrangerWidget * chord_arranger = NULL; \
  AutomationArrangerWidget * automation_arranger = \
    NULL; \
  if (ARRANGER_IS_MIDI (self)) \
    { \
      midi_arranger = Z_MIDI_ARRANGER_WIDGET (self); \
    } \
  else if (ARRANGER_IS_TIMELINE (self)) \
    { \
      timeline_arranger = \
        Z_TIMELINE_ARRANGER_WIDGET (self); \
    } \
  else if (Z_IS_AUDIO_ARRANGER_WIDGET (self)) \
    { \
      audio_arranger = \
        Z_AUDIO_ARRANGER_WIDGET (self); \
    } \
  else if (ARRANGER_IS_MIDI_MODIFIER (self)) \
    { \
      midi_modifier_arranger = \
        Z_MIDI_MODIFIER_ARRANGER_WIDGET (self); \
    } \
  else if (Z_IS_CHORD_ARRANGER_WIDGET (self)) \
    { \
      chord_arranger = \
        Z_CHORD_ARRANGER_WIDGET (self); \
    } \
  else if (Z_IS_AUTOMATION_ARRANGER_WIDGET (self)) \
    { \
      automation_arranger = \
        Z_AUTOMATION_ARRANGER_WIDGET (self); \
    }

#define FORALL_ARRANGERS(func) \
  func (timeline); \
  func (midi); \
  func (audio); \
  func (automation); \
  func (midi_modifier); \
  func (chord); \

#define ACTION_IS(x) \
  (ar_prv->action == UI_OVERLAY_ACTION_##x)

ArrangerWidgetPrivate *
arranger_widget_get_private (ArrangerWidget * self)
{
  return arranger_widget_get_instance_private (self);
}

/**
 * Gets called to set the position/size of each overlayed widget.
 */
static gboolean
arranger_get_child_position (
  GtkOverlay   *overlay,
  GtkWidget    *widget,
  GdkRectangle *allocation,
  ArrangerWidget * self)
{
  GET_ARRANGER_ALIASES (self);

#define ELIF_ARR_SET_ALLOCATION(sc) \
  if (sc##_arranger) \
    { \
      sc##_arranger_widget_set_allocation ( \
        sc##_arranger, widget, allocation); \
    }

  if (Z_IS_ARRANGER_PLAYHEAD_WIDGET (widget))
    {
      /* note: -1 (half the width of the playhead
       * widget */
      if (timeline_arranger)
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
            ui_pos_to_px_editor (
              &TRANSPORT->playhead_pos,
              1);
        }
      allocation->y = 0;
      allocation->width = 2;
      allocation->height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (self));
    }
  FORALL_ARRANGERS (ELIF_ARR_SET_ALLOCATION)
#undef ELIF_ARR_SET_ALLOCATION

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
      SET_X_CURSOR (left_resize);
      break;
    case ARRANGER_CURSOR_RESIZING_L_LOOP:
      SET_X_CURSOR (left_resize_loop);
      break;
    case ARRANGER_CURSOR_RESIZING_R:
      SET_X_CURSOR (right_resize);
      break;
    case ARRANGER_CURSOR_RESIZING_R_LOOP:
      SET_X_CURSOR (right_resize_loop);
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
                (int) start_x,
                (int) start_y,
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
  g_list_free (children);
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
    sc##_select ( \
      (cc *)child, select);

  if (timeline_arranger)
    {
      FIND_ARRAY_AND_NUM (
        REGION, region, TL_SELECTIONS);
      FIND_ARRAY_AND_NUM (
        SCALE_OBJECT, scale_object, TL_SELECTIONS);
      FIND_ARRAY_AND_NUM (
        MARKER, marker, TL_SELECTIONS);
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
  if (chord_arranger)
    {
      FIND_ARRAY_AND_NUM (
        CHORD_OBJECT, chord_object,
        CHORD_SELECTIONS);
    }
  if (automation_arranger)
    {
      FIND_ARRAY_AND_NUM (
        AUTOMATION_POINT, automation_point,
        AUTOMATION_SELECTIONS);
    }

  if (select && !append)
    {
      /* deselect existing selections */
      if (timeline_arranger)
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

  g_return_if_fail (array || num);

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
      (ArrangerWidget *) (MW_TIMELINE));
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
  ar_prv =
    arranger_widget_get_private (
      (ArrangerWidget *) (MW_MIDI_ARRANGER));
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
  ar_prv =
    arranger_widget_get_private (
      (ArrangerWidget *) (MW_MIDI_MODIFIER_ARRANGER));
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
  ar_prv =
    arranger_widget_get_private (
      (ArrangerWidget *) (MW_AUDIO_ARRANGER));
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
}

void
arranger_widget_select_all (
  ArrangerWidget *  self,
  int               select)
{
  GET_ARRANGER_ALIASES (self);

#define ARR_SELECT_ALL(sc) \
  if (sc##_arranger) \
    { \
      sc##_arranger_widget_select_all ( \
        sc##_arranger, select); \
    }
  FORALL_ARRANGERS (ARR_SELECT_ALL);

#undef ARR_SELECT_ALL
}

static void
show_context_menu (
  ArrangerWidget * self,
  gdouble          x,
  gdouble          y)
{
  GET_ARRANGER_ALIASES (self);

#define SHOW_CONTEXT_MENU(sc) \
  if (sc##_arranger) \
    sc##_arranger_widget_show_context_menu ( \
      sc##_arranger, x, y)

  FORALL_ARRANGERS (SHOW_CONTEXT_MENU);

#undef SHOW_CONTEXT_MENU
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
auto_scroll (
  ArrangerWidget * self,
  int              x,
  int              y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* figure out if we should scroll */
  int scroll_h = 0, scroll_v = 0;
  switch (ar_prv->action)
    {
    case UI_OVERLAY_ACTION_MOVING:
    case UI_OVERLAY_ACTION_MOVING_COPY:
    case UI_OVERLAY_ACTION_CREATING_MOVING:
    case UI_OVERLAY_ACTION_SELECTING:
    case UI_OVERLAY_ACTION_RAMPING:
      scroll_h = 1;
      scroll_v = 1;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
    case UI_OVERLAY_ACTION_RESIZING_L:
    case UI_OVERLAY_ACTION_STRETCHING_L:
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
    case UI_OVERLAY_ACTION_STRETCHING_R:
    case UI_OVERLAY_ACTION_AUTOFILLING:
    case UI_OVERLAY_ACTION_AUDITIONING:
      scroll_h = 1;
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP:
      scroll_v = 1;
      break;
    default:
      break;
    }

  if (!scroll_h && !scroll_v)
    return;

	GtkScrolledWindow *scroll =
		arranger_widget_get_scrolled_window (self);
  g_return_if_fail (scroll);
  int h_scroll_speed = 20;
  int v_scroll_speed = 10;
  int border_distance = 5;
  int scroll_width =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (scroll));
  int scroll_height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (scroll));
  GtkAdjustment *hadj =
    gtk_scrolled_window_get_hadjustment (
      GTK_SCROLLED_WINDOW (scroll));
  GtkAdjustment *vadj =
    gtk_scrolled_window_get_vadjustment (
      GTK_SCROLLED_WINDOW (scroll));
  int v_delta = 0;
  int h_delta = 0;
  int adj_x =
    (int)
    gtk_adjustment_get_value (hadj);
  int adj_y =
    (int)
    gtk_adjustment_get_value (vadj);
  if (y + border_distance
        >= adj_y + scroll_height)
    {
      v_delta = v_scroll_speed;
    }
  else if (y - border_distance <= adj_y)
    {
      v_delta = - v_scroll_speed;
    }
  if (x + border_distance >= adj_x + scroll_width)
    {
      h_delta = h_scroll_speed;
    }
  else if (x - border_distance <= adj_x)
    {
      h_delta = - h_scroll_speed;
    }
  if (h_delta != 0 && scroll_h)
  {
    gtk_adjustment_set_value (
      hadj,
      gtk_adjustment_get_value (hadj) + h_delta);
  }
  if (v_delta != 0 && scroll_v)
  {
    gtk_adjustment_set_value (
      vadj,
      gtk_adjustment_get_value (vadj) + v_delta);
  }

  return;
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

  const guint keyval = event->keyval;

  if (z_gtk_keyval_is_ctrl (keyval))
    {
      ar_prv->ctrl_held = 0;
    }

  if (z_gtk_keyval_is_shift (keyval))
    {
      ar_prv->shift_held = 0;
    }
  if (z_gtk_keyval_is_alt (keyval))
    {
      ar_prv->alt_held = 0;
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

  if (timeline_arranger)
    {
      timeline_arranger_widget_set_cut_lines_visible (
        timeline_arranger);
    }


#define UPDATE_VISIBILITY(sc) \
  if (sc##_arranger) \
    sc##_arranger_widget_update_visibility ( \
      sc##_arranger)

  FORALL_ARRANGERS (UPDATE_VISIBILITY);

#undef UPDATE_VISIBILITY

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

  const guint keyval = event->keyval;

  if (z_gtk_keyval_is_ctrl (keyval))
    {
      ar_prv->ctrl_held = 1;
    }

  if (z_gtk_keyval_is_shift (keyval))
    {
      ar_prv->shift_held = 1;
    }
  if (z_gtk_keyval_is_alt (keyval))
    {
      ar_prv->alt_held = 1;
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
  if (timeline_arranger)
    {
    }
  if (midi_modifier_arranger)
    {

    }
  if (audio_arranger)
    {

    }
  if (chord_arranger)
    {}
  if (automation_arranger)
    {}

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

  if (timeline_arranger)
    {
      timeline_arranger_widget_set_cut_lines_visible (
        timeline_arranger);
    }

#define UPDATE_VISIBILITY(sc) \
  if (sc##_arranger) \
    sc##_arranger_widget_update_visibility ( \
      sc##_arranger)

  FORALL_ARRANGERS (UPDATE_VISIBILITY);

#undef UPDATE_VISIBILITY

  arranger_widget_refresh_cursor (
    self);

  /*if (num > 0)*/
    /*auto_scroll (self);*/

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
  GdkModifierType state_mask =
    ui_get_state_mask (
      GTK_GESTURE (gesture));
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
  int note, chord_index;
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

  if (timeline_arranger)
    {
      /* figure out if we are creating a region or
       * automation point */
      at =
        timeline_arranger_widget_get_automation_track_at_y (
          timeline_arranger, start_y);
      if (!at)
        track =
          timeline_arranger_widget_get_track_at_y (
            timeline_arranger, start_y);

      /* creating automation point */
      if (at)
        {
          timeline_arranger_widget_create_region (
            timeline_arranger,
            REGION_TYPE_AUTOMATION, track, NULL, at,
            &pos);
        }
      /* double click inside a track */
      else if (track)
        {
          TrackLane * lane =
            timeline_arranger_widget_get_track_lane_at_y (
              timeline_arranger, start_y);
          switch (track->type)
            {
            case TRACK_TYPE_INSTRUMENT:
              timeline_arranger_widget_create_region (
                timeline_arranger,
                REGION_TYPE_MIDI, track,
                lane, NULL, &pos);
              break;
            case TRACK_TYPE_MIDI:
              timeline_arranger_widget_create_region (
                timeline_arranger,
                REGION_TYPE_MIDI, track,
                lane, NULL, &pos);
              break;
            case TRACK_TYPE_AUDIO:
              break;
            case TRACK_TYPE_MASTER:
              break;
            case TRACK_TYPE_CHORD:
              timeline_arranger_widget_create_chord_or_scale (
                timeline_arranger, track,
                start_y, &pos);
            case TRACK_TYPE_AUDIO_BUS:
              break;
            case TRACK_TYPE_AUDIO_GROUP:
              break;
            case TRACK_TYPE_MARKER:
              timeline_arranger_widget_create_marker (
                timeline_arranger, track, &pos);
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
  if (chord_arranger)
    {
      /* find the chord and region at x,y */
      chord_index =
        chord_arranger_widget_get_chord_at_y (
          start_y);
      region =
        CLIP_EDITOR->region;

      /* create a chord object */
      if (region)
        {
          chord_arranger_widget_create_chord (
            chord_arranger, &pos, chord_index,
            region);
        }
    }
  if (automation_arranger)
    {
      region =
        CLIP_EDITOR->region;

      if (region)
        {
          automation_arranger_widget_create_ap (
            automation_arranger, &pos, start_y,
            region);
        }
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

/**
 * Sets the start pos of the earliest object and
 * the flag whether the earliest object exists.
 */
static void
set_earliest_obj (
  ArrangerWidget * self)
{
  GET_PRIVATE;

  position_set_to_bar (
    &ar_prv->earliest_obj_start_pos, 2000);

#define _SET_WITH_GLOBAL( \
  _arranger, _selections, _selections_name) \
  else if ( \
    _arranger && \
    _selections_name##_selections_has_any ( \
    _selections)) \
    { \
      _selections_name##_selections_get_start_pos ( \
        _selections, \
        &ar_prv->earliest_obj_start_pos, \
        0, 1); \
      _selections_name##_selections_set_cache_poses ( \
        _selections); \
      ar_prv->earliest_obj_exists = 1; \
    }

  GET_ARRANGER_ALIASES (self);
  if (timeline_arranger &&
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
  _SET_WITH_GLOBAL (
    midi_arranger, MA_SELECTIONS, midi_arranger)
  _SET_WITH_GLOBAL (
    chord_arranger, CHORD_SELECTIONS, chord)
  _SET_WITH_GLOBAL (
    automation_arranger, AUTOMATION_SELECTIONS,
    automation)
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
  else if (audio_arranger)
    {
      g_warn_if_reached ();
      /* TODO */
    }
  else
    {
      ar_prv->earliest_obj_exists = 0;
    }

#undef _SET_WITH_GLOBAL
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

  /* get current pos */
  arranger_widget_px_to_pos (
    self, ar_prv->start_x,
    &ar_prv->curr_pos, 1);

  /* get difference with drag start pos */
  ar_prv->curr_ticks_diff_from_start =
    position_get_ticks_diff (
      &ar_prv->curr_pos,
      &ar_prv->start_pos,
      NULL);

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
        midi_arranger_widget_get_hit_note (
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
  else if (timeline_arranger)
    {
      rw =
        timeline_arranger_widget_get_hit_region (
          timeline_arranger, start_x, start_y);
      scale_widget =
        timeline_arranger_widget_get_hit_scale (
          timeline_arranger, start_x, start_y);
      mw =
        timeline_arranger_widget_get_hit_marker (
          timeline_arranger, start_x, start_y);
    }
  else if (automation_arranger)
    {
      ac_widget =
        automation_arranger_widget_get_hit_ac (
          automation_arranger, start_x, start_y);
      ap_widget =
        automation_arranger_widget_get_hit_ap (
          automation_arranger, start_x, start_y);
    }
  else if (chord_arranger)
    {
      chord_widget =
        chord_arranger_widget_get_hit_chord (
          chord_arranger, start_x, start_y);
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
            midi_arranger, start_x, start_y,
            midi_note_widget);
        }
      else if (rw)
        {
          timeline_arranger_widget_on_drag_begin_region_hit (
            timeline_arranger, start_x, start_y, rw);
        }
      else if (chord_widget)
        {
          chord_arranger_widget_on_drag_begin_chord_hit (
            chord_arranger, start_x,
            start_y, chord_widget);
        }
      else if (scale_widget)
        {
          timeline_arranger_widget_on_drag_begin_scale_hit (
            timeline_arranger, start_x,
            start_y, scale_widget);
        }
      else if (ap_widget)
        {
          automation_arranger_widget_on_drag_begin_ap_hit (
            automation_arranger, start_x,
            start_y, ap_widget);
        }
      else if (ac_widget)
        {
          automation_arranger->start_ac =
            ac_widget->ac;
        }
      else if (vel_widget)
        {
          midi_modifier_arranger_widget_on_drag_begin_velocity_hit (
            midi_modifier_arranger, start_x, start_y,
            vel_widget);
        }
      else if (mw)
        {
          timeline_arranger_widget_on_drag_begin_marker_hit (
            timeline_arranger, start_x,
            start_y, mw);
        }
    }
  else /* nothing hit */
    {
      /* single click */
      if (ar_prv->n_press == 1)
        {
          switch (P_TOOL)
            {
            case TOOL_SELECT_NORMAL:
            case TOOL_SELECT_STRETCH:
              /* selection */
              ar_prv->action =
                UI_OVERLAY_ACTION_STARTING_SELECTION;

              /* deselect all */
              arranger_widget_select_all (self, 0);

              /* if timeline, set either selecting
               * objects or selecting range */
              if (timeline_arranger)
                {
                  timeline_arranger_widget_set_select_type (
                    timeline_arranger, start_y);
                }
              break;
            case TOOL_EDIT:
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
              break;
            case TOOL_ERASER:
              /* delete selection */
              ar_prv->action =
                UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION;
              break;
            case TOOL_RAMP:
              ar_prv->action =
                UI_OVERLAY_ACTION_STARTING_RAMP;
              break;
            default:
              break;
            }
        }
      /* double click */
      else if (ar_prv->n_press == 2)
        {
          switch (P_TOOL)
            {
            case TOOL_SELECT_NORMAL:
            case TOOL_SELECT_STRETCH:
            case TOOL_EDIT:
              create_item (
                self, start_x, start_y);
              break;
            case TOOL_ERASER:
              /* delete selection */
              ar_prv->action =
                UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION;
              break;
            default:
              break;
            }
        }
    }

  /* set the start pos of the earliest object and
   * the flag whether the earliest object exists */
  set_earliest_obj (self);

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
  GdkModifierType state_mask =
    ui_get_state_mask (
      GTK_GESTURE (gesture));
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
  switch (ar_prv->action)
    {
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
      ar_prv->action =
        UI_OVERLAY_ACTION_SELECTING;
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
      ar_prv->action =
        UI_OVERLAY_ACTION_DELETE_SELECTING;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
      if (ar_prv->ctrl_held)
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING_COPY;
      else
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING;
      break;
    case UI_OVERLAY_ACTION_MOVING:
      if (ar_prv->ctrl_held)
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING_COPY;
      break;
    case UI_OVERLAY_ACTION_MOVING_COPY:
      if (!ar_prv->ctrl_held)
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING;
      break;
    case UI_OVERLAY_ACTION_STARTING_RAMP:
      ar_prv->action =
        UI_OVERLAY_ACTION_RAMPING;
      break;
    default:
      break;
    }

#define MOVE_ITEMS(_arranger,_copy_moving) \
      if (_arranger) \
        { \
          _arranger##_widget_update_visibility ( \
            _arranger); \
          if ((MidiArrangerWidget *) _arranger == \
              midi_arranger) \
            { \
              midi_modifier_arranger_widget_update_visibility ( \
                midi_modifier_arranger); \
            } \
          _arranger##_widget_move_items_x ( \
            _arranger, \
            ar_prv->adj_ticks_diff, \
            _copy_moving); \
          _arranger##_widget_move_items_y ( \
            _arranger, \
            offset_y); \
        }

  /* if drawing a selection */
  switch (ar_prv->action)
    {
    case UI_OVERLAY_ACTION_SELECTING:
      /* find and select objects inside selection */
#define DO_SELECT(sc) \
  if (sc##_arranger) \
    { \
      sc##_arranger_widget_select ( \
        sc##_arranger, offset_x, offset_y, \
        F_NO_DELETE); \
    }
      FORALL_ARRANGERS (DO_SELECT);
#undef DO_SELECT
      EVENTS_PUSH (
        ET_SELECTING_IN_ARRANGER, self);
      break;
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
      /* find and delete objects inside
       * selection */
#define DO_SELECT(sc) \
  if (sc##_arranger) \
    { \
      sc##_arranger_widget_select ( \
        sc##_arranger, offset_x, offset_y, \
        F_DELETE); \
    }
      FORALL_ARRANGERS (DO_SELECT);
#undef DO_SELECT
      EVENTS_PUSH (
        ET_SELECTING_IN_ARRANGER, self);
      break;
    case UI_OVERLAY_ACTION_RESIZING_L_LOOP:
      /* snap selections based on new pos */
      if (timeline_arranger)
        {
          timeline_arranger_widget_update_visibility (
            timeline_arranger);
          int ret =
            timeline_arranger_widget_snap_regions_l (
              timeline_arranger,
              &ar_prv->curr_pos, 1);
          if (!ret)
            timeline_arranger_widget_snap_regions_l (
              timeline_arranger,
              &ar_prv->curr_pos, 0);
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      /* snap selections based on new pos */
      if (timeline_arranger)
        {
          timeline_arranger_widget_update_visibility (
            timeline_arranger);
          int ret =
            timeline_arranger_widget_snap_regions_l (
              timeline_arranger,
              &ar_prv->curr_pos, 1);
          if (!ret)
            timeline_arranger_widget_snap_regions_l (
              timeline_arranger,
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
    case UI_OVERLAY_ACTION_RESIZING_R_LOOP:
      if (timeline_arranger)
        {
          if (timeline_arranger->resizing_range)
            timeline_arranger_widget_snap_range_r (
              &ar_prv->curr_pos);
          else
            {
              timeline_arranger_widget_update_visibility (
                timeline_arranger);
              int ret =
                timeline_arranger_widget_snap_regions_r (
                  timeline_arranger,
                  &ar_prv->curr_pos, 1);
              if (!ret)
                timeline_arranger_widget_snap_regions_r (
                  timeline_arranger,
                  &ar_prv->curr_pos, 0);
            }
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
      if (timeline_arranger)
        {
          if (timeline_arranger->resizing_range)
            timeline_arranger_widget_snap_range_r (
              &ar_prv->curr_pos);
          else
            {
              timeline_arranger_widget_update_visibility (
                timeline_arranger);
              int ret =
                timeline_arranger_widget_snap_regions_r (
                  timeline_arranger,
                  &ar_prv->curr_pos, 1);
              if (!ret)
                timeline_arranger_widget_snap_regions_r (
                  timeline_arranger,
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
      break;
    case UI_OVERLAY_ACTION_MOVING:
    case UI_OVERLAY_ACTION_CREATING_MOVING:
      MOVE_ITEMS (
        timeline_arranger, F_NOT_COPY_MOVING);
      MOVE_ITEMS (
        midi_arranger, F_NOT_COPY_MOVING);
      MOVE_ITEMS (
        automation_arranger, F_NOT_COPY_MOVING);
      MOVE_ITEMS (
        chord_arranger, F_NOT_COPY_MOVING);
      break;
    case UI_OVERLAY_ACTION_MOVING_COPY:
      MOVE_ITEMS (
        timeline_arranger, F_COPY_MOVING);
      MOVE_ITEMS (
        midi_arranger, F_COPY_MOVING);
      MOVE_ITEMS (
        automation_arranger, F_COPY_MOVING);
      MOVE_ITEMS (
        chord_arranger, F_COPY_MOVING);
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
      if (timeline_arranger)
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
    case UI_OVERLAY_ACTION_CUTTING:
      /* nothing to do, wait for drag end */
      break;
    default:
      /* TODO */
      break;
    }
#undef MOVE_ITEMS

  if (ar_prv->action != UI_OVERLAY_ACTION_NONE)
    auto_scroll (
      self, (int) (ar_prv->start_x + offset_x),
      (int) (ar_prv->start_y + offset_y));

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

  if (ACTION_IS (SELECTING) ||
      ACTION_IS (DELETE_SELECTING))
    {
      EVENTS_PUSH (
        ET_SELECTING_IN_ARRANGER, self);
    }

  GET_ARRANGER_ALIASES (self);

#define ON_DRAG_END(sc) \
  if (sc##_arranger) \
    sc##_arranger_widget_on_drag_end ( \
      sc##_arranger)

  FORALL_ARRANGERS (ON_DRAG_END);

#undef ON_DRAG_END

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

  if (timeline_arranger)
    return ui_pos_to_px_timeline (
             pos, use_padding);
  else if (midi_arranger ||
           midi_modifier_arranger ||
           audio_arranger ||
           automation_arranger ||
           chord_arranger)
    return ui_pos_to_px_editor (
             pos, use_padding);

  return -1;
}

/**
 * Causes a redraw of the background.
 */
void
arranger_widget_redraw_bg (
  ArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  ARRANGER_BG_WIDGET_GET_PRIVATE (ar_prv->bg);
  ab_prv->redraw = 1;
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
}

/**
 * Gets the corresponding scrolled window.
 */
GtkScrolledWindow *
arranger_widget_get_scrolled_window (
  ArrangerWidget * self)
{
  GET_ARRANGER_ALIASES (self);

  if (timeline_arranger)
    return MW_TIMELINE_PANEL->timeline_scroll;
  else if (midi_arranger)
    return MW_MIDI_EDITOR_SPACE->arranger_scroll;
  else if (midi_modifier_arranger)
    return MW_MIDI_EDITOR_SPACE->
      modifier_arranger_scroll;
  else if (audio_arranger)
    return MW_AUDIO_EDITOR_SPACE->arranger_scroll;
  else if (chord_arranger)
    return MW_CHORD_EDITOR_SPACE->arranger_scroll;
  else if (automation_arranger)
    return MW_AUTOMATION_EDITOR_SPACE->
      arranger_scroll;

  return NULL;
}

static gboolean
arranger_tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  gpointer        user_data)
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

static gboolean
on_scroll (
  GtkWidget *widget,
  GdkEventScroll  *event,
  ArrangerWidget * self)
{
  arranger_widget_redraw_bg (self);

  GET_ARRANGER_ALIASES (self);
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

  if (timeline_arranger)
    ruler = Z_RULER_WIDGET (MW_RULER);
  else if (midi_modifier_arranger ||
           midi_arranger ||
           audio_arranger ||
           chord_arranger ||
           automation_arranger)
    ruler = Z_RULER_WIDGET (EDITOR_RULER);

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
        rw_prv->zoom_level / 1.3);
    }
  else /* scroll up, zoom in */
    {
      ruler_widget_set_zoom_level (
        ruler,
        rw_prv->zoom_level * 1.3);
    }

  new_x = arranger_widget_pos_to_px (
    self, &cursor_pos, 1);

  /* refresh relevant widgets */
  if (timeline_arranger)
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

/**
 * Motion callback.
 */
static gboolean
on_motion (
  GtkEventControllerMotion * event,
  gdouble                    x,
  gdouble                    y,
  ArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  GET_ARRANGER_ALIASES (self);

  ar_prv->hover_x = x;
  ar_prv->hover_y = y;

  GdkModifierType state;
  int has_state =
    gtk_get_current_event_state (&state);
  if (has_state)
    {
      ar_prv->alt_held =
        state & GDK_MOD1_MASK;
      ar_prv->ctrl_held =
        state & GDK_CONTROL_MASK;
      ar_prv->shift_held =
        state & GDK_SHIFT_MASK;
    }

  arranger_widget_refresh_cursor (self);

  if (timeline_arranger)
    {
      timeline_arranger_widget_set_cut_lines_visible (
        timeline_arranger);
    }
  else if (automation_arranger)
    {
    }
  else if (chord_arranger)
    {
    }
  else if (audio_arranger)
    {
    }
  else if (midi_modifier_arranger)
    {
    }
  else if (midi_arranger)
    {
    }

  return FALSE;
}

static gboolean
on_focus_out (GtkWidget *widget,
               GdkEvent  *event,
               ArrangerWidget * self)
{
  g_message ("arranger focus out");
  ARRANGER_WIDGET_GET_PRIVATE (self);

  ar_prv->alt_held = 0;
  ar_prv->ctrl_held = 0;
  ar_prv->shift_held = 0;

  return FALSE;
}

static gboolean
on_grab_broken (GtkWidget *widget,
               GdkEvent  *event,
               gpointer   user_data)
{
  /*GdkEventGrabBroken * ev =*/
    /*(GdkEventGrabBroken *) event;*/
  g_warning ("arranger grab broken");
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
      ar_prv->bg = (ArrangerBgWidget *) (
        midi_arranger_bg_widget_new (
          Z_RULER_WIDGET (EDITOR_RULER),
          self));
      midi_arranger_widget_setup (
        midi_arranger);
    }
  else if (timeline_arranger)
    {
      ar_prv->bg = (ArrangerBgWidget *) (
        timeline_bg_widget_new (
          Z_RULER_WIDGET (MW_RULER),
          self));
      timeline_arranger_widget_setup (
        timeline_arranger);
    }
  else if (midi_modifier_arranger)
    {
      ar_prv->bg = (ArrangerBgWidget *) (
        midi_modifier_arranger_bg_widget_new (
          Z_RULER_WIDGET (EDITOR_RULER),
          self));
      midi_modifier_arranger_widget_setup (
        midi_modifier_arranger);
    }
  else if (audio_arranger)
    {
      ar_prv->bg = (ArrangerBgWidget *) (
        audio_arranger_bg_widget_new (
          Z_RULER_WIDGET (EDITOR_RULER),
          self));
      audio_arranger_widget_setup (
        audio_arranger);
    }
  else if (chord_arranger)
    {
      ar_prv->bg = (ArrangerBgWidget *) (
        chord_arranger_bg_widget_new (
          Z_RULER_WIDGET (EDITOR_RULER),
          self));
      chord_arranger_widget_setup (
        chord_arranger);
    }
  else if (automation_arranger)
    {
      ar_prv->bg = (ArrangerBgWidget *) (
        automation_arranger_bg_widget_new (
          Z_RULER_WIDGET (EDITOR_RULER),
          self));
      automation_arranger_widget_setup (
        automation_arranger);
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
    G_CALLBACK (arranger_get_child_position), self);
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
    GTK_WIDGET (self), arranger_tick_cb,
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

  if (timeline_arranger)
    ui_px_to_pos_timeline (
      px, pos, has_padding);
  else if (midi_arranger ||
           midi_modifier_arranger ||
           chord_arranger ||
           audio_arranger ||
           automation_arranger)
    ui_px_to_pos_editor (
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

#define GET_CURSOR(sc) \
  if (sc##_arranger) \
    ac = \
      sc##_arranger_widget_get_cursor ( \
        sc##_arranger, ar_prv->action, \
        P_TOOL)

  FORALL_ARRANGERS (GET_CURSOR);

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
      midi_arranger_widget_set_size (
        midi_arranger);
      midi_arranger_widget_refresh_children (
        midi_arranger);
    }
  else if (timeline_arranger)
    {
      timeline_arranger_widget_set_size (
        timeline_arranger);
      timeline_arranger_widget_refresh_children (
        timeline_arranger);
    }
  else if (midi_modifier_arranger)
    {
      RULER_WIDGET_GET_PRIVATE (EDITOR_RULER);
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        (int) rw_prv->total_px,
        -1);
      midi_modifier_arranger_widget_refresh_children (
        midi_modifier_arranger);
    }
  else if (audio_arranger)
    {
      RULER_WIDGET_GET_PRIVATE (EDITOR_RULER);
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        (int) rw_prv->total_px,
        -1);
      audio_arranger_widget_refresh_children (
        audio_arranger);
    }
  else if (chord_arranger)
    {
      RULER_WIDGET_GET_PRIVATE (EDITOR_RULER);
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        (int) rw_prv->total_px,
        -1);
      chord_arranger_widget_refresh_children (
        chord_arranger);
    }
  else if (automation_arranger)
    {
      RULER_WIDGET_GET_PRIVATE (EDITOR_RULER);
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        (int) rw_prv->total_px,
        -1);
      automation_arranger_widget_refresh_children (
        automation_arranger);
    }

	if (ar_prv->bg)
	{
		arranger_bg_widget_refresh (ar_prv->bg);
		arranger_widget_refresh_cursor (self);
	}

	return FALSE;
}

static void
arranger_widget_class_init (
  ArrangerWidgetClass * _klass)
{
}

static void
arranger_widget_init (
  ArrangerWidget *self)
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
    GTK_EVENT_CONTROLLER_MOTION (
      gtk_event_controller_motion_new (
        GTK_WIDGET (self)));
}
