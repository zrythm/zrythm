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

/**
 * \file
 *
 * Zrythm test helper.
 */

#ifndef __TEST_HELPERS_ZRYTHM_H__
#define __TEST_HELPERS_ZRYTHM_H__

#include "audio/engine_dummy.h"
#include "project.h"
#include "zrythm.h"

#include <glib.h>

/**
 * @addtogroup tests
 *
 * @{
 */

/**
 * To be called by every test's main to initialize
 * Zrythm to default values.
 */
static void
test_helper_zrythm_init ()
{
  ZRYTHM = calloc (1, sizeof (Zrythm));
  ZRYTHM->symap = symap_new ();
  ZRYTHM->testing = 1;
  PROJECT = calloc (1, sizeof (Project));
  AUDIO_ENGINE->block_length = 256;

  transport_init (TRANSPORT, 0);
  engine_dummy_setup (AUDIO_ENGINE, 0);
}

/**
 * To be called after test_helper_zrythm_init() to
 * initialize the UI (GTK).
 */
static void
test_helper_zrythm_gui_init ()
{
  /* set theme */
  g_object_set (gtk_settings_get_default (),
                "gtk-theme-name",
                "Matcha-dark-sea",
                NULL);
  g_message ("set theme");

  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/breeze-icons");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/fork-awesome");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/font-awesome");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/zrythm");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/ext");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/gnome-builder");

  g_message ("set resource paths");

  // set default css provider
  GtkCssProvider * css_provider =
    gtk_css_provider_new();
  gtk_css_provider_load_from_resource (
    css_provider,
    "/org/zrythm/Zrythm/app/theme.css");
  gtk_style_context_add_provider_for_screen (
          gdk_screen_get_default (),
          GTK_STYLE_PROVIDER (css_provider),
          800);
  g_object_unref (css_provider);
  g_message ("set default css provider");
}

/**
 * @}
 */

#endif
