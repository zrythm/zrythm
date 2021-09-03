/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * GAction actions.
 */

#include "zrythm-config.h"

#include "actions/actions.h"
#include "actions/arranger_selections.h"
#include "actions/undo_manager.h"
#include "actions/tracklist_selections.h"
#include "actions/range_action.h"
#include "audio/audio_function.h"
#include "audio/automation_function.h"
#include "audio/graph.h"
#include "audio/graph_export.h"
#include "audio/instrument_track.h"
#include "audio/midi.h"
#include "audio/midi_function.h"
#include "audio/router.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/clipboard.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/backend/timeline_selections.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel_slot.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/dialogs/create_project_dialog.h"
#include "gui/widgets/dialogs/about_dialog.h"
#include "gui/widgets/dialogs/object_color_chooser_dialog.h"
#include "gui/widgets/dialogs/string_entry_dialog.h"
#include "gui/widgets/event_viewer.h"
#include "gui/widgets/dialogs/export_dialog.h"
#include "gui/widgets/file_browser_window.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/header.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/log_viewer.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/preferences.h"
#include "gui/widgets/project_assistant.h"
#include "gui/widgets/dialogs/quantize_dialog.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/ruler.h"
#ifdef HAVE_GUILE
#include "gui/widgets/scripting_window.h"
#endif
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_bot_box.h"
#include "gui/widgets/timeline_minimap.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/toolbox.h"
#include "gui/widgets/tracklist.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/dialogs.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/localization.h"
#include "utils/log.h"
#include "utils/resources.h"
#include "utils/stack.h"
#include "utils/string.h"
#include "utils/system.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

static GtkWindow * file_browser_window = NULL;

#define DEFINE_SIMPLE(x) \
void \
x ( \
  GSimpleAction * action, \
  GVariant *      variant, \
  gpointer        user_data) \

void
actions_set_app_action_enabled (
  const char * action_name,
  const bool   enabled)
{
  GAction * action =
    g_action_map_lookup_action (
      G_ACTION_MAP (zrythm_app), action_name);
  g_simple_action_set_enabled (
    G_SIMPLE_ACTION (action), enabled);
}

void
activate_news (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  gtk_show_uri_on_window (
    GTK_WINDOW (MAIN_WINDOW),
    "https://mastodon.social/@zrythm",
    GDK_CURRENT_TIME, NULL);
}

void
activate_manual (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  LocalizationLanguage lang =
    (LocalizationLanguage)
    g_settings_get_enum (
      S_P_UI_GENERAL, "language");
  const char * lang_code =
    localization_get_string_code (lang);
#ifdef MANUAL_PATH
  char * path =
    g_strdup_printf (
      "file://%s/%s/index.html",
      MANUAL_PATH, lang_code);
#else
  char * path =
    g_strdup_printf (
      "https://manual.zrythm.org/%s/index.html",
      lang_code);
#endif
  gtk_show_uri_on_window (
    GTK_WINDOW (MAIN_WINDOW), path, 0, NULL);
  g_free (path);
}

void
activate_chat (GSimpleAction *action,
                GVariant      *variant,
                gpointer       user_data)
{
  gtk_show_uri_on_window (
    GTK_WINDOW (MAIN_WINDOW),
    "https://matrix.to/#/#zrythmdaw:matrix.org",
    0, NULL);
}

void
activate_donate (GSimpleAction *action,
                GVariant      *variant,
                gpointer       user_data)
{
  gtk_show_uri_on_window (
    GTK_WINDOW (MAIN_WINDOW),
    "https://liberapay.com/Zrythm", 0, NULL);
}

void
activate_bugreport (GSimpleAction *action,
                GVariant      *variant,
                gpointer       user_data)
{
#ifdef _WOE32
  ShellExecute (
    0, (LPCSTR)"open",
    (LPCSTR) NEW_ISSUE_URL,
    0, 0, SW_SHOWNORMAL);
#else
  gtk_show_uri_on_window (
    GTK_WINDOW (MAIN_WINDOW),
    NEW_ISSUE_URL,
    0,
    NULL);
#endif
}

