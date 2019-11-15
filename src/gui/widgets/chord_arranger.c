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

/**
 * \file
 *
 * The chord containing regions and other
 * objects.
 */

#include "actions/arranger_selections.h"
#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/audio_track.h"
#include "audio/audio_bus_track.h"
#include "audio/channel.h"
#include "audio/chord_object.h"
#include "audio/chord_region.h"
#include "audio/chord_track.h"
#include "audio/instrument_track.h"
#include "audio/marker_track.h"
#include "audio/master_track.h"
#include "audio/midi_region.h"
#include "audio/mixer.h"
#include "audio/scale_object.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/region.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_arranger_bg.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (ChordArrangerWidget,
               chord_arranger_widget,
               ARRANGER_WIDGET_TYPE)

/**
 * To be called from get_child_position in parent widget.
 *
 * Used to allocate the overlay children.
 */
void
chord_arranger_widget_set_allocation (
  ChordArrangerWidget * self,
  GtkWidget *          widget,
  GdkRectangle *       allocation)
{
  if (Z_IS_CHORD_OBJECT_WIDGET (widget))
    {
      ChordObjectWidget * cw =
        Z_CHORD_OBJECT_WIDGET (widget);
      ChordObject * co =
        cw->chord_object;
      ChordDescriptor * descr =
        CHORD_EDITOR->chords[co->index];

      /* use transient or non transient region
       * depending on which is visible */
      Region * region = co->region;
      region =
        region_get_visible_counterpart (region);
      ArrangerObject * region_obj =
        (ArrangerObject *) region;

      long region_start_ticks =
        region_obj->pos.total_ticks;
      Position tmp;
      int adj_px_per_key =
        MW_CHORD_EDITOR_SPACE->px_per_key + 1;

      /* use absolute position */
      ArrangerObject * co_obj =
        (ArrangerObject *) co;
      position_from_ticks (
        &tmp,
        region_start_ticks +
        co_obj->pos.total_ticks);
      allocation->x =
        ui_pos_to_px_editor (
          &tmp, 1);
      allocation->y =
        adj_px_per_key *
        co->index;

      char chord_str[100];
      chord_descriptor_to_string (descr, chord_str);
      int textw, texth;
      PangoLayout * layout =
        z_cairo_create_default_pango_layout (
          widget);
      z_cairo_get_text_extents_for_widget (
        widget, layout,
        chord_str, &textw, &texth);
      g_object_unref (layout);
      allocation->width =
        textw + CHORD_OBJECT_WIDGET_TRIANGLE_W +
        Z_CAIRO_TEXT_PADDING * 2;

      allocation->height = adj_px_per_key;
    }
}

/**
 * Returns the appropriate cursor based on the
 * current hover_x and y.
 */
ArrangerCursor
chord_arranger_widget_get_cursor (
  ChordArrangerWidget * self,
  UiOverlayAction action,
  Tool            tool)
{
  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  int is_hit =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_CHORD_OBJECT,
      ar_prv->hover_x, ar_prv->hover_y) != NULL;

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      switch (P_TOOL)
        {
        case TOOL_SELECT_NORMAL:
        {
          if (is_hit)
            {
              return ARRANGER_CURSOR_GRAB;
            }
          else
            {
              /* set cursor to normal */
              return ARRANGER_CURSOR_SELECT;
            }
        }
          break;
        case TOOL_SELECT_STRETCH:
          break;
        case TOOL_EDIT:
          ac = ARRANGER_CURSOR_EDIT;
          break;
        case TOOL_CUT:
          ac = ARRANGER_CURSOR_CUT;
          break;
        case TOOL_ERASER:
          ac = ARRANGER_CURSOR_ERASER;
          break;
        case TOOL_RAMP:
          ac = ARRANGER_CURSOR_RAMP;
          break;
        case TOOL_AUDITION:
          ac = ARRANGER_CURSOR_AUDITION;
          break;
        }
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
      ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    default:
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
}

/**
 * Create a ChordObject at the given Position in the
 * given Track.
 *
 * @param pos The pre-snapped position.
 */
void
chord_arranger_widget_create_chord (
  ChordArrangerWidget * self,
  const Position *      pos,
  int                   chord_index,
  Region *              region)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  ar_prv->action =
    UI_OVERLAY_ACTION_CREATING_MOVING;

  ArrangerObject * region_obj =
    (ArrangerObject *) region;

  /* get local pos */
  Position local_pos;
  position_from_ticks (
    &local_pos,
    pos->total_ticks -
    region_obj->pos.total_ticks);

  /* create a new chord */
  ChordObject * chord =
    chord_object_new (
      CLIP_EDITOR->region, chord_index, 1);
  ArrangerObject * chord_obj =
    (ArrangerObject *) chord;

  /* add it to chord region */
  chord_region_add_chord_object (
    region, chord);

  arranger_object_gen_widget (chord_obj);

  /* set visibility */
  arranger_object_set_widget_visibility_and_state (
    chord_obj, 1);

  arranger_object_set_position (
    chord_obj, &local_pos,
    ARRANGER_OBJECT_POSITION_TYPE_START,
    F_NO_CACHED, F_NO_VALIDATE,
    AO_UPDATE_ALL);

  EVENTS_PUSH (ET_ARRANGER_OBJECT_CREATED, chord);
  arranger_object_select (
    chord_obj, F_SELECT, F_NO_APPEND);
}

