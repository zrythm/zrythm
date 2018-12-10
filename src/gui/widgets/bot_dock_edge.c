/*
 * gui/widgets/bot_dock_edge.c - Main window widget
 *
 * Copybot (C) 2018 Alexandros Theodotou
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

#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/connections.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/piano_roll_page.h"
#include "gui/widgets/rack.h"
#include "utils/gtk.h"
#include "utils/resources.h"

G_DEFINE_TYPE (BotDockEdgeWidget,
               bot_dock_edge_widget,
               GTK_TYPE_BOX)

static void
bot_dock_edge_widget_init (BotDockEdgeWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->mixer = mixer_widget_new ();
  self->piano_roll_page = piano_roll_page_widget_new ();
  self->connections = connections_widget_new ();
  self->rack = rack_widget_new ();
  GtkWidget * box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (box),
                      resources_get_icon ("piano_roll.svg"),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_NO_FILL,
                      2);
  gtk_box_pack_end (GTK_BOX (box),
                    gtk_label_new ("Piano Roll"),
                    Z_GTK_NO_EXPAND,
                    Z_GTK_NO_FILL,
                    2);
  gtk_widget_show_all (box);
  gtk_notebook_prepend_page (
    self->bot_notebook,
    GTK_WIDGET (self->piano_roll_page),
    box);
  gtk_notebook_append_page (self->bot_notebook,
                            GTK_WIDGET (self->rack),
                            gtk_label_new ("Rack"));
  gtk_notebook_append_page (self->bot_notebook,
                            GTK_WIDGET (self->connections),
                            gtk_label_new ("Connections"));
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (box),
                      resources_get_icon ("mixer.svg"),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_NO_FILL,
                      2);
  gtk_box_pack_end (GTK_BOX (box),
                    gtk_label_new ("Mixer"),
                    Z_GTK_NO_EXPAND,
                    Z_GTK_NO_FILL,
                    2);
  gtk_widget_show_all (box);
  gtk_notebook_append_page (self->bot_notebook,
                            GTK_WIDGET (self->mixer),
                            box);
  gtk_widget_show_all (GTK_WIDGET (self->bot_notebook));

  /* set icons */
  gtk_button_set_image (
    GTK_BUTTON (self->mixer->channels_add),
    resources_get_icon ("plus.svg"));
}

static void
bot_dock_edge_widget_class_init (BotDockEdgeWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass,
    "bot_dock_edge.ui");

  gtk_widget_class_bind_template_child (
    klass,
    BotDockEdgeWidget,
    bot_notebook);
}
