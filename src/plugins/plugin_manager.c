/*
 * plugins/plugin_manager.c - Manages plugins
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

#include "plugins/plugin_manager.h"

#include <gtk/gtk.h>

/**
 * Global variable
 */
Plugin_Manager * plugin_manager;

void
init_plugin_manager ()
{
    g_message ("Initializing plugin manager...");
    plugin_manager = malloc (sizeof (Plugin_Manager));

	/* Find all installed plugins */
	plugin_manager->world = lilv_world_new();
	lilv_world_load_all (plugin_manager->world);
	plugin_manager->plugins = lilv_world_get_all_plugins (
            plugin_manager->world);
}