void
activate_about (GSimpleAction *action,
                GVariant      *variant,
                gpointer       user_data)
{
  GtkDialog * dialog =
    GTK_DIALOG (
      about_dialog_widget_new (
        GTK_WINDOW (MAIN_WINDOW)));
  gtk_dialog_run (dialog);
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

/**
 * @note This is never called but keep it around
 * for shortcut window.
 */
void
activate_quit (
  GSimpleAction * action,
  GVariant      * variant,
  gpointer        user_data)
{
  g_application_quit (G_APPLICATION (user_data));
}

/**
 * Show preferences window.
 */
void
activate_preferences (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  if (MAIN_WINDOW->preferences_opened)
    {
      return;
    }

  GtkWindow * preferences_window =
    GTK_WINDOW (
      preferences_widget_new ());
  g_return_if_fail (preferences_window);
  gtk_window_set_transient_for (
    preferences_window,
    (GtkWindow *) MAIN_WINDOW);
  gtk_widget_show_all (
    (GtkWidget *) preferences_window);
  MAIN_WINDOW->preferences_opened = true;
}

/**
 * Show log window.
 */
void
activate_log (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  if (!LOG || !LOG->log_filepath)
    {
      g_message ("No log file found");
      return;
    }

  const char * cmd[3] = {
    OPEN_DIR_CMD, LOG->log_filepath, NULL };

  int ret =
    system_run_cmd_w_args (
      cmd, 4000, false, NULL, false);
  if (ret)
    {
      g_warning (
        "an error occured running %s %s",
        OPEN_DIR_CMD, LOG->log_filepath);
    }

#if 0
  if (GTK_IS_WINDOW (LOG->viewer))
    {
      gtk_window_present (
        GTK_WINDOW (LOG->viewer));
    }
  else
    {
      LOG->viewer = log_viewer_widget_new ();
      gtk_window_set_transient_for (
        GTK_WINDOW (LOG->viewer),
        GTK_WINDOW (MAIN_WINDOW));
      gtk_widget_set_visible (
        GTK_WIDGET (LOG->viewer), 1);
    }
#endif

  if (ZRYTHM_HAVE_UI && MAIN_WINDOW && MW_HEADER)
    {
      MW_HEADER->log_has_pending_warnings = false;
      EVENTS_PUSH (
        ET_LOG_WARNING_STATE_CHANGED, NULL);
    }
}

/**
 * Show scripting interface.
 */
void
activate_scripting_interface (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
#ifdef HAVE_GUILE
  ScriptingWindowWidget * widget =
    scripting_window_widget_new ();
  gtk_window_set_transient_for (
    GTK_WINDOW (widget),
    GTK_WINDOW (MAIN_WINDOW));
  gtk_widget_set_visible (GTK_WIDGET (widget), 1);
#else
  GtkWidget * dialog =
    gtk_message_dialog_new_with_markup (
      GTK_WINDOW (MAIN_WINDOW),
      GTK_DIALOG_MODAL |
        GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_INFO,
      GTK_BUTTONS_OK,
      _("Scripting extensibility with "
      "<a href=\"%s\">GNU Guile</a> "
      "is not enabled in your %s "
      "installation."),
      "https://www.gnu.org/software/guile",
      PROGRAM_NAME);
  gtk_window_set_transient_for (
    GTK_WINDOW (dialog),
    GTK_WINDOW (MAIN_WINDOW));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (GTK_WIDGET (dialog));
#endif
}

/**
 * Activate audition mode.
 */
void
activate_audition_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data)
{
  P_TOOL = TOOL_AUDITION;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

/**
 * Activate select mode.
 */
void
activate_select_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data)
{
  if (P_TOOL == TOOL_SELECT_NORMAL)
    P_TOOL = TOOL_SELECT_STRETCH;
  else
    P_TOOL = TOOL_SELECT_NORMAL;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

/**
 * Activate edit mode.
 */
void
activate_edit_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data)
{
  P_TOOL = TOOL_EDIT;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

/**
 * Activate cut mode.
 */
void
activate_cut_mode (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  P_TOOL = TOOL_CUT;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

/**
 * Activate eraser mode.
 */
void
activate_eraser_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data)
{
  P_TOOL = TOOL_ERASER;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

/**
 * Activate ramp mode.
 */
void
activate_ramp_mode (GSimpleAction *action,
                      GVariant      *variant,
                      gpointer       user_data)
{
  P_TOOL = TOOL_RAMP;
  EVENTS_PUSH (ET_TOOL_CHANGED, NULL);
}

/* FIXME rename to timeline zoom in */
void
activate_zoom_in (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  ruler_widget_set_zoom_level (
    MW_RULER,
    ruler_widget_get_zoom_level (MW_RULER)
    * RULER_ZOOM_LEVEL_MULTIPLIER);

  EVENTS_PUSH (ET_TIMELINE_VIEWPORT_CHANGED,
               NULL);
}

void
activate_zoom_out (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  double zoom_level =
    ruler_widget_get_zoom_level (MW_RULER)
    / RULER_ZOOM_LEVEL_MULTIPLIER;
  ruler_widget_set_zoom_level (
    MW_RULER, zoom_level);

  EVENTS_PUSH (ET_TIMELINE_VIEWPORT_CHANGED,
               NULL);
}

void
activate_best_fit (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  /* TODO */
  g_message ("ZOOMING IN");

  EVENTS_PUSH (
    ET_TIMELINE_VIEWPORT_CHANGED, NULL);
}

void
activate_original_size (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  ruler_widget_set_zoom_level (MW_RULER, 1.0);

  EVENTS_PUSH (
    ET_TIMELINE_VIEWPORT_CHANGED, NULL);
}

void
activate_loop_selection (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  if (PROJECT->last_selection ==
        SELECTION_TYPE_TIMELINE)
    {
      if (!arranger_selections_has_any (
            (ArrangerSelections *) TL_SELECTIONS))
        return;

      Position start, end;
      arranger_selections_get_start_pos (
        (ArrangerSelections *) TL_SELECTIONS,
        &start, F_GLOBAL);
      arranger_selections_get_end_pos (
        (ArrangerSelections *) TL_SELECTIONS,
        &end, F_GLOBAL);

      position_set_to_pos (
        &TRANSPORT->loop_start_pos,
        &start);
      position_set_to_pos (
        &TRANSPORT->loop_end_pos,
        &end);

      transport_update_position_frames (
        TRANSPORT);
      EVENTS_PUSH (
        ET_TIMELINE_LOOP_MARKER_POS_CHANGED,
        NULL);
    }
}

static void
on_project_new_finish (
  GtkAssistant * _assistant,
  gpointer       user_data)
{
  g_message (
    "%s (%s) called", __func__, __FILE__);

  ProjectAssistantWidget * pa =
    Z_PROJECT_ASSISTANT_WIDGET (_assistant);
  ZRYTHM->creating_project = 1;
  if (user_data) /* if cancel */
    {
      gtk_widget_destroy (GTK_WIDGET (_assistant));
      ZRYTHM->open_filename = NULL;
    }
  /* if we are loading a template and template
   * exists */
  else if (pa->load_template &&
           pa->template_selection &&
           pa->template_selection->
             filename[0] != '-')
    {
      ZRYTHM->open_filename =
        pa->template_selection->filename;
      g_message (
        "Creating project from template: %s",
        ZRYTHM->open_filename);
      ZRYTHM->opening_template = 1;
    }
  /* if we are loading a project */
  else if (!pa->load_template &&
           pa->project_selection)
    {
      ZRYTHM->open_filename =
        pa->project_selection->filename;
      g_message (
        "Loading project: %s",
        ZRYTHM->open_filename);
      ZRYTHM->creating_project = 0;
    }
  /* no selection, load blank project */
  else
    {
      ZRYTHM->open_filename = NULL;
      g_message (
        "Creating blank project");
    }

  /* if not loading a project, show dialog to
   * select directory and name */
  int quit = 0;
  if (ZRYTHM->creating_project)
    {
      CreateProjectDialogWidget * dialog =
        create_project_dialog_widget_new ();

      int ret =
        gtk_dialog_run (GTK_DIALOG (dialog));
      if (ret != GTK_RESPONSE_OK)
        quit = 1;
      gtk_widget_destroy (GTK_WIDGET (dialog));

      g_message (
        "%s (%s): creating project %s",
        __func__, __FILE__,
        ZRYTHM->create_project_path);
    }

  /* FIXME error if the assistant is deleted
   * here, setting invisible for now, but
   * eventually must be destroyed */
  gtk_widget_set_visible (GTK_WIDGET (pa), 0);

  if (quit)
    {
    }
  else
    {
      project_load (
        ZRYTHM->open_filename,
        ZRYTHM->opening_template);
    }
}

void
activate_new (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
#ifdef TRIAL_VER
  char msg[600];
  sprintf (
    msg,
    _("Creating new projects is disabled. Please "
    "restart %s to start a new project"),
    PROGRAM_NAME);
  ui_show_error_message (
    MAIN_WINDOW, msg);
  return;
#endif

  GtkWidget * dialog =
    gtk_dialog_new_with_buttons (
      _("Create new project"),
      GTK_WINDOW (MAIN_WINDOW),
      GTK_DIALOG_MODAL |
        GTK_DIALOG_DESTROY_WITH_PARENT,
      _("Yes"),
      GTK_RESPONSE_ACCEPT,
      _("No"),
      GTK_RESPONSE_REJECT,
      NULL);
  GtkWidget * label =
    gtk_label_new (
      _("Any unsaved changes to the current "
        "project will be lost. Continue?"));
  gtk_widget_set_visible (label, true);
  GtkWidget * content =
    gtk_dialog_get_content_area (
      GTK_DIALOG (dialog));
  gtk_container_add (
    GTK_CONTAINER (content), label);
  int res = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (GTK_WIDGET (dialog));
  if (res == GTK_RESPONSE_ACCEPT)
    {
      ProjectAssistantWidget * pa =
        project_assistant_widget_new (
          GTK_WINDOW (MAIN_WINDOW), 1);
      g_signal_connect (
        G_OBJECT (pa), "apply",
        G_CALLBACK (on_project_new_finish), NULL);
      g_signal_connect (
        G_OBJECT (pa), "cancel",
        G_CALLBACK (on_project_new_finish),
        (void *) 1);
      gtk_window_present (GTK_WINDOW (pa));
    }
}

static int
run_open_dialog (GtkDialog * dialog)
{
  int res = gtk_dialog_run (GTK_DIALOG (dialog));
  if (res == GTK_RESPONSE_ACCEPT)
    {
      GtkFileChooser *chooser =
        GTK_FILE_CHOOSER (dialog);
      char * filename =
        gtk_file_chooser_get_filename (chooser);
      g_message ("filename %s", filename);
      res = project_load (filename, 0);
      g_free (filename);
    }

  return res;
}

void
activate_open (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
#ifdef TRIAL_VER
  ui_show_error_message (
    MAIN_WINDOW,
    _("Loading is disabled in the trial version"));
  return;
#endif

  GtkDialog * dialog =
    dialogs_get_open_project_dialog (
      GTK_WINDOW (MAIN_WINDOW));

  int res = run_open_dialog (dialog);
  if (res == GTK_RESPONSE_ACCEPT)
    {
      char *filename =
        gtk_file_chooser_get_filename (
          GTK_FILE_CHOOSER (dialog));
      project_load (filename, 0);
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
activate_save (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
#ifdef TRIAL_VER
  ui_show_error_message (
    MAIN_WINDOW,
    _("Saving is disabled in the trial version"));
  return;
#endif

  if (!PROJECT->dir ||
      !PROJECT->title)
    {
      activate_save_as (
        action, variant, user_data);
      return;
    }
  g_message (
    "project dir: %s", PROJECT->dir);

  project_save (
    PROJECT, PROJECT->dir, F_NOT_BACKUP,
    ZRYTHM_F_NOTIFY, F_NO_ASYNC);
}

void
activate_save_as (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
#ifdef TRIAL_VER
  ui_show_error_message (
    MAIN_WINDOW,
    _("Saving is disabled in the trial version"));
  return;
#endif

  GtkWidget *dialog;
  GtkFileChooser *chooser;
  GtkFileChooserAction _action =
    GTK_FILE_CHOOSER_ACTION_SAVE;
  gint res;

  dialog =
    gtk_file_chooser_dialog_new (
      _("Save Project"),
      GTK_WINDOW (MAIN_WINDOW),
      _action,
      _("_Cancel"),
      GTK_RESPONSE_CANCEL,
      _("_Save"),
      GTK_RESPONSE_ACCEPT,
      NULL);
  chooser = GTK_FILE_CHOOSER (dialog);

  gtk_file_chooser_set_do_overwrite_confirmation (
    chooser, TRUE);

  char * project_file_path =
    project_get_path (
      PROJECT, PROJECT_PATH_PROJECT_FILE, false);
  char * str =
    io_path_get_parent_dir (
      project_file_path);
  g_free (project_file_path);
  gtk_file_chooser_select_filename (
    chooser,
    str);
  g_free (str);
  gtk_file_chooser_set_current_name (
    chooser, PROJECT->title);

  res = gtk_dialog_run (GTK_DIALOG (dialog));
  if (res == GTK_RESPONSE_ACCEPT)
    {
      char *filename;

      filename =
        gtk_file_chooser_get_filename (chooser);
      project_save (
        PROJECT, filename, F_NOT_BACKUP,
        ZRYTHM_F_NO_NOTIFY, F_NO_ASYNC);
      g_free (filename);
    }

  gtk_widget_destroy (dialog);
}

void
activate_iconify (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  gtk_window_iconify (GTK_WINDOW (MAIN_WINDOW));
}

void
activate_export_as (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  ExportDialogWidget * export =
    export_dialog_widget_new ();
  gtk_window_set_transient_for (
    GTK_WINDOW (export),
    GTK_WINDOW (MAIN_WINDOW));
  gtk_dialog_run (GTK_DIALOG (export));
  gtk_widget_destroy (GTK_WIDGET (export));
}

void
activate_export_graph (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
#ifdef HAVE_CGRAPH
  Graph * graph = graph_new (ROUTER);
  graph_setup (graph, false, false);

  char * exports_dir =
    project_get_path (
      PROJECT, PROJECT_PATH_EXPORTS, false);

  GtkWidget *dialog, *label, *content_area;
  GtkDialogFlags flags;

  // Create the widgets
  flags = GTK_DIALOG_DESTROY_WITH_PARENT;
  dialog =
    gtk_dialog_new_with_buttons (
      _("Export routing graph"),
      GTK_WINDOW (MAIN_WINDOW), flags,
      _("Image (PNG)"), 1,
      _("Image (SVG)"), 2,
      _("Dot graph"), 3,
      NULL);
  content_area =
    gtk_dialog_get_content_area (
      GTK_DIALOG (dialog));
  char lbl[600];
  sprintf (
    lbl,
    _("The graph will be exported to "
    "<a href=\"%s\">%s</a>\n"
    "Please select a format to export as"),
    exports_dir, exports_dir);
  label = gtk_label_new (NULL);
  gtk_label_set_markup (
    GTK_LABEL (label), lbl);
  gtk_widget_set_visible (label, true);
  gtk_label_set_justify (
    GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  z_gtk_widget_set_margin (label, 3);

  g_signal_connect (
    G_OBJECT (label), "activate-link",
    G_CALLBACK (z_gtk_activate_dir_link_func),
    NULL);

 // Add the label, and show everything we’ve added
  gtk_container_add (
    GTK_CONTAINER (content_area), label);

  int result = gtk_dialog_run (GTK_DIALOG (dialog));
  const char * filename = NULL;
  GraphExportType export_type;
  switch (result)
    {
      /* png */
      case 1:
        filename = "graph.png";
        export_type = GRAPH_EXPORT_PNG;
        break;
      /* svg */
      case 2:
        filename = "graph.svg";
        export_type = GRAPH_EXPORT_SVG;
        break;
      /* dot */
      case 3:
        filename = "graph.dot";
        export_type = GRAPH_EXPORT_DOT;
        break;
      default:
        gtk_widget_destroy (dialog);
        return;
    }
  gtk_widget_destroy (dialog);

  char * path =
    g_build_filename (
      exports_dir, filename, NULL);
  graph_export_as (
    graph, export_type, path);
  g_free (exports_dir);
  g_free (path);

  ui_show_notification (_("Graph exported"));

  graph_free (graph);
#endif
}

void
activate_properties (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

DEFINE_SIMPLE (activate_undo)
{
  if (undo_stack_is_empty (
        UNDO_MANAGER->undo_stack))
    return;

  GError * err = NULL;
  int ret = undo_manager_undo (UNDO_MANAGER, &err);
  if (ret != 0)
    {
      HANDLE_ERROR (
        err, "%s", _("Failed to undo"));
    }
}

DEFINE_SIMPLE (activate_redo)
{
  if (undo_stack_is_empty (
        UNDO_MANAGER->redo_stack))
    return;

  GError * err = NULL;
  int ret = undo_manager_redo (UNDO_MANAGER, &err);
  if (ret != 0)
    {
      HANDLE_ERROR (
        err, "%s", _("Failed to redo"));
    }

  EVENTS_PUSH (ET_UNDO_REDO_ACTION_DONE,
               NULL);
}

void
activate_cut (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  /* activate copy and then delete the selections */
  activate_copy (action, variant, user_data);

  ArrangerSelections * sel =
    project_get_arranger_selections_for_last_selection (
      PROJECT);

  switch (PROJECT->last_selection)
    {
    case SELECTION_TYPE_TIMELINE:
    case SELECTION_TYPE_EDITOR:
      if (sel && arranger_selections_has_any (sel))
        {
          GError * err = NULL;
          bool ret =
            arranger_selections_action_perform_delete (
              sel, &err);
          if (!ret)
            {
              HANDLE_ERROR (
                err, "%s",
                _("Failed to delete selections"));
            }
        }
      break;
    case SELECTION_TYPE_INSERT:
    case SELECTION_TYPE_MIDI_FX:
      if (mixer_selections_has_any (
            MIXER_SELECTIONS))
        {
          GError * err = NULL;
          bool ret =
            mixer_selections_action_perform_delete (
              MIXER_SELECTIONS,
              PORT_CONNECTIONS_MGR,
              &err);
          if (!ret)
            {
              HANDLE_ERROR (
                err, "%s",
                _("Failed to delete selections"));
            }
        }
      break;
    default:
      g_debug ("doing nothing");
      break;
    }
}


void
activate_copy (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  ArrangerSelections * sel =
    project_get_arranger_selections_for_last_selection (
      PROJECT);

  switch (PROJECT->last_selection)
    {
    case SELECTION_TYPE_TIMELINE:
    case SELECTION_TYPE_EDITOR:
      if (sel)
        {
          Clipboard * clipboard =
            clipboard_new_for_arranger_selections (
              sel, F_CLONE);
          if (clipboard->timeline_sel)
            {
              timeline_selections_set_vis_track_indices (
                clipboard->timeline_sel);
            }
          char * serialized =
            yaml_serialize (
              clipboard, &clipboard_schema);
          g_return_if_fail (serialized);
          gtk_clipboard_set_text (
            DEFAULT_CLIPBOARD,
            serialized, -1);
          clipboard_free (clipboard);
          g_free (serialized);
        }
      else
        {
          g_warning ("no selections to copy");
        }
      break;
    case SELECTION_TYPE_INSERT:
    case SELECTION_TYPE_MIDI_FX:
      if (mixer_selections_has_any (
            MIXER_SELECTIONS))
        {
          Clipboard * clipboard =
            clipboard_new_for_mixer_selections (
              MIXER_SELECTIONS, F_CLONE);
          char * serialized =
            yaml_serialize (
              clipboard, &clipboard_schema);
          g_return_if_fail (serialized);
          gtk_clipboard_set_text (
            DEFAULT_CLIPBOARD,
            serialized, -1);
          clipboard_free (clipboard);
          g_free (serialized);
        }
      break;
    default:
      g_warning ("not implemented yet");
      break;
    }
}

static void
on_clipboard_received (
  GtkClipboard *     gtk_clipboard,
  const char *       text,
  gpointer           data)
{
  if (!text)
    return;

  Clipboard * clipboard =
    (Clipboard *)
    yaml_deserialize (text, &clipboard_schema);
  if (!clipboard)
    {
      g_message (
        "invalid clipboard data received:\n%s",
        text);
      return;
    }

  ArrangerSelections * sel = NULL;
  MixerSelections * mixer_sel = NULL;
  switch (clipboard->type)
    {
    case CLIPBOARD_TYPE_TIMELINE_SELECTIONS:
    case CLIPBOARD_TYPE_MIDI_SELECTIONS:
    case CLIPBOARD_TYPE_AUTOMATION_SELECTIONS:
    case CLIPBOARD_TYPE_CHORD_SELECTIONS:
      sel = clipboard_get_selections (clipboard);
      break;
    case CLIPBOARD_TYPE_MIXER_SELECTIONS:
      mixer_sel = clipboard->mixer_sel;
      break;
    default:
      g_warn_if_reached ();
    }

  bool incompatible = false;
  if (sel)
    {
      arranger_selections_post_deserialize (sel);
      if (arranger_selections_can_be_pasted (sel))
        {
          arranger_selections_paste_to_pos (
            sel, PLAYHEAD, F_UNDOABLE);
        }
      else
        {
          g_message (
            "can't paste arranger selections:\n%s",
            text);
          incompatible = true;
        }
    }
  else if (mixer_sel)
    {
      ChannelSlotWidget * slot =
        MW_MIXER->paste_slot;
      mixer_selections_post_deserialize (mixer_sel);
      if (mixer_selections_can_be_pasted (
            mixer_sel, slot->track->channel,
            slot->type, slot->slot_index))
        {
          mixer_selections_paste_to_slot (
            mixer_sel, slot->track->channel,
            slot->type, slot->slot_index);
        }
      else
        {
          g_message (
            "can't paste mixer selections:\n%s",
            text);
          incompatible = true;
        }
    }

  if (incompatible)
    {
      ui_show_notification (
        _("Can't paste incompatible data"));
    }

  clipboard_free (clipboard);
}

void
activate_paste (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  g_message ("paste");
  gtk_clipboard_request_text (
    DEFAULT_CLIPBOARD,
    on_clipboard_received,
    NULL);
}

void
activate_delete (
  GSimpleAction * simple_action,
  GVariant      * variant,
  gpointer        user_data)
{
  g_message ("%s", __func__);

  ArrangerSelections * sel =
    project_get_arranger_selections_for_last_selection (
      PROJECT);

  if (sel &&
      arranger_selections_has_any (sel) &&
      !arranger_selections_contains_undeletable_object (
        sel))
    {
      GError * err = NULL;
      bool ret =
        arranger_selections_action_perform_delete (
          sel, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s",
            _("Failed to delete selections"));
        }
    }

  switch (PROJECT->last_selection)
    {
    case SELECTION_TYPE_TRACKLIST:
      g_message (
        "activating delete selected tracks");
      g_action_group_activate_action (
        G_ACTION_GROUP (MAIN_WINDOW),
        "delete-selected-tracks", NULL);
      break;
    case SELECTION_TYPE_INSERT:
    case SELECTION_TYPE_MIDI_FX:
      {
        GError * err = NULL;
        bool ret =
          mixer_selections_action_perform_delete (
            MIXER_SELECTIONS,
            PORT_CONNECTIONS_MGR,
            &err);
        if (!ret)
          {
            HANDLE_ERROR (
              err, "%s",
              _("Failed to delete plugins"));
          }
      }
      break;
    default:
      g_warning ("not implemented yet");
      break;
    }
}

void
activate_duplicate (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  ArrangerSelections * sel =
    project_get_arranger_selections_for_last_selection (
      PROJECT);

  if (sel && arranger_selections_has_any (sel))
    {
      double length =
        arranger_selections_get_length_in_ticks (
          sel);

      GError * err = NULL;
      bool ret =
        arranger_selections_action_perform_duplicate (
          sel, length, 0, 0, 0, 0, 0,
          F_NOT_ALREADY_MOVED, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s",
            _("Failed to duplicate selections"));
        }
    }
}

void
activate_clear_selection (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  ArrangerSelections * sel =
    project_get_arranger_selections_for_last_selection (
      PROJECT);

  switch (PROJECT->last_selection)
    {
    case SELECTION_TYPE_TIMELINE:
    case SELECTION_TYPE_EDITOR:
      if (sel)
        {
          arranger_selections_clear (
            sel, F_NO_FREE, F_PUBLISH_EVENTS);
        }
      break;
    case SELECTION_TYPE_TRACKLIST:
      tracklist_select_all (
        TRACKLIST, F_NO_SELECT, F_PUBLISH_EVENTS);
      break;
    case SELECTION_TYPE_INSERT:
    case SELECTION_TYPE_MIDI_FX:
      {
        Track * track =
          tracklist_selections_get_lowest_track (
            TRACKLIST_SELECTIONS);
        g_return_if_fail (
          IS_TRACK_AND_NONNULL (track));
        if (track_type_has_channel (track->type))
          {
            Channel * ch =
              track_get_channel (track);
            PluginSlotType slot_type =
              PLUGIN_SLOT_INSERT;
            if (PROJECT->last_selection ==
                  SELECTION_TYPE_MIDI_FX)
              {
                slot_type = PLUGIN_SLOT_MIDI_FX;
              }
            channel_select_all (
              ch, slot_type, F_NO_SELECT);
          }
      }
      break;
    default:
      g_debug ("%s: doing nothing", __func__);
    }
}

void
activate_select_all (
  GSimpleAction *action,
  GVariant *variant,
  gpointer user_data)
{
  ArrangerSelections * sel =
    project_get_arranger_selections_for_last_selection (
      PROJECT);

  switch (PROJECT->last_selection)
    {
    case SELECTION_TYPE_TIMELINE:
    case SELECTION_TYPE_EDITOR:
      if (sel)
        {
          arranger_selections_select_all (
            sel, F_PUBLISH_EVENTS);
        }
      break;
    case SELECTION_TYPE_TRACKLIST:
      tracklist_select_all (
        TRACKLIST, F_SELECT, F_PUBLISH_EVENTS);
      break;
    case SELECTION_TYPE_INSERT:
    case SELECTION_TYPE_MIDI_FX:
      {
        Track * track =
          tracklist_selections_get_lowest_track (
            TRACKLIST_SELECTIONS);
        g_return_if_fail (
          IS_TRACK_AND_NONNULL (track));
        if (track_type_has_channel (track->type))
          {
            Channel * ch =
              track_get_channel (track);
            PluginSlotType slot_type =
              PLUGIN_SLOT_INSERT;
            if (PROJECT->last_selection ==
                  SELECTION_TYPE_MIDI_FX)
              {
                slot_type = PLUGIN_SLOT_MIDI_FX;
              }
            channel_select_all (
              ch, slot_type, F_SELECT);
          }
      }
      break;
    default:
      g_debug ("%s: doing nothing", __func__);
    }
}

void
activate_toggle_left_panel (
  GSimpleAction * action,
  GVariant      * variant,
  gpointer        user_data)
{
  foldable_notebook_widget_toggle_visibility (
    MW_LEFT_DOCK_EDGE->inspector_notebook);
}

void
activate_toggle_right_panel (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  foldable_notebook_widget_toggle_visibility (
    MW_RIGHT_DOCK_EDGE->right_notebook);
}

void
activate_toggle_bot_panel (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  foldable_notebook_widget_toggle_visibility (
    MW_BOT_DOCK_EDGE->bot_notebook);
}

/**
 * Toggle timeline visibility.
 */
void
activate_toggle_top_panel (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  g_return_if_fail (
    MW_CENTER_DOCK && MW_MAIN_NOTEBOOK);
  GtkWidget * widget =
    (GtkWidget *) MW_MAIN_NOTEBOOK;
  gtk_widget_set_visible (
    widget, !gtk_widget_get_visible (widget));
}

void
activate_toggle_status_bar (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  gtk_widget_set_visible (
    GTK_WIDGET (MW_BOT_BAR),
    !gtk_widget_get_visible (
      GTK_WIDGET (MW_BOT_BAR)));
}

void
change_state_piano_roll_drum_mode (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  Track * tr =
    clip_editor_get_track (CLIP_EDITOR);
  g_return_if_fail (IS_TRACK_AND_NONNULL (tr));
  tr->drum_mode = enabled;

  EVENTS_PUSH (ET_DRUM_MODE_CHANGED, tr);
}

DEFINE_SIMPLE (activate_fullscreen)
{
  if (MAIN_WINDOW->is_fullscreen)
    {
      gtk_window_unfullscreen (
        GTK_WINDOW (MAIN_WINDOW));
    }
  else
    {
      gtk_window_fullscreen (
        GTK_WINDOW (MAIN_WINDOW));
    }
}

void
activate_snap_to_grid (
  GSimpleAction * action,
  GVariant      * _variant,
  gpointer         user_data)
{
  g_return_if_fail (_variant);

  gsize size;
  const char * variant =
    g_variant_get_string (_variant, &size);
  if (string_is_equal (variant, "timeline"))
    {
      SNAP_GRID_TIMELINE->snap_to_grid =
        !SNAP_GRID_TIMELINE->snap_to_grid;
      EVENTS_PUSH (
        ET_SNAP_GRID_OPTIONS_CHANGED,
        SNAP_GRID_TIMELINE);
    }
  else if (string_is_equal (variant, "editor"))
    {
      SNAP_GRID_EDITOR->snap_to_grid =
        !SNAP_GRID_EDITOR->snap_to_grid;
      EVENTS_PUSH (
        ET_SNAP_GRID_OPTIONS_CHANGED,
        SNAP_GRID_EDITOR);
    }
  else if (string_is_equal (variant, "global"))
    {
      if (PROJECT->last_selection ==
            SELECTION_TYPE_TIMELINE)
        {
          GError * err = NULL;
          bool ret =
            arranger_selections_action_perform_quantize (
              (ArrangerSelections *) TL_SELECTIONS,
              QUANTIZE_OPTIONS_TIMELINE, &err);
          if (!ret)
            {
              HANDLE_ERROR (
                err, "%s",
                _("Failed to quantize"));
            }
        }
    }
  else
    {
      g_return_if_reached ();
    }
}

void
activate_snap_keep_offset (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  g_return_if_fail (_variant);
  gsize size;
  const char * variant =
    g_variant_get_string (_variant, &size);

  if (string_is_equal (variant, "timeline"))
    {
      SNAP_GRID_TIMELINE->
        snap_to_grid_keep_offset =
          !SNAP_GRID_TIMELINE->
            snap_to_grid_keep_offset;
      EVENTS_PUSH (
        ET_SNAP_GRID_OPTIONS_CHANGED,
        SNAP_GRID_TIMELINE);
    }
  else if (string_is_equal (variant, "editor"))
    {
      SNAP_GRID_EDITOR->snap_to_grid_keep_offset =
        !SNAP_GRID_EDITOR->
          snap_to_grid_keep_offset;
      EVENTS_PUSH (
        ET_SNAP_GRID_OPTIONS_CHANGED,
        SNAP_GRID_EDITOR);
    }
  else
    g_return_if_reached ();
}

void
activate_snap_events (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  g_return_if_fail (_variant);
  gsize size;
  const char * variant =
    g_variant_get_string (_variant, &size);

  if (string_is_equal (variant, "timeline"))
    {
      SNAP_GRID_TIMELINE->snap_to_events =
        !SNAP_GRID_TIMELINE->snap_to_events;
      EVENTS_PUSH (
        ET_SNAP_GRID_OPTIONS_CHANGED,
        SNAP_GRID_TIMELINE);
    }
  else if (string_is_equal (variant, "editor"))
    {
      SNAP_GRID_EDITOR->snap_to_events =
        !SNAP_GRID_EDITOR->snap_to_events;
      EVENTS_PUSH (
        ET_SNAP_GRID_OPTIONS_CHANGED,
        SNAP_GRID_EDITOR);
    }
  else
    g_return_if_reached ();
}

DEFINE_SIMPLE (activate_create_audio_track)
{
  GError * err = NULL;
  bool ret =
    track_create_empty_with_action (
      TRACK_TYPE_AUDIO, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to create audio track"));
    }
}

DEFINE_SIMPLE (activate_create_midi_track)
{
  GError * err = NULL;
  bool ret =
    track_create_empty_with_action (
      TRACK_TYPE_MIDI, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to create MIDI track"));
    }
}

DEFINE_SIMPLE (activate_create_audio_bus_track)
{
  GError * err = NULL;
  bool ret =
    track_create_empty_with_action (
      TRACK_TYPE_AUDIO_BUS, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to create audio FX track"));
    }
}

DEFINE_SIMPLE (activate_create_midi_bus_track)
{
  GError * err = NULL;
  bool ret =
    track_create_empty_with_action (
      TRACK_TYPE_MIDI_BUS, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to create MIDI FX track"));
    }
}

DEFINE_SIMPLE (activate_create_audio_group_track)
{
  GError * err = NULL;
  bool ret =
    track_create_empty_with_action (
      TRACK_TYPE_AUDIO_GROUP, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to create audio group track"));
    }
}

DEFINE_SIMPLE (activate_create_midi_group_track)
{
  GError * err = NULL;
  bool ret =
    track_create_empty_with_action (
      TRACK_TYPE_MIDI_GROUP, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to create MIDI group track"));
    }
}

DEFINE_SIMPLE (activate_create_folder_track)
{
  GError * err = NULL;
  bool ret =
    track_create_empty_with_action (
      TRACK_TYPE_FOLDER, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to create folder track"));
    }
}

DEFINE_SIMPLE (activate_duplicate_selected_tracks)
{
  GError * err = NULL;
  bool ret =
    tracklist_selections_action_perform_copy (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR,
    TRACKLIST_SELECTIONS->tracks[0]->pos + 1,
    &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to duplicate tracks"));
    }
}

DEFINE_SIMPLE (activate_change_track_color)
{
  ObjectColorChooserDialogWidget * color_chooser =
    object_color_chooser_dialog_widget_new_for_tracklist_selections (
      TRACKLIST_SELECTIONS);
  object_color_chooser_dialog_widget_run (
    color_chooser);
}

DEFINE_SIMPLE (activate_goto_prev_marker)
{
  transport_goto_prev_marker (TRANSPORT);
}

DEFINE_SIMPLE (activate_goto_next_marker)
{
  transport_goto_next_marker (TRANSPORT);
}

DEFINE_SIMPLE (activate_play_pause)
{
  if (TRANSPORT_IS_ROLLING)
    {
      transport_request_pause (TRANSPORT);
      midi_panic_all (1);
    }
  else if (TRANSPORT_IS_PAUSED)
    {
      transport_request_roll (TRANSPORT);
    }
}

void
activate_delete_selected_tracks (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  g_message ("deleting selected tracks");

  GError * err = NULL;
  bool ret =
    tracklist_selections_action_perform_delete (
      TRACKLIST_SELECTIONS,
      PORT_CONNECTIONS_MGR,
      &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _("Failed to delete tracks"));
    }
}

DEFINE_SIMPLE (activate_hide_selected_tracks)
{
  g_message ("hiding selected tracks");
  tracklist_selections_toggle_visibility (
    TRACKLIST_SELECTIONS);
}

DEFINE_SIMPLE (activate_pin_selected_tracks)
{
  g_message ("pin/unpinning selected tracks");

  Track * track = TRACKLIST_SELECTIONS->tracks[0];
  bool pin = !track_is_pinned (track);

  GError * err = NULL;
  bool ret;
  if (pin)
    {
      ret =
        tracklist_selections_action_perform_pin (
          TRACKLIST_SELECTIONS,
          PORT_CONNECTIONS_MGR, &err);
    }
  else
    {
      ret =
        tracklist_selections_action_perform_unpin (
          TRACKLIST_SELECTIONS,
          PORT_CONNECTIONS_MGR, &err);
    }

  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _("Failed to pin/unpin tracks"));
    }
}

DEFINE_SIMPLE (activate_solo_selected_tracks)
{
  GError * err = NULL;
  bool ret =
    tracklist_selections_action_perform_edit_solo (
      TRACKLIST_SELECTIONS, F_SOLO, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _("Failed to solo tracks"));
    }
}