/**
 * Returns the chord index at y.
 */
int
chord_arranger_widget_get_chord_at_y (
  double y)
{
  double adj_y = y - 1;
  double adj_px_per_key =
    MW_CHORD_EDITOR_SPACE->px_per_key + 1;
  return (int) (adj_y / adj_px_per_key);
}

/**
 * Finds and selects items.
 *
 * @param[in] delete If this is a select-delete
 *   operation
 */
void
chord_arranger_widget_select (
  ChordArrangerWidget * self,
  double                   offset_x,
  double                   offset_y,
  int                      delete)
{
  int i;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (!delete)
    /* deselect all */
    arranger_widget_select_all (
      Z_ARRANGER_WIDGET (self), 0);

#define FIND_ENCLOSED_WIDGETS_OF_TYPE( \
  caps,cc,sc) \
  cc * sc; \
  cc##Widget * sc##_widget; \
  GtkWidget *  sc##_widgets[800]; \
  int          num_##sc##_widgets = 0; \
  arranger_widget_get_hit_widgets_in_range ( \
    Z_ARRANGER_WIDGET (self), \
    caps##_WIDGET_TYPE, \
    ar_prv->start_x, \
    ar_prv->start_y, \
    offset_x, \
    offset_y, \
    sc##_widgets, \
    &num_##sc##_widgets)

  FIND_ENCLOSED_WIDGETS_OF_TYPE (
    CHORD_OBJECT, ChordObject, chord_object);
  for (i = 0; i < num_chord_object_widgets; i++)
    {
      chord_object_widget =
        Z_CHORD_OBJECT_WIDGET (
          chord_object_widgets[i]);

      chord_object =
        chord_object_get_main (
          chord_object_widget->chord_object);

      if (delete)
        {
          chord_region_remove_chord_object (
            chord_object->region,
            chord_object, F_FREE);
        }
      else
        {
          arranger_object_select (
            (ArrangerObject *) chord_object,
            F_SELECT, F_APPEND);
        }
    }

#undef FIND_ENCLOSED_WIDGETS_OF_TYPE
}

static int
on_motion (
  GtkWidget *      widget,
  GdkEventMotion * event,
  ChordArrangerWidget * self)
{
  if (event->type == GDK_LEAVE_NOTIFY)
    self->hovered_index = -1;
  else
    self->hovered_index =
      chord_arranger_widget_get_chord_at_y (
        event->y);
  /*g_message ("hovered index: %d",*/
             /*self->hovered_index);*/

  ARRANGER_WIDGET_GET_PRIVATE (self);
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));

  return FALSE;
}

/**
 * Sets width to ruler width and height to
 * tracklist height.
 */
void
chord_arranger_widget_set_size (
  ChordArrangerWidget * self)
{
  // set the size
  RULER_WIDGET_GET_PRIVATE (EDITOR_RULER);
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    (int) rw_prv->total_px,
    MW_CHORD_EDITOR_SPACE->total_key_px);
}

/**
 * To be called once at init time.
 */
void
chord_arranger_widget_setup (
  ChordArrangerWidget * self)
{
  chord_arranger_widget_set_size (
    self);

  ARRANGER_WIDGET_GET_PRIVATE (self);
  g_signal_connect (
    G_OBJECT(ar_prv->bg),
    "motion-notify-event",
    G_CALLBACK (on_motion),  self);
}

void
chord_arranger_widget_move_items_y (
  ChordArrangerWidget * self,
  double                   offset_y)
{
}

/**
 * Returns the ticks objects were moved by since
 * the start of the drag.
 *
 * FIXME not really needed, can use
 * chord_selections_get_start_pos and the
 * arranger's earliest_obj_start_pos.
 */
/*static long*/
/*get_moved_diff (*/
  /*ChordArrangerWidget * self)*/
/*{*/
/*#define GET_DIFF(sc,pos_name) \*/
  /*if (CHORD_SELECTIONS->num_##sc##s) \*/
    /*{ \*/
      /*return \*/
        /*position_to_ticks ( \*/
          /*&sc##_get_main_trans_##sc ( \*/
            /*CHORD_SELECTIONS->sc##s[0])->pos_name) - \*/
        /*position_to_ticks ( \*/
          /*&sc##_get_main_##sc ( \*/
            /*CHORD_SELECTIONS->sc##s[0])->pos_name); \*/
    /*}*/

  /*GET_DIFF (chord_object, pos);*/

  /*g_return_val_if_reached (0);*/
/*}*/

