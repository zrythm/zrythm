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

#include "audio/automation_region.h"
#include "audio/chord_track.h"
#include "audio/marker_track.h"
#include "gui/backend/arranger_object.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/chord_region.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/piano_roll_keys.h"
#include "gui/widgets/region.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/track.h"
#include "gui/widgets/velocity.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/ui.h"

#include <glib/gi18n-lib.h>

#define TYPE(x) \
  ARRANGER_OBJECT_TYPE_##x

/**
 * Queues a redraw in the area covered by this
 * object.
 */
void
arranger_object_queue_redraw (
  ArrangerObject * self)
{
  ArrangerWidget * arranger =
    arranger_object_get_arranger (self);
  g_return_if_fail (arranger);
  GdkRectangle arranger_rect;
  arranger_widget_get_visible_rect (
    arranger, &arranger_rect);

  /* if arranger is not visible ignore */
  if (arranger_rect.width < 2 &&
      arranger_rect.height < 2)
    return;

  /* set rectangle if not initialized yet */
  if (self->full_rect.width == 0 &&
      self->full_rect.height == 0)
    arranger_object_set_full_rectangle (
      self, arranger);

  GdkRectangle full_rect = self->full_rect;

  /* add some padding to the full rect for any
   * effects or things like automation points */
  static const int padding = 6;
  if (self->type == TYPE (AUTOMATION_POINT))
    {
      full_rect.x = MAX (full_rect.x - padding, 0);
      full_rect.y = MAX (full_rect.y - padding, 0);
      full_rect.width += padding * 2;
      full_rect.height += padding * 2;
    }

  GdkRectangle draw_rect;
  int draw_rect_visible =
    arranger_object_get_draw_rectangle (
      self, &arranger_rect, &full_rect,
      &draw_rect);

  /* if draw rect is not visible ignore */
  if (!draw_rect_visible)
    return;

  arranger_widget_redraw_rectangle (
    arranger, &draw_rect);

  /* if region and lanes are visible, redraw
   * lane too */
  if (self->type == TYPE (REGION))
    {
      Track * track =
        arranger_object_get_track (self);
      if (track->lanes_visible &&
          self->can_have_lanes)
        {
          ZRegion * r = (ZRegion *) self;
          region_get_lane_full_rect (
            r, &full_rect);
          arranger_object_get_draw_rectangle (
            self, &arranger_rect, &full_rect,
            &draw_rect);
          arranger_widget_redraw_rectangle (
            arranger, &draw_rect);
        }
    }
}

#if 0
/**
 * Draws the cut line if in cut mode.
 */
void
arranger_object_widget_draw_cut_line (
  ArrangerObjectWidget * self,
  cairo_t *              cr,
  GdkRectangle *         rect)
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
      switch (obj->type)
        {
        case ARRANGER_OBJECT_TYPE_REGION:
          start_pos_px =
            ui_pos_to_px_timeline (
              &obj->pos, F_PADDING);
          ui_px_to_pos_timeline (
            start_pos_px + ao_prv->hover_x,
            &pos, F_PADDING);
          break;
        case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
          start_pos_px =
            ui_pos_to_px_editor (
              &obj->pos, F_PADDING);
          ui_px_to_pos_editor (
            start_pos_px + ao_prv->hover_x,
            &pos, F_PADDING);
          break;
        default:
          g_return_if_reached ();
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
      switch (obj->type)
        {
        case ARRANGER_OBJECT_TYPE_REGION:
          px =
            ui_pos_to_px_timeline (
              &pos, F_PADDING);
          break;
        case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
          px =
            ui_pos_to_px_editor (
              &pos, F_PADDING);
          break;
        default:
          g_return_if_reached ();
        }

      /* convert to local px */
      px -= start_pos_px;

      if (px >= rect->x &&
          px < rect->x + rect->width)
        {
          double dashes[] = { 5 };
          cairo_set_dash (cr, dashes, 1, 0);
          cairo_set_source_rgba (
            cr, 1, 1, 1, 1.0);
          cairo_move_to (cr, px, 0);
          cairo_line_to (cr, px, height);
          cairo_stroke (cr);
        }
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
          self, (int) event->x, (int) event->y);
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
 * Returns if the ArrangerObjectWidget should
 * be redrawn.
 *
 * @param rect The rectangle for this draw cycle.
 */