DEFINE_SIMPLE (activate_unsolo_selected_tracks)
{
  GError * err = NULL;
  bool ret =
    tracklist_selections_action_perform_edit_solo (
      TRACKLIST_SELECTIONS, F_NO_SOLO, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _("Failed to unsolo tracks"));
    }
}

DEFINE_SIMPLE (activate_mute_selected_tracks)
{
  GError * err = NULL;
  bool ret =
    tracklist_selections_action_perform_edit_mute (
      TRACKLIST_SELECTIONS, F_MUTE, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _("Failed to mute tracks"));
    }
}

DEFINE_SIMPLE (activate_unmute_selected_tracks)
{
  GError * err = NULL;
  bool ret =
    tracklist_selections_action_perform_edit_mute (
      TRACKLIST_SELECTIONS, F_NO_MUTE, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _("Failed to unmute tracks"));
    }
}

DEFINE_SIMPLE (activate_listen_selected_tracks)
{
  GError * err = NULL;
  bool ret =
    tracklist_selections_action_perform_edit_listen (
      TRACKLIST_SELECTIONS, F_LISTEN, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _("Failed to listen tracks"));
    }
}

DEFINE_SIMPLE (activate_unlisten_selected_tracks)
{
  GError * err = NULL;
  bool ret =
    tracklist_selections_action_perform_edit_listen (
      TRACKLIST_SELECTIONS, F_NO_LISTEN, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _("Failed to unlisten tracks"));
    }
}