/**
 * Sets the default cursor in all selected regions and
 * intializes start positions.
 */
void
chord_arranger_widget_on_drag_end (
  ChordArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  switch (ar_prv->action)
    {
    case UI_OVERLAY_ACTION_STARTING_MOVING:
      {
        /* if something was clicked with ctrl without
         * moving*/
        if (ar_prv->ctrl_held)
          {
            if (ar_prv->start_object &&
                arranger_object_is_selected (
                  ar_prv->start_object))
              {
                /*[> deselect it <]*/
                arranger_object_select (
                  ar_prv->start_object,
                  F_NO_SELECT, F_APPEND);
              }
          }
      }
      break;
    case UI_OVERLAY_ACTION_MOVING:
      {
        ChordObject * co =
          chord_object_get_main (
            CHORD_SELECTIONS->chord_objects[0]);
        ArrangerObject * co_obj =
          (ArrangerObject *) co;
        long ticks_diff =
          co_obj->pos.total_ticks -
            co_obj->cache_pos.total_ticks;
        UndoableAction * ua =
          (UndoableAction *)
          arranger_selections_action_new_move_chord (
            CHORD_SELECTIONS,
            ticks_diff,
            0);
        undo_manager_perform (
          UNDO_MANAGER, ua);
        arranger_selections_reset_counterparts (
          (ArrangerSelections *)
          CHORD_SELECTIONS, 1);
      }
      break;
    case UI_OVERLAY_ACTION_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      {
        ChordObject * co =
          chord_object_get_main (
            CHORD_SELECTIONS->chord_objects[0]);
        ArrangerObject * co_obj =
          (ArrangerObject *) co;
        long ticks_diff =
          co_obj->pos.total_ticks -
            co_obj->cache_pos.total_ticks;
        arranger_selections_reset_counterparts (
          (ArrangerSelections *)
          CHORD_SELECTIONS, 0);
        UndoableAction * ua =
          (UndoableAction *)
          arranger_selections_action_new_duplicate_chord (
            CHORD_SELECTIONS,
            ticks_diff, 0);
        arranger_selections_reset_counterparts (
          (ArrangerSelections *)
          CHORD_SELECTIONS, 0);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_NONE:
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
      {
        arranger_selections_clear (
          (ArrangerSelections *)
          CHORD_SELECTIONS);
      }
      break;
    case UI_OVERLAY_ACTION_CREATING_MOVING:
      {
        UndoableAction * ua =
          arranger_selections_action_new_create (
            (ArrangerSelections *) CHORD_SELECTIONS);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    /* if didn't click on something */
    default:
      {
      }
      break;
    }
}

static void
add_children_from_chord_track (
  ChordArrangerWidget * self,
  ChordTrack *          ct)
{
  int i, j, k;
  Region * r;
  ChordObject * c;
  for (i = 0; i < ct->num_chord_regions; i++)
    {
      r = ct->chord_regions[i];

      for (j = 0; j < r->num_chord_objects; j++)
        {
          c = r->chord_objects[j];

          for (k = 0 ;
               k <= AOI_COUNTERPART_MAIN_TRANSIENT;
               k++)
            {
              if (k == AOI_COUNTERPART_MAIN)
                c =
                  chord_object_get_main (c);
              else if (
                k == AOI_COUNTERPART_MAIN_TRANSIENT)
                c =
                  chord_object_get_main_trans (c);

              g_return_if_fail (c);
              ArrangerObject * obj =
                (ArrangerObject *) c;

              if (!obj->widget)
                arranger_object_gen_widget (obj);

              arranger_widget_add_overlay_child (
                (ArrangerWidget *) self, obj);
            }
        }
    }
}

/**
 * Readd children.
 */
void
chord_arranger_widget_refresh_children (
  ChordArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* remove all children except bg && playhead */
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
          /*g_object_ref (widget);*/
          gtk_container_remove (
            GTK_CONTAINER (self),
            widget);
        }
    }
  g_list_free (children);

  add_children_from_chord_track (
    self, P_CHORD_TRACK);

  gtk_overlay_reorder_overlay (
    GTK_OVERLAY (self),
    (GtkWidget *) ar_prv->playhead, -1);
}

/**
 * Scroll to the given position.
 */
/*void*/
/*chord_arranger_widget_scroll_to (*/
  /*ChordArrangerWidget * self,*/
  /*Position *               pos)*/
/*{*/
  /*[> TODO <]*/

/*}*/

static gboolean
on_focus (
  GtkWidget * widget,
  gpointer    user_data)
{
  /*g_message ("chord focused");*/
  MAIN_WINDOW->last_focused = widget;

  return FALSE;
}

static void
chord_arranger_widget_class_init (
  ChordArrangerWidgetClass * klass)
{
}

static void
chord_arranger_widget_init (
  ChordArrangerWidget *self )
{
  g_signal_connect (
    self, "grab-focus",
    G_CALLBACK (on_focus), self);
}