int
arranger_object_widget_should_redraw (
  ArrangerObjectWidget * self,
  GdkRectangle *         rect)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  return
    ao_prv->redraw ||
    ao_prv->last_rect.x != rect->x ||
    ao_prv->last_rect.y != rect->y ||
    ao_prv->last_rect.width != rect->width ||
    ao_prv->last_rect.height != rect->height;
}

/**
 * To be called after redrawing an arranger object.
 */
void
arranger_object_widget_post_redraw (
  ArrangerObjectWidget * self,
  GdkRectangle *         rect)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);
  ao_prv->redraw = 0;
  ao_prv->last_rect = *rect;
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
#endif

/**
 * Returns if the current position is for resizing
 * L.
 *
 * @param x X in local coordinates.
 */
int
arranger_object_is_resize_l (
  ArrangerObject * self,
  const int        x)
{
  if (!self->has_length)
    return 0;

  if (x < UI_RESIZE_CURSOR_SPACE)
    {
      return 1;
    }
  return 0;
}

/**
 * Returns if the current position is for resizing
 * R.
 *
 * @param x X in local coordinates.
 */
int
arranger_object_is_resize_r (
  ArrangerObject * self,
  const int        x)
{
  if (!self->has_length)
    return 0;

  long size_frames =
    self->end_pos.frames - self->pos.frames;
  Position pos;
  position_from_frames (
    &pos, size_frames);
  int width_px =
    arranger_widget_pos_to_px (
      arranger_object_get_arranger (self),
      &pos, 0);

  if (x > width_px - UI_RESIZE_CURSOR_SPACE)
    {
      return 1;
    }
  return 0;
}

/**
 * Returns if the current position is for resizing
 * up (eg, Velocity).
 *
 * @param x X in local coordinates.
 * @param y Y in local coordinates.
 */
int
arranger_object_is_resize_up (
  ArrangerObject * self,
  const int        x,
  const int        y)
{
  if (self->type ==
         ARRANGER_OBJECT_TYPE_VELOCITY)
    {
      if (y < UI_RESIZE_CURSOR_SPACE)
        return 1;
    }
  else if (self->type ==
             ARRANGER_OBJECT_TYPE_AUTOMATION_POINT)
    {
      AutomationPoint * ap =
        (AutomationPoint*) self;
      int curve_up =
        automation_point_curves_up (ap);
      if (curve_up)
        {
          if (x > AP_WIDGET_POINT_SIZE ||
              self->full_rect.height - y >
                AP_WIDGET_POINT_SIZE)
            return 1;
        }
      else
        {
          if (x > AP_WIDGET_POINT_SIZE ||
              y > AP_WIDGET_POINT_SIZE)
            return 1;
        }
    }
  return 0;
}

/**
 * Returns if the current position is for resizing
 * loop.
 *
 * @param y Y in local coordinates.
 */
int
arranger_object_is_resize_loop (
  ArrangerObject * self,
  const int        y)
{
  if (!self->has_length || !self->can_loop)
    return 0;

  if (self->type == ARRANGER_OBJECT_TYPE_REGION)
    {
      ZRegion * r = (ZRegion *) self;
      if (r->type == REGION_TYPE_AUDIO)
        return 1;

      if ((position_to_ticks (&self->end_pos) -
           position_to_ticks (&self->pos)) >
          position_to_ticks (&self->loop_end_pos))
        {
          return 1;
        }

      /* TODO */
      int height_px = 60;
        /*gtk_widget_get_allocated_height (*/
          /*GTK_WIDGET (self));*/

      if (y > height_px / 2)
        {
          return 1;
        }
      return 0;
    }

  return 0;
}

/**
 * Returns if arranger_object widgets should show
 * cut lines.
 *
 * To be used to set the arranger_object's
 * "show_cut".
 *
 * @param alt_pressed Whether alt is currently
 *   pressed.
 */
int
arranger_object_should_show_cut_lines (
  ArrangerObject * self,
  int              alt_pressed)
{
  if (!self->has_length)
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
 * Returns Y in pixels from the value based on the
 * allocation of the arranger.
 */
static int
get_automation_point_y (
  AutomationPoint * ap,
  ArrangerWidget *  arranger)
{
  /* ratio of current value in the range */
  float ap_ratio = ap->normalized_val;

  int allocated_h =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (arranger));
  int point =
    allocated_h -
    (int) (ap_ratio * (float) allocated_h);
  return point;
}

