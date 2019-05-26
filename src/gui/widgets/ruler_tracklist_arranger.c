/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "audio/chord.h"
#include "audio/chord_track.h"
#include "audio/ruler_tracklist.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/ruler_tracklist.h"
#include "gui/widgets/ruler_tracklist_arranger.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (RulerTracklistArrangerWidget,
               ruler_tracklist_arranger_widget,
               ARRANGER_WIDGET_TYPE)

/**
 * To be called from get_child_position in parent widget.
 *
 * Used to allocate the overlay children.
 */
void
ruler_tracklist_arranger_widget_set_allocation (
  RulerTracklistArrangerWidget * self,
  GtkWidget *          widget,
  GdkRectangle *       allocation)
{
  if (Z_IS_CHORD_WIDGET (widget))
    {
      ChordWidget * cw = Z_CHORD_WIDGET (widget);
      Track * track = P_CHORD_TRACK;

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (track->widget),
                GTK_WIDGET (self),
                0,
                0,
                &wx,
                &wy);

      allocation->x =
        ui_pos_to_px_timeline (
          &cw->chord->pos,
          1);
      Position tmp;
      position_set_to_pos (&tmp,
                           &cw->chord->pos);
      position_set_beat (
        &tmp,
        tmp.beats + 1);
      allocation->y = wy;
      allocation->width =
        (ui_pos_to_px_timeline (
          &tmp,
          1) - allocation->x) - 1;
      allocation->height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (track->widget));
    }
}

/**
 * Returns the appropriate cursor based on the
 * current hover_x and y.
 */
