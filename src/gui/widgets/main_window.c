/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "project.h"
#include "actions/actions.h"
#include "audio/master_track.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/accel.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/center_dock_bot_box.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/editor_toolbar.h"
#include "gui/widgets/file_browser.h"
#include "gui/widgets/header_notebook.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/plugin_browser.h"
#include "gui/widgets/rack.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_toolbar.h"
#include "gui/widgets/top_bar.h"
#include "gui/widgets/tracklist.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MainWindowWidget,
               main_window_widget,
               GTK_TYPE_APPLICATION_WINDOW)

static void
on_main_window_destroy (MainWindowWidget * self,
                        gpointer user_data)
{
  if (PROJECT->loaded)
    g_application_quit (G_APPLICATION (zrythm_app));
}

static void
on_state_changed (MainWindowWidget * self,
                  GdkEventWindowState  * event,
                  gpointer    user_data)
{
  if (event->new_window_state &
        GDK_WINDOW_STATE_MAXIMIZED)
    self->is_maximized = 1;
  else
    self->is_maximized = 0;
}

MainWindowWidget *
main_window_widget_new (ZrythmApp * _app)
{
  MainWindowWidget * self = g_object_new (
    MAIN_WINDOW_WIDGET_TYPE,
    "application", G_APPLICATION (_app),
    "title", "Zrythm",
    NULL);

  return self;
}

static void
on_close_notification_clicked (
  GtkButton * btn,
  gpointer    user_data)
{
  gtk_revealer_set_reveal_child (
    GTK_REVEALER (MAIN_WINDOW->revealer),
    0);
}

void
main_window_widget_refresh (MainWindowWidget * self)
{
  header_notebook_widget_setup (MW_HEADER_NOTEBOOK,
                           PROJECT->title);

  /* setup center dock */
  center_dock_widget_setup (MW_CENTER_DOCK);

  editor_toolbar_widget_setup (
    MW_EDITOR_TOOLBAR);
  timeline_toolbar_widget_setup (
    MW_TIMELINE_TOOLBAR);

  /* setup piano roll */
  if (MW_BOT_DOCK_EDGE && MW_CLIP_EDITOR)
    {
      clip_editor_widget_setup (
        MW_CLIP_EDITOR);
    }

  // set icons
  GtkWidget * image =
    resources_get_icon (ICON_TYPE_ZRYTHM,
                        "zrythm.svg");
  gtk_window_set_icon (
          GTK_WINDOW (self),
          gtk_image_get_pixbuf (GTK_IMAGE (image)));

  /* setup top and bot bars */
  top_bar_widget_refresh (self->top_bar);
  bot_bar_widget_refresh (self->bot_bar);

  /* setup mixer */
  g_warn_if_fail (
    P_MASTER_TRACK && P_MASTER_TRACK->channel);
  mixer_widget_setup (
    MW_MIXER, P_MASTER_TRACK->channel);

  gtk_window_maximize (
    GTK_WINDOW (self));
}

static void
main_window_widget_class_init (MainWindowWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "main_window.ui");

  gtk_widget_class_bind_template_child (
    klass,
    MainWindowWidget,
    main_box);
  gtk_widget_class_bind_template_child (
    klass,
    MainWindowWidget,
    header_notebook);
  gtk_widget_class_bind_template_child (
    klass,
    MainWindowWidget,
    top_bar);
  gtk_widget_class_bind_template_child (
    klass,
    MainWindowWidget,
    center_box);
  gtk_widget_class_bind_template_child (
    klass,
    MainWindowWidget,
    center_dock);
  gtk_widget_class_bind_template_child (
    klass,
    MainWindowWidget,
    bot_bar);
  gtk_widget_class_bind_template_child (
    klass,
    MainWindowWidget,
    revealer);
  gtk_widget_class_bind_template_child (
    klass,
    MainWindowWidget,
    close_notification_button);
  gtk_widget_class_bind_template_child (
    klass,
    MainWindowWidget,
    notification_label);
  gtk_widget_class_bind_template_callback (
    klass,
    on_main_window_destroy);
  gtk_widget_class_bind_template_callback (
    klass,
    on_state_changed);

  gtk_widget_class_set_css_name (klass,
                                 "main-window");
}