void
arranger_object_set_full_rectangle (
  ArrangerObject * self,
  ArrangerWidget * arranger)
{
  switch (self->type)
    {
    case TYPE (CHORD_OBJECT):
      {
        ChordObject * co = (ChordObject *) self;
        ChordDescriptor * descr =
          CHORD_EDITOR->chords[co->index];

        ZRegion * region = co->region;
        ArrangerObject * region_obj =
          (ArrangerObject *) region;

        long region_start_ticks =
          region_obj->pos.total_ticks;
        Position tmp;
        int adj_px_per_key =
          MW_CHORD_EDITOR_SPACE->px_per_key + 1;

        /* use absolute position */
        position_from_ticks (
          &tmp,
          region_start_ticks +
          self->pos.total_ticks);
        self->full_rect.x =
          ui_pos_to_px_editor (
            &tmp, 1);
        self->full_rect.y =
          adj_px_per_key *
          co->index;

        char chord_str[100];
        chord_descriptor_to_string (
          descr, chord_str);

        chord_object_recreate_pango_layouts (
          (ChordObject *) self);
        self->full_rect.width =
          self->textw +
          CHORD_OBJECT_WIDGET_TRIANGLE_W +
          Z_CAIRO_TEXT_PADDING * 2;

        self->full_rect.width =
          self->textw +
          CHORD_OBJECT_WIDGET_TRIANGLE_W +
          Z_CAIRO_TEXT_PADDING * 2;

        self->full_rect.height = adj_px_per_key;
      }
      break;
    case TYPE (AUTOMATION_POINT):
      {
        AutomationPoint * ap =
          (AutomationPoint *) self;
        ZRegion * region = ap->region;
        ArrangerObject * region_obj =
          (ArrangerObject *) region;

        /* use absolute position */
        long region_start_ticks =
          region_obj->pos.total_ticks;
        Position tmp;
        position_from_ticks (
          &tmp,
          region_start_ticks +
          self->pos.total_ticks);
        self->full_rect.x =
          ui_pos_to_px_editor (&tmp, 1) -
            AP_WIDGET_POINT_SIZE / 2;
        g_warn_if_fail (self->full_rect.x >= 0);

        AutomationPoint * next_ap =
          automation_region_get_next_ap (
            ap->region, ap);

        if (next_ap)
          {
            ArrangerObject * next_obj =
              (ArrangerObject *) next_ap;

            /* get relative position from the
             * start AP to the next ap. */
            position_from_ticks (
              &tmp,
              next_obj->pos.total_ticks -
                self->pos.total_ticks);

            /* width is the relative position in px
             * plus half an AP_WIDGET_POINT_SIZE for
             * each side */
            self->full_rect.width =
              AP_WIDGET_POINT_SIZE +
              ui_pos_to_px_editor (&tmp, 0);

            int cur_y =
              get_automation_point_y (ap, arranger);
            int next_y =
              get_automation_point_y (
                next_ap, arranger);

            self->full_rect.y =
              (cur_y > next_y ?
               next_y : cur_y) -
              AP_WIDGET_POINT_SIZE / 2;

            /* height is the relative relative diff in
             * px between the two points plus half an
             * AP_WIDGET_POINT_SIZE for each side */
            self->full_rect.height =
              (cur_y > next_y ?
               cur_y - next_y :
               next_y - cur_y) + AP_WIDGET_POINT_SIZE;
          }
        else
          {
            self->full_rect.y =
              get_automation_point_y (
                 ap, arranger) -
              AP_WIDGET_POINT_SIZE / 2;

            self->full_rect.width =
              AP_WIDGET_POINT_SIZE;
            self->full_rect.height =
              AP_WIDGET_POINT_SIZE;
          }
        }
      break;
    case TYPE (REGION):
      {
        ZRegion * region =
          (ZRegion *) self;
        Track * track =
          arranger_object_get_track (self);

        if (!track->widget)
          track->widget = track_widget_new (track);

        self->full_rect.x =
          ui_pos_to_px_timeline (
            &self->pos,
            1);
        self->full_rect.width =
          (ui_pos_to_px_timeline (
            &self->end_pos,
            1) - self->full_rect.x) - 1;
        if (self->full_rect.width < 1)
          self->full_rect.width = 1;

        gint wx, wy;

        gtk_widget_translate_coordinates(
          (GtkWidget *) (track->widget),
          (GtkWidget *) (arranger),
          0, 0,
          &wx, &wy);

        if (region->type == REGION_TYPE_CHORD)
          {
            chord_region_recreate_pango_layouts (
              region);

            self->full_rect.y = wy;
            /* full height minus the space the
             * scales would require, plus some
             * padding */
            self->full_rect.height =
              track->main_height -
                (self->texth +
                 Z_CAIRO_TEXT_PADDING * 4);
          }
        else if (region->type ==
                   REGION_TYPE_AUTOMATION)
          {
            AutomationTrack * at =
              region_get_automation_track (region);
            g_return_if_fail (at);
            if (!at->created ||
                !track->automation_visible)
              return;

            gtk_widget_translate_coordinates(
              (GtkWidget *) (track->widget),
              (GtkWidget *) (arranger),
              0, 0, &wx, &wy);

            self->full_rect.y =
              wy + at->y;
            self->full_rect.height = at->height;
          }
        else
          {
            self->full_rect.y = wy;
            self->full_rect.height =
              track->main_height;
          }
        /* leave some space for the line below
         * the region */
        self->full_rect.height--;
      }
      break;
    case TYPE (MIDI_NOTE):
      {
        MidiNote * mn = (MidiNote *) self;
        ZRegion * region =
          mn->region;
        g_return_if_fail (region);
        ArrangerObject * region_obj =
          (ArrangerObject *) region;

        long region_start_ticks =
          region_obj->pos.total_ticks;
        Position tmp;
        double adj_px_per_key =
          MW_PIANO_ROLL_KEYS->px_per_key + 1.0;

        /* use absolute position */
        position_from_ticks (
          &tmp,
          region_start_ticks +
          self->pos.total_ticks);
        self->full_rect.x =
          ui_pos_to_px_editor (
            &tmp, 1);
        self->full_rect.y =
          (int)
          (adj_px_per_key *
           piano_roll_find_midi_note_descriptor_by_val (
             PIANO_ROLL, mn->val)->index);

        self->full_rect.height =
          (int) adj_px_per_key;
        if (PIANO_ROLL->drum_mode)
          {
            self->full_rect.width =
              self->full_rect.height;
            self->full_rect.x -=
              self->full_rect.width / 2;
          }
        else
          {
            /* use absolute position */
            position_from_ticks (
              &tmp,
              region_start_ticks +
              self->end_pos.total_ticks);
            self->full_rect.width =
              ui_pos_to_px_editor (
                &tmp, 1) - self->full_rect.x;
          }
      }
      break;
    case TYPE (SCALE_OBJECT):
      {
        Track * track = P_CHORD_TRACK;

        gint wx, wy;
        gtk_widget_translate_coordinates(
          (GtkWidget *) (track->widget),
          (GtkWidget *) (arranger),
          0, 0,
          &wx, &wy);

        self->full_rect.x =
          ui_pos_to_px_timeline (
            &self->pos, 1);
        scale_object_recreate_pango_layouts (
          (ScaleObject *) self);
        self->full_rect.width =
          self->textw +
          SCALE_OBJECT_WIDGET_TRIANGLE_W +
          Z_CAIRO_TEXT_PADDING * 2;

        int obj_height =
          self->texth + Z_CAIRO_TEXT_PADDING * 2;
        self->full_rect.y =
          (wy + track->main_height) - obj_height;
        self->full_rect.height = obj_height;
      }
      break;
    case TYPE (MARKER):
      {
        Track * track = P_MARKER_TRACK;

        gint wx, wy;
        gtk_widget_translate_coordinates(
          (GtkWidget *) (track->widget),
          (GtkWidget *) (arranger),
          0, 0,
          &wx, &wy);

        self->full_rect.x =
          ui_pos_to_px_timeline (
            &self->pos, 1);
        marker_recreate_pango_layouts (
          (Marker *) self);
        self->full_rect.width =
          self->textw + MARKER_WIDGET_TRIANGLE_W +
          Z_CAIRO_TEXT_PADDING * 2;

        int obj_height =
          self->texth + Z_CAIRO_TEXT_PADDING * 2;
        self->full_rect.y =
          (wy + track->main_height) - obj_height;
        self->full_rect.height = obj_height;
      }
      break;
    case TYPE (VELOCITY):
      {
        /* use transient or non transient note
         * depending on which is visible */
        Velocity * vel = (Velocity *) self;
        MidiNote * mn = vel->midi_note;
        ArrangerObject * mn_obj =
          (ArrangerObject *) mn;
        ZRegion * region = mn->region;
        g_return_if_fail (region);
        ArrangerObject * region_obj =
          (ArrangerObject *) region;

        /* use absolute position */
        long region_start_ticks =
          region_obj->pos.total_ticks;
        Position tmp;
        position_from_ticks (
          &tmp,
          region_start_ticks +
          mn_obj->pos.total_ticks);
        self->full_rect.x =
          ui_pos_to_px_editor (&tmp, 1);
        int height =
          gtk_widget_get_allocated_height (
            GTK_WIDGET (arranger));

        int vel_px =
          (int)
          ((float) height *
            ((float) vel->vel / 127.f));
        self->full_rect.y = height - vel_px;
        self->full_rect.width = 12;
        self->full_rect.height = vel_px;
      }
      break;
    default:
      g_warn_if_reached ();
      break;
    }
}

