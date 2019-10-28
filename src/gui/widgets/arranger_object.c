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

#include "gui/widgets/arranger_object.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/ui.h"

#include <glib/gi18n-lib.h>

G_DEFINE_TYPE_WITH_PRIVATE (
  ArrangerObjectWidget,
  arranger_object_widget,
  GTK_TYPE_BOX)

/**
 * Draws the cut line if in cut mode.
 */
void
arranger_object_widget_draw_cut_line (
  ArrangerObjectWidget * self,
  cairo_t *              cr)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);
  ArrangerObject * obj = ao_prv->arranger_object;

  if (ao_prv->show_cut)
    {
      int start_pos_px;
      double px;
      int height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (self));

      /* get absolute position */
      Position pos;
      if (obj->type == ARRANGER_OBJECT_TYPE_REGION)
        {
          start_pos_px =
            ui_pos_to_px_timeline (
              &obj->pos, F_PADDING);
          ui_px_to_pos_timeline (
            start_pos_px + ao_prv->hover_x,
            &pos, F_PADDING);
        }
      else if (obj->type ==
              ARRANGER_OBJECT_TYPE_MIDI_NOTE)
        {
          start_pos_px =
            ui_pos_to_px_editor (
              &obj->pos, F_PADDING);
          ui_px_to_pos_editor (
            start_pos_px + ao_prv->hover_x,
            &pos, F_PADDING);
        }

      /* snap */
      ArrangerWidget * arranger =
        arranger_object_get_arranger (obj);
      ArrangerWidgetPrivate * ar_prv =
        arranger_widget_get_private (
          arranger);
      if (!ar_prv->shift_held)
        {
          if (obj->type ==
                ARRANGER_OBJECT_TYPE_REGION)
            {
              position_snap_simple (
                &pos, SNAP_GRID_TIMELINE);
            }
          else if (obj->type ==
                     ARRANGER_OBJECT_TYPE_MIDI_NOTE)
            {
              position_snap_simple (
                &pos, SNAP_GRID_MIDI);
            }
        }

      /* convert back to px */
      if (obj->type ==
            ARRANGER_OBJECT_TYPE_REGION)
        {
          px =
            ui_pos_to_px_timeline (
              &pos, F_PADDING);
        }
      else if (obj->type ==
                 ARRANGER_OBJECT_TYPE_MIDI_NOTE)
        {
          px =
            ui_pos_to_px_editor (
              &pos, F_PADDING);
        }

      /* convert to local px */
      px -= start_pos_px;

      double dashes[] = { 5 };
      cairo_set_dash (cr, dashes, 1, 0);
      cairo_set_source_rgba (
        cr, 1, 1, 1, 1.0);
      cairo_move_to (cr, px, 0);
      cairo_line_to (cr, px, height);
      cairo_stroke (cr);
    }
}

static int
on_motion (
  GtkWidget *            widget,
  GdkEventMotion *       event,
  ArrangerObjectWidget * self)
{
  if (!MAIN_WINDOW || !MW_TIMELINE)
    return FALSE;

  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  int alt_pressed =
    event->state & GDK_MOD1_MASK;

  if (event->type == GDK_MOTION_NOTIFY)
    {
      ao_prv->show_cut =
        arranger_object_widget_should_show_cut_lines (
          self, alt_pressed);
      ao_prv->resize_l =
        arranger_object_widget_is_resize_l (
          self, (int) event->x);
      ao_prv->resize_r =
        arranger_object_widget_is_resize_r (
          self, (int) event->x);
      ao_prv->resize_up =
        arranger_object_widget_is_resize_up (
          self, (int) event->y);
      ao_prv->resize_loop =
        arranger_object_widget_is_resize_loop (
          self, (int) event->y);
      ao_prv->hover_x = (int) event->x;
    }

  if (event->type == GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT,
        0);
    }
  /* if leaving */
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      ArrangerWidget * arranger =
        arranger_object_get_arranger (
          ao_prv->arranger_object);
      ARRANGER_WIDGET_GET_PRIVATE (arranger);
      if (ar_prv->action !=
            UI_OVERLAY_ACTION_MOVING &&
          ar_prv->action !=
            UI_OVERLAY_ACTION_RESIZING_L &&
          ar_prv->action !=
            UI_OVERLAY_ACTION_RESIZING_R)
        {
          gtk_widget_unset_state_flags (
            GTK_WIDGET (self),
            GTK_STATE_FLAG_PRELIGHT);
          ao_prv->show_cut = 0;
        }
    }
  arranger_object_widget_force_redraw (self);

  return FALSE;
}