DEFINE_SIMPLE (activate_enable_selected_tracks)
{
  GError * err = NULL;
  bool ret =
    tracklist_selections_action_perform_edit_enable (
      TRACKLIST_SELECTIONS, F_ENABLE, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _("Failed to enable tracks"));
    }
}

DEFINE_SIMPLE (activate_disable_selected_tracks)
{
  GError * err = NULL;
  bool ret =
    tracklist_selections_action_perform_edit_enable (
      TRACKLIST_SELECTIONS, F_NO_ENABLE, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _("Failed to disable tracks"));
    }
}

void
change_state_dim_output (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int dim = g_variant_get_boolean (value);

  g_message ("dim is %d", dim);

  g_simple_action_set_state (action, value);
}

void
change_state_loop (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  transport_set_loop (TRANSPORT, enabled);
}

void
change_state_metronome (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  bool enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  transport_set_metronome_enabled (
    TRANSPORT, enabled);
}

void
change_state_musical_mode (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  g_settings_set_boolean (
    S_UI, "musical-mode", enabled);
}

void
change_state_listen_notes (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  g_settings_set_boolean (
    S_UI, "listen-notes", enabled);
}

static void
do_quantize (
  const char * variant,
  bool         quick)
{
  g_debug ("quantize opts: %s", variant);

  if (string_is_equal (variant, "timeline")
      ||
      (string_is_equal (variant, "global")
       && PROJECT->last_selection
          == SELECTION_TYPE_TIMELINE))
    {
      if (quick)
        {
          GError * err = NULL;
          bool ret =
            arranger_selections_action_perform_quantize (
              (ArrangerSelections *) TL_SELECTIONS,
              QUANTIZE_OPTIONS_TIMELINE, &err);
          if (!ret)
            {
              HANDLE_ERROR (
                err, "%s",
                _("Failed to quantize"));
            }
        }
      else
        {
          QuantizeDialogWidget * quant =
            quantize_dialog_widget_new (
              QUANTIZE_OPTIONS_TIMELINE);
          gtk_window_set_transient_for (
            GTK_WINDOW (quant),
            GTK_WINDOW (MAIN_WINDOW));
          gtk_dialog_run (GTK_DIALOG (quant));
          gtk_widget_destroy (GTK_WIDGET (quant));
        }
    }
  else if (string_is_equal (variant, "editor")
           ||
           (string_is_equal (variant, "global")
            && PROJECT->last_selection
               == SELECTION_TYPE_EDITOR))
    {
      if (quick)
        {
          ArrangerSelections * sel =
            clip_editor_get_arranger_selections (
              CLIP_EDITOR);
          g_return_if_fail (sel);

          GError * err = NULL;
          bool ret =
            arranger_selections_action_perform_quantize (
              sel, QUANTIZE_OPTIONS_EDITOR, &err);
          if (!ret)
            {
              HANDLE_ERROR (
                err, "%s",
                _("Failed to quantize"));
            }
        }
      else
        {
          QuantizeDialogWidget * quant =
            quantize_dialog_widget_new (
              QUANTIZE_OPTIONS_EDITOR);
          gtk_window_set_transient_for (
            GTK_WINDOW (quant),
            GTK_WINDOW (MAIN_WINDOW));
          gtk_dialog_run (GTK_DIALOG (quant));
          gtk_widget_destroy (GTK_WIDGET (quant));
        }
    }
  else
    {
      ui_show_message_printf (
        MAIN_WINDOW, GTK_MESSAGE_WARNING,
        _("Must select either the timeline or the "
        "editor first. The current selection is "
        "%s"),
        selection_type_strings[
          PROJECT->last_selection].str);
    }
}

