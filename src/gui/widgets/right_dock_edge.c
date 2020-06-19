/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/control_room.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/control_room.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/panel_file_browser.h"
#include "gui/widgets/plugin_browser.h"
#include "gui/widgets/right_dock_edge.h"
#include "project.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (RightDockEdgeWidget,
               right_dock_edge_widget,
               GTK_TYPE_BOX)

/*static DzlDockRevealer **/
/*get_revealer (*/
  /*RightDockEdgeWidget * self)*/
/*{*/
  /*[>return<]*/
    /*[>DZL_DOCK_REVEALER (<]*/
      /*[>gtk_widget_get_parent (<]*/
        /*[>gtk_widget_get_parent (<]*/
          /*[>GTK_WIDGET (self))));<]*/
  /*return NULL;*/
/*}*/

/*static void*/
/*on_divider_pos_changed (*/
  /*GObject    *gobject,*/
  /*GParamSpec *pspec,*/
  /*DzlDockRevealer * revealer)*/
/*{*/
  /*g_settings_set_int (*/
    /*S_UI, "right-panel-divider-position",*/
    /*dzl_dock_revealer_get_position (revealer));*/
/*}*/

void
right_dock_edge_widget_setup (
  RightDockEdgeWidget * self)
{
  foldable_notebook_widget_setup (
    self->right_notebook,
    MW_CENTER_DOCK->center_right_paned,
    GTK_POS_RIGHT);

  /* remember divider pos */
  /*DzlDockRevealer * revealer =*/
    /*get_revealer (self);*/
  /*dzl_dock_revealer_set_position (*/
    /*revealer,*/
    /*g_settings_get_int (*/
      /*S_UI, "right-panel-divider-position"));*/
  /*g_signal_connect (*/
    /*G_OBJECT (revealer), "notify::position",*/
    /*G_CALLBACK (on_divider_pos_changed), revealer);*/
  control_room_widget_setup (
    self->control_room, CONTROL_ROOM);
}

static void
right_dock_edge_widget_init (
  RightDockEdgeWidget * self)
{
  g_type_ensure (CONTROL_ROOM_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  GtkWidget * img;
  GtkBox * box;
  GtkNotebook * notebook =
    GTK_NOTEBOOK (self->right_notebook);

  /* add plugin browser */
  self->plugin_browser =
    plugin_browser_widget_new ();
  img =
    gtk_image_new_from_icon_name (
      "plugins",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_widget_set_tooltip_text (
    img, _("Plugin Browser"));
  box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_container_add (
    GTK_CONTAINER (box),
    GTK_WIDGET (self->plugin_browser));
  gtk_widget_set_visible (
    GTK_WIDGET (box), 1);
  gtk_notebook_prepend_page (
    notebook, GTK_WIDGET (box), img);

  /* add file browser */
  self->file_browser =
    panel_file_browser_widget_new ();
  GdkPixbuf * pixbuf =
    gtk_icon_theme_load_icon_for_scale (
      gtk_icon_theme_get_default (),
      /* the scale only accepts integers and we want
       * 24, but if we pass 24 and 1 a different
       * icon is loaded, so load the 12 icon and
       * scale it 2 times */
      "media-optical-audio",
      12, 2,
      GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
  img =
    gtk_image_new_from_pixbuf (pixbuf);
  gtk_widget_set_tooltip_text (
    img, _("File Browser"));
  box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_container_add (
    GTK_CONTAINER (box),
    GTK_WIDGET (self->file_browser));
  gtk_widget_set_visible (
    GTK_WIDGET (box), 1);
  gtk_notebook_append_page (
    notebook, GTK_WIDGET (box), img);

  /* add control room */
  self->control_room = control_room_widget_new ();
  img =
    gtk_image_new_from_icon_name (
      "audio-headphones",
      GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_set_tooltip_text (
    img, _("Control Room"));
  box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_container_add (
    GTK_CONTAINER (box),
    GTK_WIDGET (self->control_room));
  gtk_widget_set_visible (
    GTK_WIDGET (box), 1);
  gtk_notebook_append_page (
    notebook, GTK_WIDGET (box), img);

  /* add file browser button */
  img =
    gtk_image_new_from_icon_name (
      "media-optical-audio",
      GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_image_set_pixel_size (
    (GtkImage *) img, 32);
  GtkToolButton * tb =
    GTK_TOOL_BUTTON (
      gtk_tool_button_new (
        img, NULL));
  gtk_widget_show_all (
    GTK_WIDGET (tb));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (tb),
    _("Show file browser"));
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (tb),
    "win.show-file-browser");
  gtk_notebook_set_action_widget (
    notebook, GTK_WIDGET (tb),
    GTK_PACK_END);

  gtk_notebook_set_current_page (
    notebook, 0);
}

static void
right_dock_edge_widget_class_init (
  RightDockEdgeWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "right_dock_edge.ui");

  gtk_widget_class_set_css_name (
    klass, "right-dock-edge");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    RightDockEdgeWidget, \
    x)

  BIND_CHILD (right_notebook);
}
