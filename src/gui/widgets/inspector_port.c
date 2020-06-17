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

#include "zrythm-config.h"

#include "audio/control_port.h"
#include "audio/engine.h"
#include "audio/midi.h"
#include "audio/midi_mapping.h"
#include "audio/port.h"
#include "gui/widgets/bar_slider.h"
#include "gui/widgets/bind_cc_dialog.h"
#include "gui/widgets/dialogs/port_info.h"
#include "gui/widgets/inspector_port.h"
#include "gui/widgets/port_connections_popover.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  InspectorPortWidget,
  inspector_port_widget,
  GTK_TYPE_OVERLAY)

#ifdef HAVE_JACK
static void
on_jack_toggled (
  GtkWidget * widget,
  InspectorPortWidget * self)
{
  port_set_expose_to_backend (
    self->port,
    gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (widget)));
}
#endif

/**
 * Gets the label for the bar slider (port string).
 *
 * @return if the string was filled in or not.
 */
static int
get_port_str (
  InspectorPortWidget * self,
  Port * port,
  char * buf)
{
  if (port->id.owner_type ==
        PORT_OWNER_TYPE_PLUGIN ||
      port->id.owner_type ==
        PORT_OWNER_TYPE_FADER ||
      port->id.owner_type ==
        PORT_OWNER_TYPE_TRACK ||
      port->id.owner_type ==
        PORT_OWNER_TYPE_PREFADER)
    {
      int num_midi_mappings = 0;
      MidiMapping ** mappings =
        midi_mappings_get_for_port (
          MIDI_MAPPINGS, port, &num_midi_mappings);
      if (mappings)
        free (mappings);

      const char * star =
        (num_midi_mappings > 0 ? "*" : "");
      char * port_label =
        g_markup_escape_text (port->id.label, -1);
      char color_prefix[60];
      sprintf (
        color_prefix,
        "<span foreground=\"%s\">",
        self->hex_color);
      char color_suffix[40] = "</span>";
      if (port->id.flow == FLOW_INPUT)
        {
          int num_unlocked_srcs =
            port_get_num_unlocked_srcs (port);
          sprintf (
            buf, "%s <small><sup>"
            "%s%d%s%s"
            "</sup></small>",
            port_label,
            color_prefix,
            num_unlocked_srcs, star,
            color_suffix);
          return 1;
        }
      else if (port->id.flow == FLOW_OUTPUT)
        {
          int num_unlocked_dests =
            port_get_num_unlocked_dests (port);
          sprintf (
            buf, "%s <small><sup>"
            "%s%d%s%s"
            "</sup></small>",
            port_label,
            color_prefix,
            num_unlocked_dests, star,
            color_suffix);
          return 1;
        }
      g_free (port_label);
    }
  else
    {
      g_warn_if_reached ();
    }
  return 0;
}