ArrangerCursor
ruler_tracklist_arranger_widget_get_cursor (
  RulerTracklistArrangerWidget * self,
  UiOverlayAction action,
  Tool            tool)
{
  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      if (P_TOOL == TOOL_SELECT_NORMAL)
        {
          ChordWidget * rw =
            ruler_tracklist_arranger_widget_get_hit_chord (
              self,
              ar_prv->hover_x,
              ar_prv->hover_y);

          int is_hit =
            rw != NULL;

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
      else if (P_TOOL == TOOL_SELECT_STRETCH)
        {
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
      ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    default:
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
}


Track *
ruler_tracklist_arranger_widget_get_track_at_y (
  RulerTracklistArrangerWidget * self,
  double y)
{
  Track * tracks[3];
  tracks[0] = RULER_TRACKLIST->chord_track;
  tracks[1] = RULER_TRACKLIST->marker_track;
  int num_tracks = 2;
  for (int i = 0; i < num_tracks; i++)
    {
      Track * track = tracks[i];

      GtkAllocation allocation;
      gtk_widget_get_allocation (
        GTK_WIDGET (track->widget),
        &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates(
        GTK_WIDGET (self),
        GTK_WIDGET (track->widget),
        0,
        y,
        &wx,
        &wy);

      if (wy >= 0 && wy <= allocation.height)
        {
          return track;
        }
    }
  return NULL;
}

ChordWidget *
ruler_tracklist_arranger_widget_get_hit_chord (
  RulerTracklistArrangerWidget *  self,
  double                    x,
  double                    y)
{
  GtkWidget * widget =
    ui_get_hit_child (
      GTK_CONTAINER (self),
      x,
      y,
      CHORD_WIDGET_TYPE);
  if (widget)
    {
      return Z_CHORD_WIDGET (widget);
    }
  return NULL;
}

void
ruler_tracklist_arranger_widget_select_all (
  RulerTracklistArrangerWidget *  self,
  int                       select)
{
  /* select chords */
  ChordTrack * ct =
    tracklist_get_chord_track (
      TRACKLIST);
  for (int i = 0; i < ct->num_chords; i++)
    {
      ZChord * chord = ct->chords[i];
      if (chord->visible)
        {
          chord_widget_select (
            chord->widget, select);
        }
    }
}

/**
 * Shows context menu.
 *
 * To be called from parent on right click.
 */
void
ruler_tracklist_arranger_widget_show_context_menu (
  RulerTracklistArrangerWidget * self,
  gdouble              x,
  gdouble              y)
{
  GtkWidget *menu, *menuitem;


  menu = gtk_menu_new();

  menuitem =
    gtk_menu_item_new_with_label("Do something");


  gtk_menu_shell_append (
    GTK_MENU_SHELL(menu), menuitem);

  gtk_widget_show_all(menu);

  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);
}

/**
 * Sets the visibility of the transient and non-
 * transient objects.
 *
 * E.g. when moving regions, it hides the original
 * ones.
 */
void
ruler_tracklist_arranger_widget_update_visibility (
  RulerTracklistArrangerWidget * self)
{
  ZChord * c;
  ZChord * c_transient;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  ARRANGER_SET_SELECTION_VISIBILITY (
    TL_SELECTIONS->chords,
    TL_SELECTIONS->transient_chords,
    TL_SELECTIONS->num_chords,
    c,
    c_transient);
}

void
ruler_tracklist_arranger_widget_on_drag_begin_chord_hit (
  RulerTracklistArrangerWidget * self,
  double                   start_x,
  ChordWidget *            cw)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* if double click */
  if (ar_prv->n_press == 2)
    {
      /* TODO */
    }

  ZChord * chord = cw->chord;
  self->start_chord = chord;

  /* update arranger action */
  ar_prv->action =
    UI_OVERLAY_ACTION_STARTING_MOVING;
  ui_set_cursor_from_name (
    GTK_WIDGET (cw), "grabbing");

  /* select/ deselect chords */
  if (ar_prv->ctrl_held)
    {
      /* if ctrl pressed toggle on/off */
      ARRANGER_WIDGET_SELECT_CHORD (
        self, chord, 1, 1, 0);
    }
  else if (!array_contains (
             (void **)TL_SELECTIONS->chords,
             TL_SELECTIONS->num_chords,
             chord))
    {
      /* else if not already selected select only it */
      ruler_tracklist_arranger_widget_select_all (
        self, 0);
      ARRANGER_WIDGET_SELECT_CHORD (
        self, chord, 1, 0, 0);
    }
}


/**
 * Fills in the positions that the RulerTracklistArranger
 * remembers at the start of each drag.
 */
void
ruler_tracklist_arranger_widget_set_init_poses (
  RulerTracklistArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  for (int i = 0; i < RT_SELECTIONS->num_chords; i++)
    {
      ZChord * r = RT_SELECTIONS->chords[i];
      if (position_compare (&r->pos,
                            &ar_prv->start_pos) <= 0)
        {
          position_set_to_pos (&ar_prv->start_pos,
                               &r->pos);
        }

      /* set start poses for chords */
      position_set_to_pos (&self->chord_start_poses[i],
                           &r->pos);
    }
}

void
ruler_tracklist_arranger_widget_create_chord (
  RulerTracklistArrangerWidget * self,
  Track *                  track,
  Position *               pos)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid) &&
      !ar_prv->shift_held)
    position_snap (NULL,
                   pos,
                   track,
                   NULL,
                   ar_prv->snap_grid);
 ZChord * chord = chord_new (NOTE_A,
                            1,
                            NOTE_A,
                            CHORD_TYPE_MIN,
                            0);
 position_set_to_pos (&chord->pos,
                      pos);
 /*ZChord * chords[1] = { chord };*/
 /*UndoableAction * action =*/
   /*create_chords_action_new (chords, 1);*/
 /*undo_manager_perform (UNDO_MANAGER,*/
                       /*action);*/
  ar_prv->action =
    UI_OVERLAY_ACTION_CREATING_MOVING;
  ARRANGER_WIDGET_SELECT_CHORD (
    self, chord, F_SELECT,
    F_NO_APPEND, F_NO_TRANSIENTS);
}

/**
 * First determines the selection type (objects/
 * range), then either finds and selects items or
 * selects a range.
 *
 * @param[in] delete If this is a select-delete
 *   operation
 */
void
ruler_tracklist_arranger_widget_select (
  RulerTracklistArrangerWidget * self,
  double                   offset_x,
  double                   offset_y,
  int                      delete)
{
  int i;
  ZChord * chord;
  ChordWidget * cw;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (!delete)
    /* deselect all */
    arranger_widget_select_all (
      Z_ARRANGER_WIDGET (self), 0);

  /* find enclosed chords */
  GtkWidget *    chord_widgets[800];
  int            num_chord_widgets = 0;
  arranger_widget_get_hit_widgets_in_range (
    Z_ARRANGER_WIDGET (self),
    CHORD_WIDGET_TYPE,
    ar_prv->start_x,
    ar_prv->start_y,
    offset_x,
    offset_y,
    chord_widgets,
    &num_chord_widgets);

  if (delete)
    {
      /* delete the enclosed chords */
      for (i = 0; i < num_chord_widgets; i++)
        {
          cw =
            Z_CHORD_WIDGET (chord_widgets[i]);

          chord = cw->chord;

          chord_track_remove_chord (
            P_CHORD_TRACK,
            chord);
      }
    }
  else
    {
      /* select the enclosed chords */
      for (i = 0; i < num_chord_widgets; i++)
        {
          cw =
            Z_CHORD_WIDGET (chord_widgets[i]);

          chord = cw->chord;

          ARRANGER_WIDGET_SELECT_CHORD (
            self, chord, F_SELECT, F_APPEND,
            F_NO_TRANSIENTS);
        }
    }
}

