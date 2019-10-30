/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "config.h"

#include "audio/engine.h"
#include "audio/midi.h"
#include "audio/port.h"
#include "gui/widgets/bar_slider.h"
#include "gui/widgets/inspector_port.h"
#include "gui/widgets/port_connections_popover.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/math.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  InspectorPortWidget,
  inspector_port_widget,
  GTK_TYPE_OVERLAY)

static void
on_jack_toggled (
  GtkWidget * widget,
  InspectorPortWidget * self)
{
  port_set_expose_to_jack (
    self->port,
    gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (widget)));
}

/**
 * Gets the label for the bar slider (port string).
 *
 * @return if the string was filled in or not.
 */
static int
get_port_str (
  Port * port,
  char * buf)
{
  if (port->identifier.owner_type ==
        PORT_OWNER_TYPE_PLUGIN ||
      port->identifier.owner_type ==
        PORT_OWNER_TYPE_FADER ||
      port->identifier.owner_type ==
        PORT_OWNER_TYPE_TRACK ||
      port->identifier.owner_type ==
        PORT_OWNER_TYPE_PREFADER)
    {
      if (port->identifier.flow == FLOW_INPUT)
        {
          sprintf (
            buf, "%s (%d)",
            port->identifier.label,
            port->num_srcs);
          return 1;
        }
      else if (port->identifier.flow == FLOW_OUTPUT)
        {
          sprintf (
            buf, "%s (%d)",
            port->identifier.label,
            port->num_dests);
          return 1;
        }
    }
  return 0;
}

static void
on_bind_midi_cc (
  GtkMenuItem *         menuitem,
  InspectorPortWidget * self)
{
  /*Port * port = self->port;*/

  /*midi_byte_t bytes[3];*/
}

static void
show_context_menu (
  InspectorPortWidget * self,
  gdouble               x,
  gdouble               y)
{
  GtkWidget *menu, *menuitem;

  menu = gtk_menu_new();

  menuitem =
    gtk_menu_item_new_with_label (
      _("Bind MIDI CC"));
  g_signal_connect (
    menuitem, "activate",
    G_CALLBACK (on_bind_midi_cc), self);
  gtk_menu_shell_append (
    GTK_MENU_SHELL(menu), menuitem);

  gtk_widget_show_all (menu);

  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);
}

static void
on_right_click (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  InspectorPortWidget * self)
{
  if (n_press != 1)
    return;

  g_message ("right click");

  show_context_menu (self, x, y);
}

static void
on_double_click (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  InspectorPortWidget * self)
{
  if (n_press != 2)
    return;

  g_message ("double click");

  PortConnectionsPopoverWidget * popover =
    port_connections_popover_widget_new (self);
  port_connections_popover_widget_refresh (
    popover);
  /*gtk_popover_popdown (GTK_POPOVER (popover));*/
  gtk_widget_show_all (GTK_WIDGET (popover));
}

/** 250 ms */
static const float MAX_TIME = 250000.f;

static float
get_port_value (
  InspectorPortWidget * self)
{
  Port * port = self->port;
  switch (port->identifier.type)
    {
    case TYPE_AUDIO:
      {
        float rms =
          math_calculate_rms_db (
            port->buf, AUDIO_ENGINE->block_length);
        return math_dbfs_to_amp (rms);
      }
      break;
    case TYPE_CV:
      {
        float rms =
          math_calculate_rms_db (
            port->buf, AUDIO_ENGINE->block_length);
        return math_dbfs_to_amp (rms);
      }
      break;
    case TYPE_EVENT:
      {
        if (port->midi_events->num_events > 0 ||
            port->midi_events->num_queued_events > 0)
          {
            self->last_midi_trigger_time =
              g_get_real_time ();
            return 1.f;
          }
        else
          {
            gint64 time_diff =
              g_get_real_time () -
              self->last_midi_trigger_time;
            if ((float) time_diff < MAX_TIME)
              {
                return
                  1.f - (float) time_diff / MAX_TIME;
              }
            else
              return 0.f;
          }
      }
      break;
    case TYPE_CONTROL:
      return port->lv2_port->control;
      break;
    default:
      break;
    }
  return 0.f;
}

static void
set_port_value (
  InspectorPortWidget * self,
  float                 val)
{
  Port * port = self->port;
  switch (port->identifier.type)
    {
    case TYPE_CONTROL:
      port->lv2_port->control = val;
      port->base_value = val;
      break;
    default:
      break;
    }
}

