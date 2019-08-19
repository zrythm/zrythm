/*
 * Copyright (C) 2019 Alexandros Theodotou
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
/**
 * Activate audition mode.
 */
void
activate_audition_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data);

/**
 * Activate select mode.
 */
void
activate_select_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data);

/**
 * Activate edit mode.
 */
void
activate_edit_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data);

/**
 * Activate cut mode.
 */
void
activate_cut_mode (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

/**
 * Activate eraser mode.
 */
void
activate_eraser_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data);

/**
 * Activate ramp mode.
 */
void
activate_ramp_mode (GSimpleAction *action,
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
activate_duplicate (GSimpleAction *action,
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

/**
 * Toggle timeline visibility.
 */
void
activate_toggle_top_panel (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

/**
 * Toggle drum mode in the piano roll.
 */
void
activate_toggle_drum_mode (
  GSimpleAction *action,
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
activate_chat (GSimpleAction *action,
                GVariant      *variant,
                gpointer       user_data);

void
activate_forums (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_bugreport (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_donate (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_loop_selection (GSimpleAction *action,
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
activate_create_audio_track (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_create_midi_track (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
activate_create_ins_track (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_create_bus_track (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_create_group_track (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
activate_duplicate_selected_tracks (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
activate_delete_selected_tracks (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_snap_events (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data);

void
activate_goto_prev_marker (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
activate_goto_next_marker (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
change_state_dim_output (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data);

void
change_state_loop (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data);

void
change_state_metronome (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data);

void
activate_quick_quantize (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
activate_quantize_options (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
activate_set_timebase_master (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
activate_set_transport_client (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);

void
activate_unlink_jack_transport (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data);
