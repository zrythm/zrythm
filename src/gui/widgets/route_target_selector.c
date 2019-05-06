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

#include "audio/channel.h"
#include "audio/track.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/route_target_selector.h"
#include "gui/widgets/route_target_selector_popover.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (RouteTargetSelectorWidget,
               route_target_selector_widget,
               GTK_TYPE_MENU_BUTTON)

static void
on_clicked (GtkButton * button,
            RouteTargetSelectorWidget * self)
{
  if (self->owner->channel->type == CT_MASTER)
    gtk_widget_set_visible (
      GTK_WIDGET (self->popover), 0);
}

static void
set_label (RouteTargetSelectorWidget * self)
{
  ChannelWidget * cw = self->owner;
  if (!(cw && cw->channel->output &&
          cw->channel->output->track))
    {
      g_message ("NONE");
      return;
    }

  Track * track = cw->channel->output->track;
  g_return_if_fail (track->name);

  gtk_label_set_text (
    self->label,
    track->name);
}

void
route_target_selector_widget_refresh (
  RouteTargetSelectorWidget * self)
{
  set_label (self);
}

void
route_target_selector_widget_setup (
  RouteTargetSelectorWidget * self,
  ChannelWidget *            owner)
{
  self->owner = owner;
  if (self->owner->channel->type == CT_MASTER)
    {
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (self->box),
        _("Routed to Stereo Out"));
    }
  else
    {
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (self->box),
        _("Select channel to route signal to"));
    }

  self->popover =
    route_target_selector_popover_widget_new (
      self);
  gtk_menu_button_set_popover (
    GTK_MENU_BUTTON (self),
    GTK_WIDGET (self->popover));

  set_label (self);
}

static void
route_target_selector_widget_class_init (
  RouteTargetSelectorWidgetClass * klass)
{
}

static void
route_target_selector_widget_init (
  RouteTargetSelectorWidget * self)
{
  self->box =
    GTK_BOX (gtk_box_new (
              GTK_ORIENTATION_HORIZONTAL, 0));
  self->img = GTK_IMAGE (
    resources_get_icon (
      ICON_TYPE_GNOME_BUILDER,
      "debug-step-out-symbolic-light.svg"));
  self->label =
    GTK_LABEL (
      gtk_label_new (_("Stereo Out")));
  gtk_box_pack_start (self->box,
                      GTK_WIDGET (self->img),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_NO_FILL,
                      1);
  gtk_box_pack_start (self->box,
                    GTK_WIDGET (self->label),
                    Z_GTK_NO_EXPAND,
                    Z_GTK_NO_FILL,
                    1);
  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (self->box));
  g_signal_connect (G_OBJECT (self),
                    "clicked",
                    G_CALLBACK (on_clicked),
                    self);

  gtk_widget_show_all (GTK_WIDGET (self));
}