/**
 * Returns if arranger_object widgets should show cut
 * lines.
 *
 * @param alt_pressed Whether alt is currently
 *   pressed.
 */
int
arranger_object_widget_should_show_cut_lines (
  ArrangerObjectWidget * self,
  int alt_pressed)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);
  ArrangerObject * obj = ao_prv->arranger_object;

  if (!obj->has_length)
    return 0;

  switch (P_TOOL)
    {
    case TOOL_SELECT_NORMAL:
    case TOOL_SELECT_STRETCH:
      if (alt_pressed)
        return 1;
      else
        return 0;
      break;
    case TOOL_CUT:
      return 1;
      break;
    default:
      return 0;
      break;
    }
  g_return_val_if_reached (-1);
}

/**
 * Sets up the ArrangerObjectWidget.
 */
void
arranger_object_widget_setup (
  ArrangerObjectWidget * self,
  ArrangerObject *       arranger_object)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  ao_prv->arranger_object = arranger_object;
}

/**
 * Returns if the current position is for resizing
 * L.
 */
int
arranger_object_widget_is_resize_l (
  ArrangerObjectWidget * self,
  const int              x)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);
  ArrangerObject * obj = ao_prv->arranger_object;

  if (!obj->has_length)
    return 0;

  if (x < UI_RESIZE_CURSOR_SPACE)
    {
      return 1;
    }
  return 0;
}

/**
 * Returns if the current position is for resizing
 * L.
 */
int
arranger_object_widget_is_resize_r (
  ArrangerObjectWidget * self,
  const int              x)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);
  ArrangerObject * obj = ao_prv->arranger_object;

  if (!obj->has_length)
    return 0;

  GtkAllocation allocation;
  gtk_widget_get_allocation (
    GTK_WIDGET (self),
    &allocation);

  if (x > allocation.width - UI_RESIZE_CURSOR_SPACE)
    {
      return 1;
    }
  return 0;
}

/**
 * Returns if the current position is for resizing
 * up (eg, Velocity).
 */
int
arranger_object_widget_is_resize_up (
  ArrangerObjectWidget * self,
  const int              y)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  if (ao_prv->arranger_object->type ==
        ARRANGER_OBJECT_TYPE_VELOCITY &&
      y < UI_RESIZE_CURSOR_SPACE)
    {
      return 1;
    }
  return 0;
}

/**
 * Returns if the current position is for resizing
 * loop.
 */
int
arranger_object_widget_is_resize_loop (
  ArrangerObjectWidget * self,
  const int              y)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);
  ArrangerObject * obj = ao_prv->arranger_object;

  if (!obj->has_length || !obj->can_loop)
    return 0;

  if (obj->type == ARRANGER_OBJECT_TYPE_REGION)
    {
      Region * r = (Region *) obj;
      if (r->type == REGION_TYPE_AUDIO)
        return 1;

      if ((position_to_ticks (&obj->end_pos) -
           position_to_ticks (&obj->pos)) >
          position_to_ticks (&obj->loop_end_pos))
        {
          return 1;
        }

      int height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (self));

      if (y > height / 2)
        {
          return 1;
        }
      return 0;
    }

  return 0;
}

ArrangerObjectWidgetPrivate *
arranger_object_widget_get_private (
  ArrangerObjectWidget * self)
{
  return
    arranger_object_widget_get_instance_private (
      self);
}

void
arranger_object_widget_force_redraw (
  ArrangerObjectWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);
  ao_prv->redraw = 1;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
update_midi_note_tooltip (
  ArrangerObjectWidget * self,
  MidiNote *             mn)
{
  const char * note =
    chord_descriptor_note_to_string (
      mn->val % 12);

  /* set tooltip text */
  char * tooltip =
    g_strdup_printf (
      "[%s%d] %d",
      note, mn->val / 12 - 1, mn->vel->vel);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), tooltip);
  g_free (tooltip);
}

