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
#include "actions/copy_tracks_action.h"
#include "actions/create_tracks_action.h"
#include "actions/delete_tracks_action.h"
#include "audio/graph.h"
#include "audio/graph_export.h"
#include "audio/instrument_track.h"
#include "audio/midi.h"
#include "audio/router.h"
#include "audio/track.h"
#include "audio/transport.h"
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
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/create_project_dialog.h"
#include "gui/widgets/dialogs/about_dialog.h"
#include "gui/widgets/event_viewer.h"
#include "gui/widgets/export_dialog.h"
#include "gui/widgets/file_browser_window.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/log_viewer.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/preferences.h"
#include "gui/widgets/project_assistant.h"
#include "gui/widgets/quantize_dialog.h"
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
#include "project.h"
#include "settings/settings.h"
#include "utils/dialogs.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/localization.h"
#include "utils/log.h"
#include "utils/resources.h"
#include "utils/stack.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

static GtkWindow * file_browser_window = NULL;

void
action_enable_window_action (
  const char * action_name)
{
  GAction * action =
    g_action_map_lookup_action (
      G_ACTION_MAP (MAIN_WINDOW),
      action_name);
  g_simple_action_set_enabled (
    G_SIMPLE_ACTION (action),
    1);
}

void
action_disable_window_action (
  const char * action_name)
{
  GAction * action =
    g_action_map_lookup_action (
      G_ACTION_MAP (MAIN_WINDOW),
      action_name);
  g_simple_action_set_enabled (
    G_SIMPLE_ACTION (action),
    0);
}

void
action_enable_app_action (
  const char * action_name)
{
  GAction * action =
    g_action_map_lookup_action (
      G_ACTION_MAP (ZRYTHM),
      action_name);
  g_simple_action_set_enabled (
    G_SIMPLE_ACTION (action),
    1);
}

void
action_disable_app_action (
  const char * action_name)
{
  GAction * action =
    g_action_map_lookup_action (
      G_ACTION_MAP (ZRYTHM),
      action_name);
  g_simple_action_set_enabled (
    G_SIMPLE_ACTION (action),
    0);
}

void
activate_news (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  gtk_show_uri_on_window (
    GTK_WINDOW (MAIN_WINDOW),
    "https://forum.zrythm.org/c/"
    "News-and-blog-entries", 0, NULL);
}

void
activate_manual (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
#ifdef MANUAL_PATH
  LocalizationLanguage lang =
    (LocalizationLanguage)
    g_settings_get_enum (
      S_P_UI_GENERAL, "language");
  const char * lang_code =
    localization_get_string_code (lang);
  char * path =
    g_strdup_printf (
      "file://%s/%s/index.html",
      MANUAL_PATH, lang_code);
  gtk_show_uri_on_window (
    GTK_WINDOW (MAIN_WINDOW), path, 0, NULL);
  g_free (path);
#else
  gtk_show_uri_on_window (
    GTK_WINDOW (MAIN_WINDOW),
    "https://manual.zrythm.org", 0, NULL);
#endif
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

void
activate_quit (GSimpleAction *action,
               GVariant      *variant,
               gpointer       user_data)
{
  g_application_quit (G_APPLICATION (user_data));
}

void
activate_shortcuts (GSimpleAction *action,
                    GVariant      *variant,
                    gpointer       user_data)
{
  GtkBuilder *builder;
  GtkWidget *overlay;

  builder =
    gtk_builder_new_from_resource (
      RESOURCE_PATH TEMPLATE_PATH "shortcuts.ui");
  overlay =
    GTK_WIDGET (
      gtk_builder_get_object (builder, "shortcuts-builder"));
  gtk_window_set_transient_for (
    GTK_WINDOW (overlay),
    GTK_WINDOW (MAIN_WINDOW));
  gtk_widget_show (overlay);
  g_object_unref (builder);
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
    MW_RULER, MW_RULER->zoom_level * 1.3);

  EVENTS_PUSH (ET_TIMELINE_VIEWPORT_CHANGED,
               NULL);
}

void
activate_zoom_out (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  double zoom_level = MW_RULER->zoom_level / 1.3;
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

  EVENTS_PUSH (ET_TIMELINE_VIEWPORT_CHANGED,
               NULL);
}

void
activate_original_size (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  MW_RULER->zoom_level =
    (double) RW_DEFAULT_ZOOM_LEVEL;

  EVENTS_PUSH (ET_TIMELINE_VIEWPORT_CHANGED,
               NULL);
}