static void
main_window_widget_init (MainWindowWidget * self)
{
  GActionEntry actions[] = {

    /* file menu */
    { "new", activate_new },
    { "open", activate_open },
    { "save", activate_save },
    { "save-as", activate_save_as },
    { "export-as", activate_export_as },
    { "properties", activate_properties },

    /* edit menu */
    { "undo", activate_undo },
    { "redo", activate_redo },
    { "cut", activate_cut },
    { "copy", activate_copy },
    { "paste", activate_paste },
    { "delete", activate_delete },
    { "duplicate", activate_duplicate },
    { "clear-selection", activate_clear_selection },
    { "select-all", activate_select_all },
    /* selection submenu */
    { "loop-selection", activate_loop_selection },

    /* view menu */
    { "toggle-left-panel",
      activate_toggle_left_panel },
    { "toggle-right-panel",
      activate_toggle_right_panel },
    { "toggle-bot-panel",
      activate_toggle_bot_panel },
    { "toggle-top-panel",
      activate_toggle_top_panel },
    { "toggle-status-bar",
      activate_toggle_status_bar },
    { "zoom-in", activate_zoom_in },
    { "zoom-out", activate_zoom_out },
    { "original-size", activate_original_size },
    { "best-fit", activate_best_fit },

    /* snapping, quantize */
    { "snap-to-grid", activate_snap_to_grid },
    { "snap-keep-offset",
      activate_snap_keep_offset },
    { "snap-events", activate_snap_events },
    { "quick-quantize",
      activate_quick_quantize, "s"},
    { "quantize-options", activate_quantize_options,
      "s" },

    /* track actions */
    { "create-audio-track",
      activate_create_audio_track },
    { "create-ins-track",
      activate_create_ins_track },
    { "create-bus-track",
      activate_create_bus_track },
    { "create-group-track",
      activate_create_group_track },
    { "delete-selected-tracks",
      activate_delete_selected_tracks },

    /* modes */
    { "select-mode", activate_select_mode },
    { "edit-mode", activate_edit_mode },
    { "cut-mode", activate_cut_mode },
    { "eraser-mode", activate_eraser_mode },
    { "ramp-mode", activate_ramp_mode },
    { "audition-mode", activate_audition_mode },

    /* transport */
    { "toggle-metronome", NULL, NULL,
      g_settings_get_int (
        S_UI, "metronome-enabled") ?
        "true" : "false",
      change_state_metronome },
    { "toggle-loop", NULL, NULL,
      "true", change_state_loop },
    { "goto-prev-marker",
      activate_goto_prev_marker },
    { "goto-next-marker",
      activate_goto_next_marker },

    /* tracks */
    { "delete-selected-tracks",
      activate_delete_selected_tracks },
    { "duplicate-selected-tracks",
      activate_duplicate_selected_tracks },

    /* piano roll */
    { "toggle-drum-mode",
      activate_toggle_drum_mode },

    /* control room */
    { "toggle-dim-output", NULL, NULL,
      "true", change_state_dim_output },
  };

  g_action_map_add_action_entries (
    G_ACTION_MAP (self),
    actions,
    G_N_ELEMENTS (actions),
    self);

  g_type_ensure (HEADER_NOTEBOOK_WIDGET_TYPE);
  g_type_ensure (TOP_BAR_WIDGET_TYPE);
  g_type_ensure (CENTER_DOCK_WIDGET_TYPE);
  g_type_ensure (BOT_BAR_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_connect (
    G_OBJECT (self->close_notification_button),
    "clicked",
    G_CALLBACK (on_close_notification_clicked),
    NULL);
}