static void
update_automation_point_tooltip (
  ArrangerObjectWidget * self,
  AutomationPoint *      ap)
{
  char * tooltip =
    g_strdup_printf (
      "%s %f",
      ap->region->at->automatable->label,
      (double) ap->fvalue);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), tooltip);
  g_free (tooltip);
}

static void
update_velocity_tooltip (
  ArrangerObjectWidget * self,
  Velocity *             vel)
{
  char * tooltip =
    g_strdup_printf ("%d", vel->vel);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), tooltip);
  g_free (tooltip);
}

/**
 * Updates the normal tooltip of the widget, and
 * shows the custom tooltip window if show is 1.
 */
void
arranger_object_widget_update_tooltip (
  ArrangerObjectWidget * self,
  int                    show)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);
  ArrangerObject * obj =
    ao_prv->arranger_object;

  switch (obj->type)
    {
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      update_midi_note_tooltip (
        self, (MidiNote *) obj);
      break;
    case ARRANGER_OBJECT_TYPE_AUTOMATION_POINT:
      update_automation_point_tooltip (
        self, (AutomationPoint *) obj);
      break;
    case ARRANGER_OBJECT_TYPE_VELOCITY:
      update_velocity_tooltip (
        self, (Velocity *) obj);
      break;
    default:
      /* TODO*/
      break;
    }

  /* set tooltip window */
  /*if (show)*/
    /*{*/
      /*char * tooltip =*/
        /*gtk_widget_get_tooltip_text (*/
          /*GTK_WIDGET (self));*/
      /*gtk_label_set_text (self->tooltip_label,*/
                          /*tooltip);*/
      /*gtk_window_present (self->tooltip_win);*/
      /*g_free (tooltip);*/
    /*}*/
  /*else*/
    /*gtk_widget_hide (*/
      /*GTK_WIDGET (self->tooltip_win));*/
}

static void
finalize (
  ArrangerObjectWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  if (ao_prv->cached_cr)
    cairo_destroy (ao_prv->cached_cr);
  if (ao_prv->cached_surface)
    cairo_surface_destroy (ao_prv->cached_surface);

  G_OBJECT_CLASS (
    arranger_object_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
arranger_object_widget_class_init (
  ArrangerObjectWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) finalize;
}

static void
arranger_object_widget_init (
  ArrangerObjectWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  gtk_widget_set_visible (GTK_WIDGET (self), 1);

  /* add drawing area */
  ao_prv->drawing_area =
    GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (ao_prv->drawing_area), 1);
  gtk_widget_set_hexpand (
    GTK_WIDGET (ao_prv->drawing_area), 1);
  gtk_container_add (
    GTK_CONTAINER (self),
    GTK_WIDGET (ao_prv->drawing_area));

  gtk_widget_set_events (
    GTK_WIDGET (ao_prv->drawing_area),
    GDK_ALL_EVENTS_MASK);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (ao_prv->drawing_area),
    "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(ao_prv->drawing_area),
    "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(ao_prv->drawing_area),
    "motion-notify-event",
    G_CALLBACK (on_motion),  self);

  /* set tooltip window */
  /*self->tooltip_win =*/
    /*GTK_WINDOW (gtk_window_new (GTK_WINDOW_POPUP));*/
  /*gtk_window_set_type_hint (*/
    /*self->tooltip_win,*/
    /*GDK_WINDOW_TYPE_HINT_TOOLTIP);*/
  /*self->tooltip_label =*/
    /*GTK_LABEL (gtk_label_new ("label"));*/
  /*gtk_widget_set_visible (*/
    /*GTK_WIDGET (self->tooltip_label), 1);*/
  /*gtk_container_add (*/
    /*GTK_CONTAINER (self->tooltip_win),*/
    /*GTK_WIDGET (self->tooltip_label));*/
  /*gtk_window_set_position (*/
    /*self->tooltip_win, GTK_WIN_POS_MOUSE);*/

  ao_prv->redraw = 1;

  g_object_ref (self);
}