void
activate_loop_selection (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  if (MAIN_WINDOW->last_focused ==
      GTK_WIDGET (MW_TIMELINE))
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

  GtkWidget *dialog;
  GtkDialogFlags flags =
    GTK_DIALOG_MODAL |
    GTK_DIALOG_DESTROY_WITH_PARENT;
  dialog =
    gtk_dialog_new_with_buttons (
      _("Create new project"),
      GTK_WINDOW (MAIN_WINDOW),
      flags,
      _("Yes"),
      GTK_RESPONSE_ACCEPT,
      _("No"),
      GTK_RESPONSE_REJECT,
      NULL);
  GtkWidget * label =
    gtk_label_new (
      _("Any unsaved changes to the current "
        "project will be lost. Continue?"));
  gtk_widget_set_visible (label, 1);
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
activate_save (GSimpleAction *action,
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
    PROJECT, PROJECT->dir, 0, 1, F_NO_ASYNC);
}

void
activate_save_as (GSimpleAction *action,
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
        PROJECT, filename, 0, 1, F_NO_ASYNC);
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
      _("Dot graph"), 2,
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
      /* dot */
      case 2:
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

void
activate_undo (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  if (undo_stack_is_empty (
        UNDO_MANAGER->undo_stack))
    return;
  undo_manager_undo (UNDO_MANAGER);

  EVENTS_PUSH (ET_UNDO_REDO_ACTION_DONE,
               NULL);
}