/**
 * Gets the draw rectangle based on the given
 * full rectangle of the arranger object.
 *
 * @param parent_rect The current arranger
 *   rectangle.
 * @param full_rect The object's full rectangle.
 *   This will usually be ArrangerObject->full_rect,
 *   unless drawing in a lane (for Region's).
 * @param draw_rect The rectangle will be set
 *   here.
 *
 * @return Whether the draw rect is visible.
 */
int
arranger_object_get_draw_rectangle (
  ArrangerObject * self,
  GdkRectangle *   parent_rect,
  GdkRectangle *   full_rect,
  GdkRectangle *   draw_rect)
{
  if (!ui_rectangle_overlap (
        parent_rect, full_rect))
    return 0;

  draw_rect->x =
    MAX (full_rect->x, parent_rect->x);
  draw_rect->width =
    MIN (
      (parent_rect->x + parent_rect->width) -
        draw_rect->x,
      (full_rect->x + full_rect->width) -
      draw_rect->x);
  g_warn_if_fail (draw_rect->width >= 0);
  draw_rect->y =
    MAX (full_rect->y, parent_rect->y);
  draw_rect->height =
    MIN (
      (parent_rect->y + parent_rect->height) -
        draw_rect->y,
      (full_rect->y + full_rect->height) -
      draw_rect->y);
  g_warn_if_fail (draw_rect->height >= 0);
  return 1;
  /*g_message ("full rect: (%d, %d) w: %d h: %d",*/
    /*full_rect->x, full_rect->y,*/
    /*full_rect->width, full_rect->height);*/
  /*g_message ("draw rect: (%d, %d) w: %d h: %d",*/
    /*draw_rect->x, draw_rect->y,*/
    /*draw_rect->width, draw_rect->height);*/
}

