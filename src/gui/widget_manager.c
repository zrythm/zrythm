/*
 * gui/widget_manager.c - Manages GUI widgets
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include <stdlib.h>

#include "zrythm_app.h"
#include "gui/widget_manager.h"

/*#define GET_WIDGET_FROM_BUILDER(object_name) GTK_WIDGET ( \*/
        /*gtk_builder_get_object (builder, object_name))*/

/*#define REGISTER_WIDGET(object_name) \*/
  /*register_widget_from_builder (builder, \*/
                                /*object_name)*/


/*void*/
/*register_widgets (GtkBuilder * builder)*/
/*{*/
  /*[>REGISTER_WIDGET ("gapplicationwindow-main");<]*/
/*}*/

void
init_widget_manager ()
{
  Widget_Manager * widget_manager = calloc (1, sizeof (Widget_Manager));
  /*widget_manager->widgets = g_hash_table_new (g_str_hash,*/
                         /*g_str_equal);*/

  zrythm_system->widget_manager = widget_manager;

  widget_manager->entries[0].target = "PLUGIN_DESCR";
  widget_manager->entries[0].flags = GTK_TARGET_SAME_APP;
  widget_manager->entries[0].info = 0;
  widget_manager->num_entries = 1;
}

/*void*/
/*register_widget_from_builder (GtkBuilder * builder,*/
                              /*gchar       * key)*/
/*{*/
  /*GtkWidget * widget = GET_WIDGET_FROM_BUILDER (key);*/
  /*gboolean result = g_hash_table_insert (*/
      /*widget_manager->widgets,*/
      /*key,*/
      /*widget);*/
  /*if (!result)*/
    /*{*/
      /*g_error ("Failed registering widget for %s",*/
               /*key);*/
    /*}*/
/*}*/