void
activate_quick_quantize (
  GSimpleAction *action,
  GVariant      * _variant,
  gpointer       user_data)
{
  g_return_if_fail (_variant);
  gsize size;
  const char * variant =
    g_variant_get_string (_variant, &size);
  do_quantize (variant, true);
}

void
activate_quantize_options (
  GSimpleAction *action,
  GVariant      *_variant,
  gpointer       user_data)
{
  g_return_if_fail (_variant);
  gsize size;
  const char * variant =
    g_variant_get_string (_variant, &size);
  do_quantize (variant, false);
}

void
activate_mute_selection (
  GSimpleAction *action,
  GVariant      *_variant,
  gpointer       user_data)
{
  g_return_if_fail (_variant);

  gsize size;
  const char * variant =
    g_variant_get_string (_variant, &size);
  ArrangerSelections * sel = NULL;
  if (string_is_equal (variant, "timeline"))
    {
      sel = (ArrangerSelections *) TL_SELECTIONS;
    }
  else if (string_is_equal (variant, "editor"))
    {
      sel = (ArrangerSelections *) MA_SELECTIONS;
    }
  else if (string_is_equal (variant, "global"))
    {
      sel =
        project_get_arranger_selections_for_last_selection (
          PROJECT);
    }
  else
    {
      g_return_if_reached ();
    }

  if (sel)
    {
      GError * err = NULL;
      bool ret =
        arranger_selections_action_perform_edit (
          sel, NULL,
          ARRANGER_SELECTIONS_ACTION_EDIT_MUTE,
          F_NOT_ALREADY_EDITED, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s",
            _("Failed to mute selections"));
        }
    }

  g_message ("mute/unmute selections");
}