void
ruler_tracklist_arranger_widget_move_items_x (
  RulerTracklistArrangerWidget * self,
  long                     ticks_diff)
{
  Position tmp;
  long length_ticks;

  /* update chord positions */
  ZChord * c;
  for (int i = 0;
       i < TL_SELECTIONS->num_chords; i++)
    {
      c = TL_SELECTIONS->transient_chords[i];
      ARRANGER_MOVE_OBJ_BY_TICKS (
        c, chord,
        &self->chord_start_poses[i],
        ticks_diff, &tmp);
    }
}

/**
 * Sets width to ruler width and height to
 * tracklist height.
 */
void
ruler_tracklist_arranger_widget_set_size (
  RulerTracklistArrangerWidget * self)
{
  // set the size
  int ww, hh;
  gtk_widget_get_size_request (
    GTK_WIDGET (MW_RULER_TRACKLIST),
    &ww,
    &hh);
  RULER_WIDGET_GET_PRIVATE (MW_RULER);
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    rw_prv->total_px,
    hh);
}

/**
 * To be called once at init time.
 */
void
ruler_tracklist_arranger_widget_setup (
  RulerTracklistArrangerWidget * self)
{
  ruler_tracklist_arranger_widget_set_size (
    self);
}

/**
 * Sets the default cursor in all selected regions and
 * intializes start positions.
 */
void
ruler_tracklist_arranger_widget_on_drag_end (
  RulerTracklistArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      /* if something was clicked with ctrl without
       * moving*/
      if (ar_prv->ctrl_held)
        {
          /* TODO */
        }
    }
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING)
    {
      /* TODO */
    }
  /* if copy/link-moved */
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING_COPY ||
           ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING_LINK)
    {
      /* TODO */
    }
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_NONE ||
           ar_prv->action ==
             UI_OVERLAY_ACTION_STARTING_SELECTION)
    {
      ruler_tracklist_selections_clear (
        RT_SELECTIONS);
    }
  /* if something was created */
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_CREATING_MOVING)
    {
      /* TODO */
    }
  /* if didn't click on something */
  else
    {
      self->start_chord = NULL;
    }
  ruler_tracklist_selections_remove_transients (
    RT_SELECTIONS);
  ar_prv->action = UI_OVERLAY_ACTION_NONE;
  ruler_tracklist_arranger_widget_update_visibility (
    self);

  EVENTS_PUSH (ET_RT_SELECTIONS_CHANGED,
               NULL);
}

static void
add_children_from_chord_track (
  RulerTracklistArrangerWidget * self,
  ChordTrack *             ct)
{
  for (int i = 0; i < ct->num_chords; i++)
    {
      ZChord * chord = ct->chords[i];
      gtk_overlay_add_overlay (
        GTK_OVERLAY (self),
        GTK_WIDGET (chord->widget));
    }
}

/**
 * Readd children.
 */
void
ruler_tracklist_arranger_widget_refresh_children (
  RulerTracklistArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* remove all children except bg && playhead */
  GList *children, *iter;

  children =
    gtk_container_get_children (GTK_CONTAINER (self));
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

  /* add tracks */
  Track * tracks[3];
  tracks[0] = RULER_TRACKLIST->chord_track;
  tracks[1] = RULER_TRACKLIST->marker_track;
  int num_tracks = 2;
  Track * track;
  for (int i = 0; i < num_tracks; i++)
    {
      track = tracks[i];
      if (track->visible)
        {
          switch (track->type)
            {
            case TRACK_TYPE_CHORD:
              add_children_from_chord_track (
                self,
                (ChordTrack *) track);
              break;
            default:
              break;
            }
        }
    }
}

/**
 * Scroll to the given position.
 */
void
ruler_tracklist_arranger_widget_scroll_to (
  RulerTracklistArrangerWidget * self,
  Position *               pos)
{
  /* TODO */

}

static gboolean
on_focus (GtkWidget       *widget,
          gpointer         user_data)
{
  MAIN_WINDOW->last_focused = widget;

  return FALSE;
}

static void
ruler_tracklist_arranger_widget_class_init (
  RulerTracklistArrangerWidgetClass * klass)
{
}

static void
ruler_tracklist_arranger_widget_init (
  RulerTracklistArrangerWidget *self )
{
  g_signal_connect (
    self, "grab-focus",
    G_CALLBACK (on_focus), self);
}
