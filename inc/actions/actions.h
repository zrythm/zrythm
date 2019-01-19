/*
 * actions/actions.h - Zrythm GActions
 *
 * Copyright (C) 2019 Alexandros Theodotou
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>

void
action_enable_window_action (
  const char * action_name);

void
action_disable_window_action (
  const char * action_name);

void
action_enable_app_action (
  const char * action_name);

void
action_disable_app_action (
  const char * action_name);

void
activate_about (GSimpleAction *action,
                GVariant      *variant,
                gpointer       user_data);

/**
 * Show preferences window.
 */
void
activate_preferences (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data);

void
activate_quit (GSimpleAction *action,
               GVariant      *variant,
               gpointer       user_data);

void
activate_shortcuts (GSimpleAction *action,
                    GVariant      *variant,
                    gpointer       user_data);

void
activate_zoom_in (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_zoom_out (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_new (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_iconify (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_open (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_save (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_save_as (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_export_as (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_properties (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_undo (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_redo (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_cut (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_copy (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_paste (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_delete (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_clear_selection (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_select_all (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_toggle_left_panel (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_toggle_right_panel (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_toggle_bot_panel (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_toggle_status_bar (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_fullscreen (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_manual (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_license (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_best_fit (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_original_size (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_snap_to_grid (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_snap_keep_offset (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_snap_events (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);
