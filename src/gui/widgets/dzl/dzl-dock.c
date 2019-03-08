/* dzl-dock.c
 *
 * Copyright (C) 2016 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gui/widgets/dzl/dzl-dock.h"
/*#include "gui/widgets/dzl-resources.h"*/

G_DEFINE_INTERFACE (DzlDock, dzl_dock, GTK_TYPE_CONTAINER)

static void
dzl_dock_default_init (DzlDockInterface *iface)
{
  GdkScreen *screen;

  /*g_resources_register (dzl_get_resource ());*/

  screen = gdk_screen_get_default ();

  /*if (screen != NULL)*/
    /*gtk_icon_theme_add_resource_path (gtk_icon_theme_get_default (),*/
                                      /*"/org/gnome/dazzle/icons");*/

  g_object_interface_install_property (iface,
                                       g_param_spec_object ("manager",
                                                            "Manager",
                                                            "Manager",
                                                            DZL_TYPE_DOCK_MANAGER,
                                                            (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
}
