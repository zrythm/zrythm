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
#include "audio/port.h"
#include "gui/widgets/port_connections_button.h"
#include "gui/widgets/port_connections_popover.h"
#include "project.h"
#include "utils/gtk.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (PortConnectionsButtonWidget,
               port_connections_button_widget,
               GTK_TYPE_OVERLAY)

static void
on_jack_toggled (
  GtkWidget * widget,
  PortConnectionsButtonWidget * self)
{
  port_set_expose_to_jack (
    self->port,
    gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (widget)));
}

static int
refresh_popover (
  PortConnectionsPopoverWidget * popover)
{
  port_connections_popover_widget_refresh (popover);
  return G_SOURCE_REMOVE;
}

PortConnectionsButtonWidget *
port_connections_button_widget_new (
  Port * port)
{
  PortConnectionsButtonWidget * self =
    g_object_new (
      PORT_CONNECTIONS_BUTTON_WIDGET_TYPE,
      NULL);

  self->port = port;

  /* add menu button */
  self->menu_button =
    GTK_MENU_BUTTON (gtk_menu_button_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->menu_button), 1);
  gtk_container_add (
    GTK_CONTAINER (self),
    GTK_WIDGET (self->menu_button));
  PortConnectionsPopoverWidget * popover =
    port_connections_popover_widget_new (self);
  gtk_menu_button_set_popover (
    self->menu_button,
    GTK_WIDGET (popover));
  g_idle_add (
    (GSourceFunc) refresh_popover, popover);
  gtk_menu_button_set_direction (
    self->menu_button,
    GTK_ARROW_RIGHT);

  char str[200];
  int has_str = 0;
  if (!port->identifier.label)
    {
      g_warning ("No port label");
      goto port_connections_button_new_end;
    }

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
            str, "%s (%d)",
            port->identifier.label,
            port->num_srcs);
          has_str = 1;
        }
      else if (port->identifier.flow == FLOW_OUTPUT)
        {
          sprintf (
            str, "%s (%d)",
            port->identifier.label,
            port->num_dests);
          has_str = 1;
        }
    }

  if (has_str)
    {
      gtk_button_set_label (
        GTK_BUTTON (self->menu_button), str);
      GtkLabel * label =
        GTK_LABEL (
          gtk_bin_get_child (
            GTK_BIN (self->menu_button)));
      gtk_label_set_ellipsize (
        label, PANGO_ELLIPSIZE_END);
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
            GTK_WIDGET (self->jack), GTK_ALIGN_START);
          gtk_widget_set_valign (
            GTK_WIDGET (self->jack), GTK_ALIGN_CENTER);
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
          GtkWidget * label =
            gtk_bin_get_child (
              GTK_BIN (self->menu_button));
          gtk_widget_set_margin_start (
            label, 12);
        }
    }
#endif

port_connections_button_new_end:

  return self;
}

static void
port_connections_button_widget_class_init (
  PortConnectionsButtonWidgetClass * klass)
{
}

static void
port_connections_button_widget_init (
  PortConnectionsButtonWidget * self)
{
  gtk_widget_set_visible (
    GTK_WIDGET (self), 1);
}