static gboolean
bar_slider_tick_cb (
  GtkWidget *       widget,
  GdkFrameClock *   frame_clock,
  InspectorPortWidget * self)
{
  get_port_str (
    self->port,
    self->bar_slider->prefix);
  gtk_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

InspectorPortWidget *
inspector_port_widget_new (
  Port * port)
{
  InspectorPortWidget * self =
    g_object_new (
      INSPECTOR_PORT_WIDGET_TYPE,
      NULL);

  self->port = port;


  char str[200];
  int has_str = 0;
  if (!port->identifier.label)
    {
      g_warning ("No port label");
      goto inspector_port_new_end;
    }

  has_str = get_port_str (port, str);

  if (has_str)
    {
      float minf = 0.f, maxf = 1.f, zero = 0.f;
      int editable = 0;
      switch (port->identifier.type)
        {
        case TYPE_AUDIO:
          minf = 0.f;
          maxf = 2.f;
          zero = 0.0f;
          break;
        case TYPE_CV:
          minf = -1.f;
          maxf = 1.f;
          zero = 0.0f;
          break;
        case TYPE_EVENT:
          minf = 0.f;
          maxf = 1.f;
          zero = 0.f;
          break;
        case TYPE_CONTROL:
          /* FIXME this wont work with zrythm controls
           * like volume and pan */
          minf = port->lv2_port->lv2_control->minf;
          maxf = port->lv2_port->lv2_control->maxf;
          zero = minf;
          editable = 1;
          break;
        default:
          break;
        }
      self->bar_slider =
        _bar_slider_widget_new (
          BAR_SLIDER_TYPE_NORMAL,
          (float (*) (void *)) get_port_value,
          (void (*) (void *, float)) set_port_value,
          (void *) self, NULL, minf, maxf, -1, 20,
          zero, 0, 2, BAR_SLIDER_UPDATE_MODE_CURSOR,
          str, "");
      self->bar_slider->show_value = 0;
      self->bar_slider->editable = editable;
      gtk_container_add (
        GTK_CONTAINER (self),
        GTK_WIDGET (self->bar_slider));
      g_message (
        "%s: minf %f maxf %f zero %f",
        str, (double) minf, (double) maxf, (double) zero);

      /* keep drawing the bar slider */
      gtk_widget_add_tick_callback (
        GTK_WIDGET (self->bar_slider),
        (GtkTickCallback) bar_slider_tick_cb,
        self, NULL);
    }

  /* jack button */
#ifdef HAVE_JACK
  if (AUDIO_ENGINE->audio_backend ==
        AUDIO_BACKEND_JACK)
    {
      if (port->identifier.type ==
            TYPE_AUDIO ||
          port->identifier.type ==
            TYPE_EVENT)
        {
          self->jack =
            z_gtk_toggle_button_new_with_icon (
              "jack");
          gtk_widget_set_halign (
            GTK_WIDGET (self->jack),
            GTK_ALIGN_START);
          gtk_widget_set_valign (
            GTK_WIDGET (self->jack),
            GTK_ALIGN_CENTER);
          gtk_widget_set_margin_start (
            GTK_WIDGET (self->jack), 2);
          GtkStyleContext * context =
            gtk_widget_get_style_context (
              GTK_WIDGET (self->jack));
          gtk_style_context_add_class (
            context, "mini-button");
          gtk_widget_set_tooltip_text (
            GTK_WIDGET (self->jack),
            _("Expose port to JACK"));
          gtk_overlay_add_overlay (
            GTK_OVERLAY (self),
            GTK_WIDGET (self->jack));
          if (port->data &&
              port->internal_type ==
                INTERNAL_JACK_PORT)
            {
              gtk_toggle_button_set_active (
                self->jack, 1);
            }
          g_signal_connect (
            G_OBJECT (self->jack), "toggled",
            G_CALLBACK (on_jack_toggled), self);

          /* add some margin to clearly show the jack
           * button */
          /*GtkWidget * label =*/
            /*gtk_bin_get_child (*/
              /*GTK_BIN (self->menu_button));*/
          /*gtk_widget_set_margin_start (*/
            /*label, 12);*/
        }
    }
#endif

  self->double_click_gesture =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self->bar_slider)));

  self->right_click_gesture =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self->bar_slider)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (
      self->right_click_gesture),
      GDK_BUTTON_SECONDARY);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self->double_click_gesture), "pressed",
    G_CALLBACK (on_double_click), self);
  g_signal_connect (
    G_OBJECT (self->right_click_gesture), "pressed",
    G_CALLBACK (on_right_click), self);

inspector_port_new_end:

  return self;
}

static void
finalize (
  InspectorPortWidget * self)
{
  if (self->double_click_gesture)
    g_object_unref (self->double_click_gesture);
  if (self->right_click_gesture)
    g_object_unref (self->right_click_gesture);

  G_OBJECT_CLASS (
    inspector_port_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
inspector_port_widget_class_init (
  InspectorPortWidgetClass * klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) finalize;
}

static void
inspector_port_widget_init (
  InspectorPortWidget * self)
{
  gtk_widget_set_visible (
    GTK_WIDGET (self), 1);
}