DEFINE_SIMPLE (activate_merge_selection)
{
  g_message ("merge selections activated");

  if (TL_SELECTIONS->num_regions == 0)
    {
      ui_show_error_message (
        MAIN_WINDOW, _("No regions selected"));
      return;
    }
  if (!arranger_selections_all_on_same_lane (
        (ArrangerSelections *) TL_SELECTIONS))
    {
      ui_show_error_message (
        MAIN_WINDOW,
        _("Selections must be on the same lane"));
      return;
    }
  if (arranger_selections_contains_looped (
       (ArrangerSelections *) TL_SELECTIONS))
    {
      ui_show_error_message (
        MAIN_WINDOW,
        _("Cannot merge looped regions"));
      return;
    }
  if (TL_SELECTIONS->num_regions == 1)
    {
      /* nothing to do */
      return;
    }

  GError * err = NULL;
  bool ret =
    arranger_selections_action_perform_merge (
      (ArrangerSelections *) TL_SELECTIONS, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to merge selections"));
    }
}

void
activate_set_timebase_master (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  g_message ("set time base master");
}

void
activate_set_transport_client (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  g_message ("set transport client");
}

void
activate_unlink_jack_transport (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  g_message ("unlink jack transport");
}

void
activate_show_file_browser (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  if (file_browser_window)
    {
      gtk_window_close (
        file_browser_window);
      file_browser_window = FALSE;
    }
  else
    {
      file_browser_window =
        GTK_WINDOW (
          file_browser_window_widget_new ());
      g_return_if_fail (file_browser_window);
      gtk_window_set_transient_for (
        file_browser_window,
        (GtkWindow *) MAIN_WINDOW);
      gtk_widget_show_all (
        (GtkWidget *) file_browser_window);
    }
}