/**
 * Draws the given object.
 *
 * To be called from the arranger's draw callback.
 *
 * @param cr Cairo context of the arranger.
 * @param rect Rectangle in the arranger.
 */
void
arranger_object_draw (
  ArrangerObject * self,
  ArrangerWidget * arranger,
  cairo_t *        cr,
  GdkRectangle *   rect)
{
  switch (self->type)
    {
    case TYPE (AUTOMATION_POINT):
      automation_point_draw (
        (AutomationPoint *) self, cr, rect);
      break;
    case TYPE (REGION):
      region_draw (
        (ZRegion *) self, cr, rect);
      break;
    case TYPE (MIDI_NOTE):
      midi_note_draw (
        (MidiNote *) self, cr, rect);
      break;
    case TYPE (MARKER):
      marker_draw (
        (Marker *) self, cr, rect);
      break;
    case TYPE (SCALE_OBJECT):
      scale_object_draw (
        (ScaleObject *) self, cr, rect);
      break;
    case TYPE (CHORD_OBJECT):
      chord_object_draw (
        (ChordObject *) self, cr, rect);
      break;
    case TYPE (VELOCITY):
      velocity_draw (
        (Velocity *) self, cr, rect);
      break;
    default:
      g_warn_if_reached ();
      break;
    }
}