void
activate_redo (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  if (undo_stack_is_empty (
        UNDO_MANAGER->redo_stack))
    return;
  undo_manager_redo (UNDO_MANAGER);

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
    main_window_get_last_focused_arranger_selections (
      MAIN_WINDOW);

  if (sel && arranger_selections_has_any (sel))
    {
      UndoableAction * ua =
        arranger_selections_action_new_delete (
          sel);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
}


void
activate_copy (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  if (MAIN_WINDOW_LAST_FOCUSED_IS (
        MW_TIMELINE) ||
      MAIN_WINDOW_LAST_FOCUSED_IS (
        MW_PINNED_TIMELINE))
    {
      TimelineSelections * ts =
        (TimelineSelections *)
        arranger_selections_clone (
          (ArrangerSelections *) TL_SELECTIONS);
      timeline_selections_set_vis_track_indices (
        ts);
      char * serialized =
        timeline_selections_serialize (ts);
      g_return_if_fail (serialized);
      g_message ("copying timeline selections");
      gtk_clipboard_set_text (
        DEFAULT_CLIPBOARD,
        serialized, -1);
      arranger_selections_free (
        (ArrangerSelections *) ts);
      g_free (serialized);
    }
  else if (MAIN_WINDOW_LAST_FOCUSED_IS (
             MW_MIDI_ARRANGER) ||
           MAIN_WINDOW_LAST_FOCUSED_IS (
             MW_MIDI_MODIFIER_ARRANGER))
    {
      MidiArrangerSelections * mas =
        (MidiArrangerSelections *)
        arranger_selections_clone (
          (ArrangerSelections *) MA_SELECTIONS);
      char * serialized =
        midi_arranger_selections_serialize (mas);
      g_return_if_fail (serialized);
      g_message ("copying midi selections");
      gtk_clipboard_set_text (
        DEFAULT_CLIPBOARD,
        serialized, -1);
      arranger_selections_free (
        (ArrangerSelections *) mas);
      g_free (serialized);
    }
  else if (MAIN_WINDOW_LAST_FOCUSED_IS (
             MW_CHORD_ARRANGER))
    {
      ChordSelections * cs =
        (ChordSelections *)
        arranger_selections_clone (
          (ArrangerSelections *)
          CHORD_SELECTIONS);
      char * serialized =
        chord_selections_serialize (cs);
      g_return_if_fail (serialized);
      g_message ("copying chord selections");
      gtk_clipboard_set_text (
        DEFAULT_CLIPBOARD,
        serialized, -1);
      arranger_selections_free (
        (ArrangerSelections *) cs);
      g_free (serialized);
    }
  else if (MAIN_WINDOW_LAST_FOCUSED_IS (
             MW_AUTOMATION_ARRANGER))
    {
      AutomationSelections * cs =
        (AutomationSelections *)
        arranger_selections_clone (
          (ArrangerSelections *)
          AUTOMATION_SELECTIONS);
      char * serialized =
        automation_selections_serialize (cs);
      g_return_if_fail (serialized);
      g_message ("copying automation selections");
      gtk_clipboard_set_text (
        DEFAULT_CLIPBOARD,
        serialized, -1);
      arranger_selections_free (
        (ArrangerSelections *) cs);
      g_free (serialized);
    }
}

static void
on_timeline_clipboard_received (
  GtkClipboard *     clipboard,
  const char *       text,
  gpointer           data)
{
  TimelineSelections * ts =
    timeline_selections_deserialize (text);
  arranger_selections_post_deserialize (
    (ArrangerSelections *) ts);

  if (timeline_selections_can_be_pasted (
        ts, PLAYHEAD,
        TRACKLIST_SELECTIONS->tracks[0]->pos))
    {
      g_message ("pasting timeline selections");
      timeline_selections_paste_to_pos (
        ts, PLAYHEAD);

      /* create action to make it undoable */
      /* (by now the TL_SELECTIONS should have
       * only the pasted items selected) */
      UndoableAction * ua =
        (UndoableAction *)
        arranger_selections_action_new_create (
          (ArrangerSelections *)
          TL_SELECTIONS);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
  else
    {
      ui_show_notification (
        _("Can't paste selections"));
    }
  arranger_selections_free (
    (ArrangerSelections *) ts);
}

static void
on_midi_clipboard_received (
  GtkClipboard *     clipboard,
  const char *       text,
  gpointer           data)
{
  MidiArrangerSelections * mas =
    midi_arranger_selections_deserialize (text);
  arranger_selections_post_deserialize (
    (ArrangerSelections *) mas);

  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  if (midi_arranger_selections_can_be_pasted (
        mas, PLAYHEAD, region))
    {
      g_message ("pasting midi selections");
      midi_arranger_selections_paste_to_pos (
        mas, PLAYHEAD);

      /* create action to make it undoable */
      /* (by now the TL_SELECTIONS should have
       * only the pasted items selected) */
      UndoableAction * ua =
        (UndoableAction *)
        arranger_selections_action_new_create (
          (ArrangerSelections *)
          MA_SELECTIONS);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
  else
    {
      ui_show_notification (
        _("Can't paste selections"));
    }
  arranger_selections_free (
    (ArrangerSelections *) mas);
}
static void
on_chord_clipboard_received (
  GtkClipboard *     clipboard,
  const char *       text,
  gpointer           data)
{
  ChordSelections * mas =
    chord_selections_deserialize (text);
  arranger_selections_post_deserialize (
    (ArrangerSelections *) mas);

  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  if (chord_selections_can_be_pasted (
        mas, PLAYHEAD, region))
    {
      g_message ("pasting midi selections");
      chord_selections_paste_to_pos (
        mas, PLAYHEAD);

      /* create action to make it undoable */
      /* (by now the TL_SELECTIONS should have
       * only the pasted items selected) */
      UndoableAction * ua =
        (UndoableAction *)
        arranger_selections_action_new_create (
          (ArrangerSelections *)
          CHORD_SELECTIONS);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
  else
    {
      ui_show_notification (
        _("Can't paste selections"));
    }
  arranger_selections_free (
    (ArrangerSelections *) mas);
}

static void
on_automation_clipboard_received (
  GtkClipboard *     clipboard,
  const char *       text,
  gpointer           data)
{
  AutomationSelections * as =
    automation_selections_deserialize (text);
  arranger_selections_post_deserialize (
    (ArrangerSelections *) as);

  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  if (automation_selections_can_be_pasted (
        as, PLAYHEAD, region))
    {
      g_message ("pasting automation selections");
      automation_selections_paste_to_pos (
        as, PLAYHEAD);

      /* create action to make it undoable */
      /* (by now the TL_SELECTIONS should have
       * only the pasted items selected) */
      UndoableAction * ua =
        (UndoableAction *)
        arranger_selections_action_new_create (
          (ArrangerSelections *)
          AUTOMATION_SELECTIONS);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
  else
    {
      ui_show_notification (
        _("Can't paste selections"));
    }
  arranger_selections_free (
    (ArrangerSelections *) as);
}

static void
on_clipboard_received (
  GtkClipboard *     clipboard,
  const char *       text,
  gpointer           data)
{
  if (!text)
    return;

#define TYPE_PREFIX "\n  type: "

  if ((MAIN_WINDOW_LAST_FOCUSED_IS (
         MW_TIMELINE) ||
       MAIN_WINDOW_LAST_FOCUSED_IS (
         MW_PINNED_TIMELINE)) &&
      string_contains_substr (
        text, TYPE_PREFIX "Timeline", false))
    {
      on_timeline_clipboard_received (
        clipboard, text, data);
    }
  else if ((MAIN_WINDOW_LAST_FOCUSED_IS (
              MW_MIDI_ARRANGER) ||
            MAIN_WINDOW_LAST_FOCUSED_IS (
              MW_MIDI_MODIFIER_ARRANGER)) &&
            string_contains_substr (
              text, TYPE_PREFIX "MIDI", false))
    {
      on_midi_clipboard_received (
        clipboard, text, data);
    }
  else if (MAIN_WINDOW_LAST_FOCUSED_IS (
             MW_CHORD_ARRANGER) &&
            string_contains_substr (
              text, TYPE_PREFIX "Chord", false))
    {
      on_chord_clipboard_received (
        clipboard, text, data);
    }
  else if (MAIN_WINDOW_LAST_FOCUSED_IS (
             MW_AUTOMATION_ARRANGER) &&
            string_contains_substr (
              text, TYPE_PREFIX "Automation", false))
    {
      on_automation_clipboard_received (
        clipboard, text, data);
    }
#undef TYPE_PREFIX
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
  ArrangerSelections * sel =
    main_window_get_last_focused_arranger_selections (
      MAIN_WINDOW);

  if (sel)
    {
      UndoableAction * action =
        arranger_selections_action_new_delete (
          sel);
      undo_manager_perform (
        UNDO_MANAGER, action);
    }
}

void
activate_duplicate (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  ArrangerSelections * sel =
    main_window_get_last_focused_arranger_selections (
      MAIN_WINDOW);

  if (sel)
    {
      double length =
        arranger_selections_get_length_in_ticks (
          sel);
      UndoableAction * ua =
        arranger_selections_action_new_duplicate (
          sel, length, 0, 0, 0, 0, 0,
          F_NOT_ALREADY_MOVED);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
}

void
activate_clear_selection (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  g_message ("ZOOMING IN");
}

void
activate_select_all (
  GSimpleAction *action,
  GVariant *variant,
  gpointer user_data)
{
  if (MAIN_WINDOW->last_focused
    == GTK_WIDGET (MW_MIDI_ARRANGER))
  {
    arranger_widget_select_all (
      Z_ARRANGER_WIDGET (
        MAIN_WINDOW->last_focused),
      1);
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
    MW_CENTER_DOCK &&
    MW_CENTER_DOCK->
      timeline_plus_event_viewer_paned);
  GtkWidget * widget =
    (GtkWidget *)
    MW_CENTER_DOCK->
      timeline_plus_event_viewer_paned;
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

/**
 * Toggle drum mode in the piano roll.
 */
void
activate_toggle_drum_mode (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  PIANO_ROLL->drum_mode = !PIANO_ROLL->drum_mode;

  EVENTS_PUSH (ET_DRUM_MODE_CHANGED, NULL);
}

void
activate_fullscreen (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
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
activate_snap_to_grid (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  SNAP_GRID_TIMELINE->snap_to_grid =
    !SNAP_GRID_TIMELINE->snap_to_grid;
}

void
activate_snap_keep_offset (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  SNAP_GRID_TIMELINE->snap_to_grid_keep_offset =
    !SNAP_GRID_TIMELINE->snap_to_grid_keep_offset;
}

void
activate_snap_events (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  SNAP_GRID_TIMELINE->snap_to_events =
    !SNAP_GRID_TIMELINE->snap_to_events;
}

void
activate_create_audio_track (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_AUDIO, NULL, NULL,
      TRACKLIST->num_tracks, PLAYHEAD, 1);

  undo_manager_perform (UNDO_MANAGER, ua);
}

void
activate_create_ins_track (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_INSTRUMENT, NULL, NULL,
      TRACKLIST->num_tracks, PLAYHEAD, 1);

  undo_manager_perform (UNDO_MANAGER, ua);
}

void
activate_create_midi_track (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_MIDI, NULL, NULL,
      TRACKLIST->num_tracks, PLAYHEAD, 1);

  undo_manager_perform (UNDO_MANAGER, ua);
}

void
activate_create_audio_bus_track (GSimpleAction *action,
                  GVariant      *variant,
                  gpointer       user_data)
{
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_AUDIO_BUS, NULL, NULL,
      TRACKLIST->num_tracks, PLAYHEAD, 1);

  undo_manager_perform (UNDO_MANAGER, ua);
}

void
activate_create_midi_bus_track (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_MIDI_BUS, NULL, NULL,
      TRACKLIST->num_tracks, PLAYHEAD, 1);

  undo_manager_perform (UNDO_MANAGER, ua);
}

void
activate_create_audio_group_track (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_AUDIO_GROUP, NULL, NULL,
      TRACKLIST->num_tracks, PLAYHEAD, 1);

  undo_manager_perform (UNDO_MANAGER, ua);
}

void
activate_create_midi_group_track (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_MIDI_GROUP, NULL, NULL,
      TRACKLIST->num_tracks, PLAYHEAD, 1);

  undo_manager_perform (UNDO_MANAGER, ua);
}

void
activate_duplicate_selected_tracks (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  UndoableAction * ua =
    copy_tracks_action_new (
      TRACKLIST_SELECTIONS,
      TRACKLIST_SELECTIONS->tracks[0]->pos + 1);

  undo_manager_perform (UNDO_MANAGER, ua);
}

void
activate_goto_prev_marker (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  transport_goto_prev_marker (
    TRANSPORT);
}

void
activate_goto_next_marker (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  transport_goto_next_marker (
    TRANSPORT);
}

void
activate_play_pause (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
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

  UndoableAction * ua =
    delete_tracks_action_new (
      TRACKLIST_SELECTIONS);
  undo_manager_perform (
    UNDO_MANAGER, ua);
}

void
activate_hide_selected_tracks (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  g_message ("hiding selected tracks");

  tracklist_selections_toggle_visibility (
    TRACKLIST_SELECTIONS);
}

void
activate_pin_selected_tracks (
  GSimpleAction *action,
  GVariant      *variant,
  gpointer       user_data)
{
  g_message ("pin/unpinning selected tracks");

  tracklist_selections_toggle_pinned (
    TRACKLIST_SELECTIONS);
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
  if (string_is_equal (variant, "timeline", 1))
    {
      UndoableAction * ua =
        arranger_selections_action_new_quantize (
          (ArrangerSelections *) TL_SELECTIONS,
          QUANTIZE_OPTIONS_TIMELINE);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
  else if (string_is_equal (variant, "editor", 1))
    {
      ArrangerSelections * sel =
        clip_editor_get_arranger_selections (
          CLIP_EDITOR);
      g_return_if_fail (sel);
      UndoableAction * ua =
        arranger_selections_action_new_quantize (
          sel, QUANTIZE_OPTIONS_EDITOR);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
  else if (string_is_equal (variant, "global", 1))
    {
      if (MAIN_WINDOW->last_focused ==
          GTK_WIDGET (MW_TIMELINE))
        {
          UndoableAction * ua =
            arranger_selections_action_new_quantize (
              (ArrangerSelections *) TL_SELECTIONS,
              QUANTIZE_OPTIONS_TIMELINE);
          undo_manager_perform (
            UNDO_MANAGER, ua);
        }
    }
  else
    {
      g_return_if_reached ();
    }
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
  if (string_is_equal (variant, "timeline", 1))
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
  else if (string_is_equal (variant, "editor", 1))
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
  else if (string_is_equal (variant, "global", 1))
    {
      if (MAIN_WINDOW->last_focused ==
          GTK_WIDGET (MW_TIMELINE))
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
  else
    {
      g_return_if_reached ();
    }
  g_message ("quantize opts");
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
  if (string_is_equal (variant, "timeline", 1))
    {
      sel = (ArrangerSelections *) TL_SELECTIONS;
    }
  else if (string_is_equal (variant, "editor", 1))
    {
      sel = (ArrangerSelections *) MA_SELECTIONS;
    }
  else if (string_is_equal (variant, "global", 1))
    {
      if (MAIN_WINDOW->last_focused ==
          GTK_WIDGET (MW_TIMELINE))
        {
          sel =
            (ArrangerSelections *) TL_SELECTIONS;
        }
      else if (MAIN_WINDOW->last_focused ==
                 GTK_WIDGET (MW_MIDI_ARRANGER))
        {
          sel =
            (ArrangerSelections *) MA_SELECTIONS;
        }
    }
  else
    {
      g_return_if_reached ();
    }

  if (sel)
    {
      UndoableAction * ua =
        arranger_selections_action_new_edit (
          sel, NULL,
          ARRANGER_SELECTIONS_ACTION_EDIT_MUTE,
          false);
      undo_manager_perform (UNDO_MANAGER, ua);
    }

  g_message ("mute/unmute selections");
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