void
activate_toggle_timeline_event_viewer (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  if (!MW_TIMELINE_EVENT_VIEWER)
    return;

  int new_visibility =
    !gtk_widget_get_visible (
       GTK_WIDGET (MW_TIMELINE_EVENT_VIEWER));

  g_settings_set_boolean (
    S_UI, "timeline-event-viewer-visible",
    new_visibility);
  gtk_widget_set_visible (
    GTK_WIDGET (MW_TIMELINE_EVENT_VIEWER),
    new_visibility);
}

void
activate_toggle_editor_event_viewer (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  if (!MW_EDITOR_EVENT_VIEWER)
    return;

  int new_visibility =
    !gtk_widget_get_visible (
       GTK_WIDGET (MW_EDITOR_EVENT_VIEWER));

  g_settings_set_boolean (
    S_UI, "editor-event-viewer-visible",
    new_visibility);
  gtk_widget_set_visible (
    GTK_WIDGET (MW_EDITOR_EVENT_VIEWER),
    new_visibility);
}

DEFINE_SIMPLE (activate_insert_silence)
{
  if (!TRANSPORT->has_range)
    return;

  Position start, end;
  transport_get_range_pos (
    TRANSPORT, true, &start);
  transport_get_range_pos (
    TRANSPORT, false, &end);

  GError * err = NULL;
  bool ret =
    range_action_perform_insert_silence (
      &start, &end, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _("Failed to insert silence"));
    }
}

DEFINE_SIMPLE (activate_remove_range)
{
  if (!TRANSPORT->has_range)
    return;

  Position start, end;
  transport_get_range_pos (
    TRANSPORT, true, &start);
  transport_get_range_pos (
    TRANSPORT, false, &end);

  GError * err = NULL;
  bool ret =
    range_action_perform_remove (
      &start, &end, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to remove range"));
    }
}

DEFINE_SIMPLE (change_state_timeline_playhead_scroll_edges)
{
  int enabled = g_variant_get_boolean (variant);

  g_simple_action_set_state (action, variant);

  g_settings_set_boolean (
    S_UI, "timeline-playhead-scroll-edges", enabled);

  EVENTS_PUSH (
    ET_PLAYHEAD_SCROLL_MODE_CHANGED, NULL);
}

DEFINE_SIMPLE (change_state_timeline_playhead_follow)
{
  int enabled = g_variant_get_boolean (variant);

  g_simple_action_set_state (action, variant);

  g_settings_set_boolean (
    S_UI, "timeline-playhead-follow", enabled);

  EVENTS_PUSH (
    ET_PLAYHEAD_SCROLL_MODE_CHANGED, NULL);
}

DEFINE_SIMPLE (change_state_editor_playhead_scroll_edges)
{
  int enabled = g_variant_get_boolean (variant);

  g_simple_action_set_state (action, variant);

  g_settings_set_boolean (
    S_UI, "editor-playhead-scroll-edges", enabled);

  EVENTS_PUSH (
    ET_PLAYHEAD_SCROLL_MODE_CHANGED, NULL);
}

DEFINE_SIMPLE (change_state_editor_playhead_follow)
{
  int enabled = g_variant_get_boolean (variant);

  g_simple_action_set_state (action, variant);

  g_settings_set_boolean (
    S_UI, "editor-playhead-follow", enabled);

  EVENTS_PUSH (
    ET_PLAYHEAD_SCROLL_MODE_CHANGED, NULL);
}

/**
 * Common routine for applying undoable MIDI
 * functions.
 */
static void
do_midi_func (
  MidiFunctionType type)
{
  ArrangerSelections * sel =
    (ArrangerSelections *) MA_SELECTIONS;
  if (!arranger_selections_has_any (sel))
    {
      g_message ("no selections, doing nothing");
      return;
    }

  GError * err = NULL;
  bool ret =
    arranger_selections_action_perform_edit_midi_function (
      sel, type, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to apply MIDI function"));
    }
}

/**
 * Common routine for applying undoable automation
 * functions.
 */
static void
do_automation_func (
  AutomationFunctionType type)
{
  ArrangerSelections * sel =
    (ArrangerSelections *) AUTOMATION_SELECTIONS;
  if (!arranger_selections_has_any (sel))
    {
      g_message ("no selections, doing nothing");
      return;
    }

  GError * err = NULL;
  bool ret =
    arranger_selections_action_perform_edit_automation_function (
      sel, type, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to apply automation function"));
    }
}

/**
 * Common routine for applying undoable audio
 * functions.
 *
 * @param uri Plugin URI, if applying plugin.
 */