static void
on_view_info_activate (
  GtkMenuItem *         menuitem,
  InspectorPortWidget * self)
{
  PortInfoDialogWidget * dialog =
    port_info_dialog_widget_new (self->port);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
on_bind_midi_cc (
  GtkMenuItem *         menuitem,
  InspectorPortWidget * self)
{
  BindCcDialogWidget * dialog =
    bind_cc_dialog_widget_new ();

  int ret =
    gtk_dialog_run (GTK_DIALOG (dialog));

  if (ret == GTK_RESPONSE_ACCEPT)
    {
      if (dialog->cc[0])
        {
          midi_mappings_bind (
            MIDI_MAPPINGS, dialog->cc,
            NULL, self->port);
        }
    }
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
show_context_menu (
  InspectorPortWidget * self,
  gdouble               x,
  gdouble               y)
{
  GtkWidget *menu, *menuitem;

  menu = gtk_menu_new();

  if (self->port->id.type ==
        TYPE_CONTROL)
    {
      menuitem =
        gtk_menu_item_new_with_label (
          _("Bind MIDI CC"));
      g_signal_connect (
        menuitem, "activate",
        G_CALLBACK (on_bind_midi_cc), self);
      gtk_menu_shell_append (
        GTK_MENU_SHELL(menu), menuitem);
    }

  menuitem =
    gtk_menu_item_new_with_label (
      _("View info"));
  g_signal_connect (
    menuitem, "activate",
    G_CALLBACK (on_view_info_activate), self);
  gtk_menu_shell_append (
    GTK_MENU_SHELL (menu), menuitem);

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
on_popover_closed (
  GtkPopover * popover,
  InspectorPortWidget * self)
{
  get_port_str (
    self, self->port, self->bar_slider->prefix);
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
  /*gtk_popover_popdown (GTK_POPOVER (popover));*/
  gtk_widget_show_all (GTK_WIDGET (popover));

  g_signal_connect (
    G_OBJECT (popover), "closed",
    G_CALLBACK (on_popover_closed), self);
}

/** 250 ms */
/*static const float MAX_TIME = 250000.f;*/

static float
get_port_value (
  InspectorPortWidget * self)
{
  Port * port = self->port;
  switch (port->id.type)
    {
    case TYPE_AUDIO:
    case TYPE_EVENT:
      {
        float val, max;
        meter_get_value (
          self->meter, AUDIO_VALUE_FADER,
          &val, &max);
        return val;
      }
      break;
    case TYPE_CV:
      {
        return port->buf[0];
      }
      break;
    case TYPE_CONTROL:
      return
        control_port_real_val_to_normalized (
          port,
          port_get_control_value (port, 0));
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
  port_set_control_value (
    self->port,
    control_port_normalized_val_to_real (
      self->port, val), false, true);
}

static gboolean
bar_slider_tick_cb (
  GtkWidget *       widget,
  GdkFrameClock *   frame_clock,
  InspectorPortWidget * self)
{
  /* set bar slider label */
  /*get_port_str (*/
    /*self->port,*/
    /*self->bar_slider->prefix);*/

  /* if enough time passed, try to update the
   * tooltip */
  gint64 now = g_get_monotonic_time ();
  if (now - self->last_tooltip_change > 100000)
    {
      char str[2000];
      char full_designation[600];
      port_get_full_designation (
        self->port, full_designation);
      char * full_designation_escaped =
        g_markup_escape_text (full_designation, -1);
      const char * src_or_dest_str =
        self->port->id.flow == FLOW_INPUT ?
        _("Incoming signals") :
        _("Outgoing signals");
      sprintf (
        str, "<b>%s</b>\n"
        "%s: <b>%d</b>\n"
        "%s: <b>%f</b>\n"
        "%s: <b>%f</b> | "
        "%s: <b>%f</b>",
        full_designation_escaped,
        src_or_dest_str,
        self->port->id.flow == FLOW_INPUT ?
          self->port->num_srcs :
          self->port->num_dests,
        _("Current val"),
        (double) get_port_value (self),
        _("Min"), (double) self->minf,
        _("Max"), (double) self->maxf);
      g_free (full_designation_escaped);

      /* if the tooltip changed, update it */
      char * cur_tooltip =
        gtk_widget_get_tooltip_markup (widget);
      if (!string_is_equal (cur_tooltip, str, 0))
        {
          gtk_widget_set_tooltip_markup (
            widget, str);
        }

      /* cleanup */
      if (cur_tooltip)
        g_free (cur_tooltip);

      /* remember time */
      self->last_tooltip_change = now;
    }
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
  self->meter = meter_new_for_port (port);

  char str[200];
  int has_str = 0;
  if (!port->id.label)
    {
      g_warning ("No port label");
      goto inspector_port_new_end;
    }

  has_str = get_port_str (self, port, str);

  if (has_str)
    {
      float minf = port->minf;
      float maxf = port->maxf;
      if (port->id.type == TYPE_AUDIO)
        {
          /* use fader val for audio */
          maxf = 1.f;
        }
      float zerof = port->zerof;
      int editable = 0;
      int is_control = 0;
      if (port->id.type == TYPE_CONTROL)
        {
          editable = 1;
          is_control = 1;
        }
      self->bar_slider =
        _bar_slider_widget_new (
          BAR_SLIDER_TYPE_NORMAL,
          (float (*) (void *)) get_port_value,
          (void (*) (void *, float)) set_port_value,
          (void *) self, NULL,
          /* use normalized vals for controls */
          is_control ? 0.f : minf,
          is_control ? 1.f : maxf, -1, 20,
          is_control ? 0.f : zerof, 0, 2,
          UI_DRAG_MODE_CURSOR,
          str, "");
      self->bar_slider->show_value = 0;
      self->bar_slider->editable = editable;
      gtk_container_add (
        GTK_CONTAINER (self),
        GTK_WIDGET (self->bar_slider));
      self->minf = minf;
      self->maxf = maxf;
      self->zerof = zerof;
      strcpy (self->port_str, str);

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
      if (port->id.type ==
            TYPE_AUDIO ||
          port->id.type ==
            TYPE_EVENT)
        {
          self->jack =
            z_gtk_toggle_button_new_with_icon (
              "expose-to-jack");
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
  if (self->meter)
    meter_free (self->meter);

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

  ui_gdk_rgba_to_hex (
    &UI_COLORS->bright_orange, self->hex_color);
}