static void
do_audio_func (
  const AudioFunctionType  type,
  const char *             uri)
{
  g_return_if_fail (
    region_find (&CLIP_EDITOR->region_id));
  AUDIO_SELECTIONS->region_id =
    CLIP_EDITOR->region_id;
  ArrangerSelections * sel =
    (ArrangerSelections *) AUDIO_SELECTIONS;
  if (!arranger_selections_has_any (sel))
    {
      g_message ("no selections, doing nothing");
      return;
    }

  sel = arranger_selections_clone (sel);

  GError * err = NULL;
  bool ret;
  if (!arranger_selections_validate (sel))
    {
      goto free_audio_sel_and_return;
    }

  zix_sem_wait (&PROJECT->save_sem);

  ret =
    arranger_selections_action_perform_edit_audio_function (
      sel, type, uri, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to apply automation function"));
    }

  zix_sem_post (&PROJECT->save_sem);

free_audio_sel_and_return:
  arranger_selections_free (sel);
}

DEFINE_SIMPLE (activate_editor_function)
{
  size_t size;
  const char * str =
    g_variant_get_string (variant, &size);

  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  if (!region)
    return;

  switch (region->id.type)
    {
    case REGION_TYPE_MIDI:
      {
        if (string_is_equal (str, "crescendo"))
          {
            do_midi_func (MIDI_FUNCTION_CRESCENDO);
          }
        else if (string_is_equal (str, "current"))
          {
            do_midi_func (
              g_settings_get_int (
                S_UI, "midi-function"));
          }
        else if (string_is_equal (str, "flam"))
          {
            do_midi_func (MIDI_FUNCTION_FLAM);
          }
        else if (string_is_equal (str, "flip-horizontal"))
          {
            do_midi_func (MIDI_FUNCTION_FLIP_HORIZONTAL);
          }
        else if (string_is_equal (str, "flip-vertical"))
          {
            do_midi_func (MIDI_FUNCTION_FLIP_VERTICAL);
          }
        else if (string_is_equal (str, "legato"))
          {
            do_midi_func (MIDI_FUNCTION_LEGATO);
          }
        else if (string_is_equal (str, "portato"))
          {
            do_midi_func (MIDI_FUNCTION_PORTATO);
          }
        else if (string_is_equal (str, "staccato"))
          {
            do_midi_func (MIDI_FUNCTION_STACCATO);
          }
        else if (string_is_equal (str, "strum"))
          {
            do_midi_func (MIDI_FUNCTION_STRUM);
          }
        else
          {
            g_return_if_reached ();
          }
      }
      break;
    case REGION_TYPE_AUTOMATION:
      {
        if (string_is_equal (str, "current"))
          {
            do_automation_func (
              g_settings_get_int (
                S_UI, "automation-function"));
          }
        else if (string_is_equal (
                   str, "flip-horizontal"))
          {
            do_automation_func (
              AUTOMATION_FUNCTION_FLIP_HORIZONTAL);
          }
        else if (string_is_equal (
                   str, "flip-vertical"))
          {
            do_automation_func (
              AUTOMATION_FUNCTION_FLIP_VERTICAL);
          }
        else
          {
            g_return_if_reached ();
          }
      }
      break;
    case REGION_TYPE_AUDIO:
      {
        bool done = false;
        if (string_is_equal (str, "current"))
          {
            do_audio_func (
              g_settings_get_int (
                S_UI, "audio-function"), NULL);
            done = true;
          }

        for (int i = AUDIO_FUNCTION_INVERT;
             i < AUDIO_FUNCTION_CUSTOM_PLUGIN; i++)
          {
            char * audio_func_target =
              audio_function_get_action_target_for_type (
                i);
            if (string_is_equal (
                  str, audio_func_target))
              do_audio_func (i, NULL);
            g_free (audio_func_target);
            done = true;
          }

        g_return_if_fail (done);
      }
      break;
    default:
      break;
    }
}

DEFINE_SIMPLE (activate_editor_function_lv2)
{
  size_t size;
  const char * str =
    g_variant_get_string (variant, &size);

  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  if (!region)
    return;

  PluginDescriptor * descr =
    plugin_manager_find_plugin_from_uri (
      PLUGIN_MANAGER, str);
  g_return_if_fail (descr);

  switch (region->id.type)
    {
    case REGION_TYPE_MIDI:
      {
      }
      break;
    case REGION_TYPE_AUDIO:
      {
        do_audio_func (
          AUDIO_FUNCTION_CUSTOM_PLUGIN, str);
      }
      break;
    default:
      g_return_if_reached ();
      break;
    }
}

DEFINE_SIMPLE (
  activate_midi_editor_highlighting)
{
  size_t size;
  const char * str =
    g_variant_get_string (variant, &size);

#define SET_HIGHLIGHT(txt,hl_type) \
  if (string_is_equal (str, txt)) \
    { \
      piano_roll_set_highlighting ( \
        PIANO_ROLL, PR_HIGHLIGHT_##hl_type); \
    }

  SET_HIGHLIGHT ("none", NONE);
  SET_HIGHLIGHT ("chord", CHORD);
  SET_HIGHLIGHT ("scale", SCALE);
  SET_HIGHLIGHT ("both", BOTH);

#undef SET_HIGHLIGHT
}

DEFINE_SIMPLE (
  activate_rename_track_or_region)
{
  if (PROJECT->last_selection ==
        SELECTION_TYPE_TIMELINE)
    {
      g_message ("timeline");
      ArrangerSelections * sel =
        arranger_widget_get_selections (
          MW_TIMELINE);
      if (arranger_selections_get_num_objects (
            sel) == 1)
        {
          ArrangerObject * obj =
            arranger_selections_get_first_object (
              sel);
          if (obj->type ==
                ARRANGER_OBJECT_TYPE_REGION)
            {
              StringEntryDialogWidget * dialog =
                string_entry_dialog_widget_new (
                  _("Region name"), obj,
                  (GenericStringGetter)
                  arranger_object_get_name,
                  (GenericStringSetter)
                  arranger_object_set_name_with_action);
              gtk_widget_show_all (
                GTK_WIDGET (dialog));
            }
        }
    }
  else if (PROJECT->last_selection ==
             SELECTION_TYPE_TRACKLIST)
    {
      g_message ("TODO - track");
    }
}

DEFINE_SIMPLE (activate_add_region)
{
  if (TRACKLIST_SELECTIONS->num_tracks == 0)
    return;

  /*Track * track = TRACKLIST_SELECTIONS->tracks[0];*/

  /* TODO add region with default size */
}

DEFINE_SIMPLE (activate_go_to_start)
{
  Position pos;
  position_init (&pos);
  transport_set_playhead_pos (
    TRANSPORT, &pos);
}

void
change_state_show_automation_values (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  g_settings_set_boolean (
    S_UI, "show-automation-values", enabled);

  EVENTS_PUSH (
    ET_AUTOMATION_VALUE_VISIBILITY_CHANGED, NULL);
}

DEFINE_SIMPLE (activate_nudge_selection)
{
  size_t size;
  const char * str =
    g_variant_get_string (variant, &size);

  double ticks =
    ARRANGER_SELECTIONS_DEFAULT_NUDGE_TICKS;
  bool left = string_is_equal (str, "left");
  if (left)
    ticks = - ticks;

  ArrangerSelections * sel =
    project_get_arranger_selections_for_last_selection (
      PROJECT);
  if (!sel)
    return;

  if (sel->type == ARRANGER_SELECTIONS_TYPE_AUDIO)
    {
      do_audio_func (
        left
        ? AUDIO_FUNCTION_NUDGE_LEFT
        : AUDIO_FUNCTION_NUDGE_RIGHT,
        NULL);
      return;
    }

  Position start_pos;
  arranger_selections_get_start_pos (
    sel, &start_pos, F_GLOBAL);
  if (start_pos.ticks + ticks < 0)
    {
      g_message ("cannot nudge left");
      return;
    }

  GError * err = NULL;
  bool ret =
    arranger_selections_action_perform_move (
      sel, ticks, 0, 0, 0, 0, 0,
      F_NOT_ALREADY_MOVED, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to move selections"));
    }
}
