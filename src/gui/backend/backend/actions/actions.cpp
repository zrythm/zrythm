// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "common/dsp/audio_function.h"
#include "common/dsp/audio_region.h"
#include "common/dsp/automation_function.h"
#include "common/dsp/graph.h"
#include "common/dsp/marker.h"
#include "common/dsp/marker_track.h"
#include "common/dsp/midi_event.h"
#include "common/dsp/midi_function.h"
#include "common/dsp/router.h"
#include "common/dsp/tempo_track.h"
#include "common/dsp/track.h"
#include "common/dsp/tracklist.h"
#include "common/dsp/transport.h"
#include "common/plugins/collections.h"
#include "common/plugins/plugin_manager.h"
#include "common/utils/debug.h"
#include "common/utils/error.h"
#include "common/utils/flags.h"
#include "common/utils/gtk.h"
#include "common/utils/localization.h"
#include "common/utils/progress_info.h"
#include "common/utils/string.h"
#include "gui/backend/backend/actions/actions.h"
#include "gui/backend/backend/actions/arranger_selections.h"
#include "gui/backend/backend/actions/midi_mapping_action.h"
#include "gui/backend/backend/actions/mixer_selections_action.h"
#include "gui/backend/backend/actions/port_action.h"
#include "gui/backend/backend/actions/port_connection_action.h"
#include "gui/backend/backend/actions/range_action.h"
#include "gui/backend/backend/actions/tracklist_selections.h"
#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/chord_editor.h"
#include "gui/backend/backend/clipboard.h"
#include "gui/backend/backend/event.h"
#include "gui/backend/backend/event_manager.h"
#include "gui/backend/backend/file_manager.h"
#include "gui/backend/backend/midi_selections.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/project/project_init_flow_manager.h"
#include "gui/backend/backend/settings/chord_preset_pack_manager.h"
#include "gui/backend/backend/settings/g_settings_manager.h"
#include "gui/backend/backend/timeline_selections.h"
#include "gui/backend/backend/tracklist_selections.h"
#include "gui/backend/backend/wrapped_object_with_change_signal.h"
#include "gui/backend/gtk_widgets/arranger.h"
#include "gui/backend/gtk_widgets/automation_arranger.h"
#include "gui/backend/gtk_widgets/automation_editor_space.h"
#include "gui/backend/gtk_widgets/bot_bar.h"
#include "gui/backend/gtk_widgets/bot_dock_edge.h"
#include "gui/backend/gtk_widgets/cc_bindings.h"
#include "gui/backend/gtk_widgets/cc_bindings_tree.h"
#include "gui/backend/gtk_widgets/center_dock.h"
#include "gui/backend/gtk_widgets/channel_slot.h"
#include "gui/backend/gtk_widgets/chord_arranger.h"
#include "gui/backend/gtk_widgets/chord_editor_space.h"
#include "gui/backend/gtk_widgets/clip_editor.h"
#include "gui/backend/gtk_widgets/clip_editor_inner.h"
#include "gui/backend/gtk_widgets/dialogs/about_dialog.h"
#include "gui/backend/gtk_widgets/dialogs/add_tracks_to_group_dialog.h"
#include "gui/backend/gtk_widgets/dialogs/arranger_object_info.h"
#include "gui/backend/gtk_widgets/dialogs/bind_cc_dialog.h"
#include "gui/backend/gtk_widgets/dialogs/bounce_dialog.h"
#include "gui/backend/gtk_widgets/dialogs/export_dialog.h"
#include "gui/backend/gtk_widgets/dialogs/export_midi_file_dialog.h"
#include "gui/backend/gtk_widgets/dialogs/export_progress_dialog.h"
#include "gui/backend/gtk_widgets/dialogs/midi_function_dialog.h"
#include "gui/backend/gtk_widgets/dialogs/object_color_chooser_dialog.h"
#include "gui/backend/gtk_widgets/dialogs/port_info.h"
#include "gui/backend/gtk_widgets/dialogs/quantize_dialog.h"
#include "gui/backend/gtk_widgets/dialogs/save_chord_preset_dialog.h"
#include "gui/backend/gtk_widgets/dialogs/string_entry_dialog.h"
#include "gui/backend/gtk_widgets/editor_ruler.h"
#include "gui/backend/gtk_widgets/event_viewer.h"
#include "gui/backend/gtk_widgets/greeter.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/left_dock_edge.h"
#include "gui/backend/gtk_widgets/main_notebook.h"
#include "gui/backend/gtk_widgets/main_window.h"
#include "gui/backend/gtk_widgets/midi_arranger.h"
#include "gui/backend/gtk_widgets/midi_editor_space.h"
#include "gui/backend/gtk_widgets/midi_modifier_arranger.h"
#include "gui/backend/gtk_widgets/mixer.h"
#include "gui/backend/gtk_widgets/panel_file_browser.h"
#include "gui/backend/gtk_widgets/plugin_browser.h"
#include "gui/backend/gtk_widgets/port_connections.h"
#include "gui/backend/gtk_widgets/port_connections_tree.h"
#include "gui/backend/gtk_widgets/preferences.h"
#include "gui/backend/gtk_widgets/right_dock_edge.h"
#include "gui/backend/gtk_widgets/ruler.h"
#include "gui/backend/gtk_widgets/timeline_arranger.h"
#include "gui/backend/gtk_widgets/timeline_bg.h"
#include "gui/backend/gtk_widgets/timeline_panel.h"
#include "gui/backend/gtk_widgets/timeline_ruler.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#define DEFINE_SIMPLE(x) \
  void x (GSimpleAction * action, GVariant * variant, gpointer user_data)

using namespace zrythm;

void
actions_set_app_action_enabled (const char * action_name, const bool enabled)
{
  auto action = zrythm_app->lookup_action (action_name);
  auto simple_action = std::dynamic_pointer_cast<Gio::SimpleAction> (action);
  if (simple_action)
    {
      simple_action->set_enabled (enabled);
    }
  else
    {
      z_warning ("action is not a simple action");
    }
}

void
activate_news ()
{
  /* this is not used anymore */
#if 0
  gtk_show_uri (
    GTK_WINDOW (MAIN_WINDOW), "https://mastodon.social/@zrythm",
    GDK_CURRENT_TIME);
#endif
}

void
activate_manual ()
{
  LocalizationLanguage lang =
    (LocalizationLanguage) g_settings_get_enum (S_P_UI_GENERAL, "language");
  const char * lang_code = localization_get_string_code (lang);
#ifdef MANUAL_PATH
  auto path = fmt::format ("file://{}/{}/index.html", MANUAL_PATH, lang_code);
#else
  auto path = fmt::format ("https://manual.zrythm.org/{}/index.html", lang_code);
#endif
  GtkUriLauncher * launcher = gtk_uri_launcher_new (path.c_str ());
  gtk_uri_launcher_launch (
    launcher, GTK_WINDOW (MAIN_WINDOW), nullptr, nullptr, nullptr);
  g_object_unref (launcher);
}

static void
cycle_focus (const bool backwards)
{
  GtkWidget * widgets[] = {
    z_gtk_get_first_focusable_child (
      GTK_WIDGET (MAIN_WINDOW->start_dock_switcher)),
    z_gtk_get_first_focusable_child (GTK_WIDGET (MW_LEFT_DOCK_EDGE)),
    z_gtk_get_first_focusable_child (GTK_WIDGET (MW_CENTER_DOCK->main_notebook)),
    z_gtk_get_first_focusable_child (GTK_WIDGET (MW_RIGHT_DOCK_EDGE)),
    z_gtk_get_first_focusable_child (GTK_WIDGET (MW_BOT_DOCK_EDGE)),
    z_gtk_get_first_focusable_child (GTK_WIDGET (MW_BOT_BAR)),
  };

  const int num_elements = 6;
  if (backwards)
    {
      for (int i = num_elements - 2; i >= 0; i--)
        {
          if (
            gtk_widget_has_focus (widgets[i + 1])
            || z_gtk_descendant_has_focus (widgets[i + 1]))
            {
              gtk_widget_grab_focus (widgets[i]);
              return;
            }
        }
      gtk_widget_grab_focus (widgets[num_elements - 1]);
    }
  else
    {
      for (int i = 1; i < num_elements; i++)
        {
          if (
            gtk_widget_has_focus (widgets[i - 1])
            || z_gtk_descendant_has_focus (widgets[i - 1]))
            {
              gtk_widget_grab_focus (widgets[i]);
              return;
            }
        }
      gtk_widget_grab_focus (widgets[0]);
    }
}

DEFINE_SIMPLE (activate_cycle_focus)
{
  cycle_focus (false);
}

DEFINE_SIMPLE (activate_cycle_focus_backwards)
{
  cycle_focus (true);
}

DEFINE_SIMPLE (activate_focus_first_widget)
{
  gtk_widget_grab_focus (z_gtk_get_first_focusable_child (
    GTK_WIDGET (MAIN_WINDOW->start_dock_switcher)));
}

void
activate_chat ()
{
  GtkUriLauncher * launcher =
    gtk_uri_launcher_new ("https://matrix.to/#/#zrythmdaw:matrix.org");
  gtk_uri_launcher_launch (
    launcher, GTK_WINDOW (MAIN_WINDOW), nullptr, nullptr, nullptr);
  g_object_unref (launcher);
}

void
activate_donate ()
{
  GtkUriLauncher * launcher =
    gtk_uri_launcher_new ("https://liberapay.com/Zrythm");
  gtk_uri_launcher_launch (
    launcher, GTK_WINDOW (MAIN_WINDOW), nullptr, nullptr, nullptr);
  g_object_unref (launcher);
}

void
activate_bugreport ()
{
  GtkUriLauncher * launcher = gtk_uri_launcher_new (NEW_ISSUE_URL);
  gtk_uri_launcher_launch (
    launcher, GTK_WINDOW (MAIN_WINDOW), nullptr, nullptr, nullptr);
  g_object_unref (launcher);
}

void
activate_about ()
{
  GtkWindow * window =
    GTK_WINDOW (about_dialog_widget_new (GTK_WINDOW (MAIN_WINDOW)));
  gtk_window_present (window);
}

/**
 * @note This is never called but keep it around
 * for shortcut window.
 */
void
activate_quit ()
{
  zrythm_app->quit ();
}

/**
 * Show preferences window.
 */
void
activate_preferences ()
{
  if (MAIN_WINDOW && MAIN_WINDOW->preferences_opened)
    {
      return;
    }

  AdwDialog * preferences_window = ADW_DIALOG (preferences_widget_new ());
  z_return_if_fail (preferences_window);
  GListModel * toplevels = gtk_window_get_toplevels ();
  GtkWidget * transient_for = GTK_WIDGET (g_list_model_get_item (toplevels, 0));
  adw_dialog_present (preferences_window, transient_for);
  if (MAIN_WINDOW)
    {
      MAIN_WINDOW->preferences_opened = true;
    }
}

#if 0
/**
 * Show log window.
 */
void
activate_log (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  if (!LOG || !LOG->log_filepath)
    {
      z_info ("No log file found");
      return;
    }

  GFile *           file = g_file_new_for_path (LOG->log_filepath);
  GtkFileLauncher * launcher = gtk_file_launcher_new (file);
  gtk_file_launcher_launch (launcher, nullptr, nullptr, nullptr, nullptr);
  g_object_unref (file);
  g_object_unref (launcher);

  if (ZRYTHM_HAVE_UI && MAIN_WINDOW)
    {
      MAIN_WINDOW->log_has_pending_warnings = false;
      EVENTS_PUSH (EventType::ET_LOG_WARNING_STATE_CHANGED, nullptr);
    }
}
#endif

DEFINE_SIMPLE (activate_audition_mode)
{
  P_TOOL = Tool::Audition;
  EVENTS_PUSH (EventType::ET_TOOL_CHANGED, nullptr);
}

DEFINE_SIMPLE (activate_select_mode)
{
  P_TOOL = Tool::Select;
  EVENTS_PUSH (EventType::ET_TOOL_CHANGED, nullptr);
}

/**
 * Activate edit mode.
 */
void
activate_edit_mode (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  P_TOOL = Tool::Edit;
  EVENTS_PUSH (EventType::ET_TOOL_CHANGED, nullptr);
}

/**
 * Activate cut mode.
 */
void
activate_cut_mode (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  P_TOOL = Tool::Cut;
  EVENTS_PUSH (EventType::ET_TOOL_CHANGED, nullptr);
}

/**
 * Activate eraser mode.
 */
void
activate_eraser_mode (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  P_TOOL = Tool::Eraser;
  EVENTS_PUSH (EventType::ET_TOOL_CHANGED, nullptr);
}

/**
 * Activate ramp mode.
 */
void
activate_ramp_mode (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  P_TOOL = Tool::Ramp;
  EVENTS_PUSH (EventType::ET_TOOL_CHANGED, nullptr);
}

DEFINE_SIMPLE (activate_zoom_in)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);

  /* if ends with v, this is a vertical zoom */
  if (g_str_has_suffix (str, "v"))
    {
      if (g_str_has_prefix (str, "editor"))
        {
          ArrangerWidget * arranger =
            clip_editor_inner_widget_get_visible_arranger (MW_CLIP_EDITOR_INNER);

          switch (arranger->type)
            {
            case ArrangerWidgetType::Midi:
              midi_arranger_handle_vertical_zoom_action (arranger, true);
              break;
            default:
              z_warning ("unimplemented");
              break;
            }
        }
    }
  else
    {
      RulerWidget * ruler = NULL;
      if (string_is_equal (str, "timeline"))
        {
          ruler = MW_RULER;
        }
      else if (string_is_equal (str, "editor"))
        {
          ruler = EDITOR_RULER;
        }
      else
        {
          switch (PROJECT->last_selection_)
            {
            case Project::SelectionType::Editor:
              ruler = EDITOR_RULER;
              break;
            default:
              ruler = MW_RULER;
              break;
            }
        }

      ruler_widget_set_zoom_level (
        ruler,
        ruler_widget_get_zoom_level (ruler) * RULER_ZOOM_LEVEL_MULTIPLIER);

      EVENTS_PUSH (EventType::ET_RULER_VIEWPORT_CHANGED, ruler);
    }
}

DEFINE_SIMPLE (activate_zoom_out)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);

  /* if ends with v, this is a vertical zoom */
  if (g_str_has_suffix (str, "v"))
    {
      if (g_str_has_prefix (str, "editor"))
        {
          ArrangerWidget * arranger =
            clip_editor_inner_widget_get_visible_arranger (MW_CLIP_EDITOR_INNER);

          switch (arranger->type)
            {
            case ArrangerWidgetType::Midi:
              midi_arranger_handle_vertical_zoom_action (arranger, false);
              break;
            default:
              z_warning ("unimplemented");
              break;
            }
        }
    }
  else
    {
      RulerWidget * ruler = NULL;
      if (string_is_equal (str, "timeline"))
        {
          ruler = MW_RULER;
        }
      else if (string_is_equal (str, "editor"))
        {
          ruler = EDITOR_RULER;
        }
      else
        {
          switch (PROJECT->last_selection_)
            {
            case Project::SelectionType::Editor:
              ruler = EDITOR_RULER;
              break;
            default:
              ruler = MW_RULER;
              break;
            }
        }

      double zoom_level =
        ruler_widget_get_zoom_level (ruler) / RULER_ZOOM_LEVEL_MULTIPLIER;
      ruler_widget_set_zoom_level (ruler, zoom_level);

      EVENTS_PUSH (EventType::ET_RULER_VIEWPORT_CHANGED, ruler);
    }
}

DEFINE_SIMPLE (activate_best_fit)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);

  RulerWidget * ruler = NULL;
  if (string_is_equal (str, "timeline"))
    {
      ruler = MW_RULER;
    }
  else if (string_is_equal (str, "editor"))
    {
      ruler = EDITOR_RULER;
    }
  else
    {
      switch (PROJECT->last_selection_)
        {
        case Project::SelectionType::Editor:
          ruler = EDITOR_RULER;
          break;
        default:
          ruler = MW_RULER;
          break;
        }
    }

  Position start, end;
  if (ruler == MW_RULER)
    {
      int total_bars = 0;
      total_bars = TRACKLIST->get_total_bars (total_bars);
      end.set_to_bar (total_bars);
    }
  else if (ruler == EDITOR_RULER)
    {
      Region * r = CLIP_EDITOR->get_region ();
      if (!r)
        return;

      ArrangerWidget *              arranger = r->get_arranger_for_children ();
      std::vector<ArrangerObject *> objs_arr;
      arranger_widget_get_all_objects (arranger, objs_arr);
      start = r->pos_;
      end = r->end_pos_;
      for (auto obj : objs_arr)
        {
          Position global_end;
          Position global_start = obj->pos_;
          global_start.add_ticks (r->pos_.ticks_);
          if (obj->has_length ())
            {
              auto lo = dynamic_cast<LengthableObject *> (obj);
              global_end = lo->end_pos_;
              global_end.add_ticks (r->pos_.ticks_);
            }
          else
            {
              global_end = global_start;
            }

          if (global_start < start)
            {
              start = global_start;
            }
          if (global_end > end)
            {
              end = global_end;
            }
        }
    }

  double total_ticks = end.ticks_ - start.ticks_;
  double allocated_px = (double) gtk_widget_get_width (GTK_WIDGET (ruler));
  double buffer_px = allocated_px / 16.0;
  double needed_px_per_tick = (allocated_px - buffer_px) / total_ticks;
  double new_zoom_level =
    ruler_widget_get_zoom_level (ruler)
    * (needed_px_per_tick / ruler->px_per_tick);
  ruler_widget_set_zoom_level (ruler, new_zoom_level);
  int              start_px = ruler_widget_pos_to_px (ruler, &start, false);
  EditorSettings * settings = ruler_widget_get_editor_settings (ruler);
  settings->set_scroll_start_x (start_px, true);

  EVENTS_PUSH (EventType::ET_RULER_VIEWPORT_CHANGED, ruler);
}

DEFINE_SIMPLE (activate_original_size)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);

  RulerWidget * ruler = NULL;
  if (string_is_equal (str, "timeline"))
    {
      ruler = MW_RULER;
    }
  else if (string_is_equal (str, "editor"))
    {
      ruler = EDITOR_RULER;
    }
  else
    {
      switch (PROJECT->last_selection_)
        {
        case Project::SelectionType::Editor:
          ruler = EDITOR_RULER;
          break;
        default:
          ruler = MW_RULER;
          break;
        }
    }

  ruler_widget_set_zoom_level (ruler, 1.0);
  EVENTS_PUSH (EventType::ET_RULER_VIEWPORT_CHANGED, ruler);
}

DEFINE_SIMPLE (activate_loop_selection)
{
  if (PROJECT->last_selection_ == Project::SelectionType::Timeline)
    {
      if (!TL_SELECTIONS->has_any ())
        return;

      auto [first_obj, start] = TL_SELECTIONS->get_first_object_and_pos (true);
      auto [last_obj, end] = TL_SELECTIONS->get_last_object_and_pos (true, true);

      TRANSPORT->loop_start_pos_ = start;
      TRANSPORT->loop_end_pos_ = end;

      /* FIXME is this needed? */
      TRANSPORT->update_positions (true);

      EVENTS_PUSH (EventType::ET_TIMELINE_LOOP_MARKER_POS_CHANGED, nullptr);
    }
}

static void
proceed_to_new_response_cb (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           data)
{
  if (string_is_equal (response, "discard"))
    {
      GreeterWidget * greeter = greeter_widget_new (
        zrythm_app.get (), GTK_WINDOW (MAIN_WINDOW), false, true);
      gtk_window_present (GTK_WINDOW (greeter));
    }
  else if (string_is_equal (response, "save"))
    {
      z_info ("saving project...");
      try
        {
          PROJECT->save (PROJECT->dir_, false, false, false);
        }
      catch (const ZrythmException &e)
        {
          e.handle (_ ("Failed to save project"));
          return;
        }
      GreeterWidget * greeter = greeter_widget_new (
        zrythm_app.get (), GTK_WINDOW (MAIN_WINDOW), false, true);
      gtk_window_present (GTK_WINDOW (greeter));
    }
  else
    {
      /* do nothing */
    }
}

void
activate_new (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  AdwMessageDialog * dialog = ADW_MESSAGE_DIALOG (adw_message_dialog_new (
    GTK_WINDOW (MAIN_WINDOW), _ ("Discard Changes?"),
    _ ("Any unsaved changes to the current "
       "project will be lost. Continue?")));
  adw_message_dialog_add_responses (
    dialog, "cancel", _ ("_Cancel"), "discard", _ ("_Discard"), "save",
    _ ("_Save"), nullptr);
  adw_message_dialog_set_close_response (dialog, "cancel");
  adw_message_dialog_set_response_appearance (
    dialog, "discard", ADW_RESPONSE_DESTRUCTIVE);
  adw_message_dialog_set_response_appearance (
    dialog, "save", ADW_RESPONSE_SUGGESTED);
  adw_message_dialog_set_default_response (dialog, "save");
  g_signal_connect (
    dialog, "response", G_CALLBACK (proceed_to_new_response_cb), nullptr);
  gtk_window_present (GTK_WINDOW (dialog));
}

static void
choose_and_open_project (void)
{
  GreeterWidget * greeter = greeter_widget_new (
    zrythm_app.get (), GTK_WINDOW (MAIN_WINDOW), false, false);
  gtk_window_present (GTK_WINDOW (greeter));
}

static void
proceed_to_open_response_cb (
  AdwMessageDialog * dialog,
  gchar *            response,
  gpointer           data)
{
  if (string_is_equal (response, "discard"))
    {
      choose_and_open_project ();
    }
  else if (string_is_equal (response, "save"))
    {
      z_info ("saving project...");
      try
        {
          PROJECT->save (
            PROJECT->dir_, F_NOT_BACKUP, ZRYTHM_F_NO_NOTIFY, F_NO_ASYNC);
        }
      catch (const ZrythmException &e)
        {
          e.handle (_ ("Failed to save project"));
          return;
        }
      choose_and_open_project ();
    }
  else
    {
      /* do nothing */
    }
}

DEFINE_SIMPLE (activate_open)
{
  if (PROJECT->has_unsaved_changes ())
    {
      AdwMessageDialog * dialog = ADW_MESSAGE_DIALOG (adw_message_dialog_new (
        GTK_WINDOW (MAIN_WINDOW), _ ("Save Changes?"),
        _ ("The project contains unsaved changes. "
           "If you proceed, any unsaved changes will be "
           "lost.")));
      adw_message_dialog_add_responses (
        dialog, "cancel", _ ("_Cancel"), "discard", _ ("_Discard"), "save",
        _ ("_Save"), nullptr);
      adw_message_dialog_set_close_response (dialog, "cancel");
      adw_message_dialog_set_response_appearance (
        dialog, "discard", ADW_RESPONSE_DESTRUCTIVE);
      adw_message_dialog_set_response_appearance (
        dialog, "save", ADW_RESPONSE_SUGGESTED);
      adw_message_dialog_set_default_response (dialog, "save");
      g_signal_connect (
        dialog, "response", G_CALLBACK (proceed_to_open_response_cb), nullptr);
      gtk_window_present (GTK_WINDOW (dialog));
    }
  else
    {
      choose_and_open_project ();
    }
}

void
activate_save (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  if (PROJECT->dir_.empty () || PROJECT->title_.empty ())
    {
      activate_save_as (action, variant, user_data);
      return;
    }
  z_debug ("project dir: {}", PROJECT->dir_);

  try
    {
      PROJECT->save (PROJECT->dir_, F_NOT_BACKUP, ZRYTHM_F_NOTIFY, F_NO_ASYNC);
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to save project"));
    }
}

static void
save_project_ready_cb (GObject * source_object, GAsyncResult * res, gpointer data)
{
  GFile * selected_file =
    gtk_file_dialog_save_finish (GTK_FILE_DIALOG (source_object), res, nullptr);
  if (selected_file)
    {
      char * filepath = g_file_get_path (selected_file);
      g_object_unref (selected_file);
      z_debug ("saving project at: {}", filepath);
      try
        {
          PROJECT->save (filepath, F_NOT_BACKUP, ZRYTHM_F_NO_NOTIFY, F_NO_ASYNC);
        }
      catch (const ZrythmException &e)
        {
          e.handle (_ ("Failed to save project"));
        }
      g_free (filepath);
    }
}

void
activate_save_as (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  GtkFileDialog * dialog = gtk_file_dialog_new ();
  fs::path        project_file_path =
    PROJECT->get_path (ProjectPath::ProjectFile, false);
  auto    str = project_file_path.parent_path ();
  GFile * file = g_file_new_for_path (str.string ().c_str ());
  gtk_file_dialog_set_initial_file (dialog, file);
  g_object_unref (file);
  gtk_file_dialog_set_initial_name (dialog, PROJECT->title_.c_str ());
  gtk_file_dialog_set_accept_label (dialog, _ ("Save Project"));
  gtk_file_dialog_save (
    dialog, GTK_WINDOW (MAIN_WINDOW), nullptr, save_project_ready_cb, nullptr);
}

void
activate_minimize ()
{
  gtk_window_minimize (GTK_WINDOW (MAIN_WINDOW));
}

void
activate_export_as (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  ExportDialogWidget * exp = export_dialog_widget_new ();
  gtk_window_set_transient_for (GTK_WINDOW (exp), GTK_WINDOW (MAIN_WINDOW));
  gtk_window_present (GTK_WINDOW (exp));
}

void
activate_export_graph (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
#ifdef HAVE_CGRAPH
  char * exports_dir = project_get_path (PROJECT, ProjectPath::EXPORTS, false);

  GtkWidget *    dialog, *label, *content_area;
  GtkDialogFlags flags;

  // Create the widgets
  flags = GTK_DIALOG_DESTROY_WITH_PARENT;
  dialog = gtk_dialog_new_with_buttons (
    _ ("Export routing graph"), GTK_WINDOW (MAIN_WINDOW), flags,
    _ ("Image (PNG)"), 1, _ ("Image (SVG)"), 2, _ ("Dot graph"), 3, nullptr);
  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  char lbl[600];
  sprintf (
    lbl,
    _ ("The graph will be exported to "
       "<a href=\"%s\">%s</a>\n"
       "Please select a format to export as"),
    exports_dir, exports_dir);
  label = gtk_label_new (nullptr);
  gtk_label_set_markup (GTK_LABEL (label), lbl);
  gtk_widget_set_visible (label, true);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  z_gtk_widget_set_margin (label, 3);

  g_signal_connect (
    G_OBJECT (label), "activate-link",
    G_CALLBACK (z_gtk_activate_dir_link_func), nullptr);

  gtk_box_append (GTK_BOX (content_area), label);

  int             result = z_gtk_dialog_run (GTK_DIALOG (dialog), false);
  const char *    filename = NULL;
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
      gtk_window_destroy (GTK_WINDOW (dialog));
      return;
    }
  gtk_window_destroy (GTK_WINDOW (dialog));

  char * path = g_build_filename (exports_dir, filename, nullptr);
  graph_export_as_simple (export_type, path);
  g_free (exports_dir);
  g_free (path);

  ui_show_notification (_ ("Graph exported"));
#endif
}

void
activate_properties (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  z_info ("ZOOMING IN");
}

DEFINE_SIMPLE (activate_undo)
{
  if (arranger_widget_any_doing_action ())
    {
      z_info ("in the middle of an action, skipping undo");
      return;
    }

  if (UNDO_MANAGER->undo_stack_->is_empty ())
    return;

  try
    {
      UNDO_MANAGER->undo ();
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to undo"));
    }
}

static void
handle_undo_or_redo_n (int idx, bool redo)
{
  for (int i = 0; i <= idx; i++)
    {
      try
        {
          if (redo)
            {
              UNDO_MANAGER->redo ();
            }
          else
            {
              UNDO_MANAGER->undo ();
            }
        }
      catch (const ZrythmException &e)
        {
          e.handle (_ ("Failed to undo/redo"));
        }

    } /* endforeach */
}

DEFINE_SIMPLE (activate_undo_n)
{
  if (arranger_widget_any_doing_action ())
    {
      z_info ("in the middle of an action, skipping undo");
      return;
    }

  int idx = (int) g_variant_get_int32 (variant);
  handle_undo_or_redo_n (idx, false);
}

DEFINE_SIMPLE (activate_redo)
{
  if (arranger_widget_any_doing_action ())
    {
      z_info ("in the middle of an action, skipping redo");
      return;
    }

  if (UNDO_MANAGER->redo_stack_->is_empty ())
    return;

  try
    {
      UNDO_MANAGER->redo ();
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to redo"));
    }

  EVENTS_PUSH (EventType::ET_UNDO_REDO_ACTION_DONE, nullptr);
}

DEFINE_SIMPLE (activate_redo_n)
{
  if (arranger_widget_any_doing_action ())
    {
      z_info ("in the middle of an action, skipping redo");
      return;
    }

  int idx = (int) g_variant_get_int32 (variant);
  handle_undo_or_redo_n (idx, true);
}

void
activate_cut (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  /* activate copy and then delete the selections */
  activate_copy (action, variant, user_data);

  ArrangerSelections * sel =
    PROJECT->get_arranger_selections_for_last_selection ();

  switch (PROJECT->last_selection_)
    {
    case Project::SelectionType::Timeline:
    case Project::SelectionType::Editor:
      if (sel && sel->has_any ())
        {
          try
            {
              UNDO_MANAGER->perform (
                std::make_unique<DeleteArrangerSelectionsAction> (*sel));
            }
          catch (const ZrythmException &e)
            {
              e.handle (_ ("Failed to delete selections"));
            }
        }
      break;
    case Project::SelectionType::Insert:
    case Project::SelectionType::MidiFX:
      if (MIXER_SELECTIONS->has_any ())
        {
          try
            {
              UNDO_MANAGER->perform (
                std::make_unique<MixerSelectionsDeleteAction> (
                  *MIXER_SELECTIONS->gen_full_from_this (),
                  *PORT_CONNECTIONS_MGR));
            }
          catch (const ZrythmException &e)
            {
              e.handle (_ ("Failed to delete selections"));
            }
        }
      break;
    default:
      z_debug ("doing nothing");
      break;
    }
}

void
activate_copy (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  ArrangerSelections * sel =
    PROJECT->get_arranger_selections_for_last_selection ();

  try
    {
      auto serialize_and_show_notification =
        [&] (const Clipboard &clipboard, const char * msg) {
          auto str = clipboard.serialize_to_json_string ();
          gdk_clipboard_set_text (DEFAULT_CLIPBOARD, str.c_str ());
          ui_show_notification (msg);
        };

      switch (PROJECT->last_selection_)
        {
        case Project::SelectionType::Timeline:
        case Project::SelectionType::Editor:
          if (sel)
            {
              Clipboard clipboard (*sel);
              if (sel->type_ == ArrangerSelections::Type::Timeline)
                {
                  auto timeline_sel = dynamic_cast<TimelineSelections *> (
                    clipboard.arranger_sel_.get ());
                  timeline_sel->set_vis_track_indices ();
                }
              serialize_and_show_notification (
                clipboard, _ ("Arranger selections copied to clipboard"));
            }
          else
            {
              z_warning ("no selections to copy");
            }
          break;
        case Project::SelectionType::Insert:
        case Project::SelectionType::MidiFX:
          if (MIXER_SELECTIONS->has_any ())
            {
              Clipboard clipboard (*MIXER_SELECTIONS);
              serialize_and_show_notification (
                clipboard, _ ("Plugins copied to clipboard"));
            }
          break;
        case Project::SelectionType::Tracklist:
          {
            /* TODO fix crashes eg when copy pasting master */
            return;

            Clipboard clipboard (*TRACKLIST_SELECTIONS);
            serialize_and_show_notification (
              clipboard, _ ("Tracks copied to clipboard"));
          }
          break;
        default:
          z_warning ("not implemented yet");
          break;
        }
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to copy selections"));
    }
}

static void
paste_cb (GObject * source_object, GAsyncResult * res, gpointer data)
{
  GError * err = NULL;
  char *   txt = gdk_clipboard_read_text_finish (DEFAULT_CLIPBOARD, res, &err);
  if (!txt)
    {
      ui_show_notification_idle_printf (
        _ ("Failed to paste data: %s"), err ? err->message : "no text found");
      if (err)
        {
          g_error_free (err);
        }
      return;
    }

  try
    {
      Clipboard clipboard{};
      clipboard.deserialize_from_json_string (txt);

      ArrangerSelections *  sel = NULL;
      FullMixerSelections * mixer_sel = NULL;
      TracklistSelections * tracklist_sel = NULL;
      switch (clipboard.type_)
        {
        case Clipboard::Type::TimelineSelections:
        case Clipboard::Type::MidiSelections:
        case Clipboard::Type::AutomationSelections:
        case Clipboard::Type::ChordSelections:
        case Clipboard::Type::AudioSelections:
          sel = clipboard.get_selections ();
          break;
        case Clipboard::Type::MixerSelections:
          mixer_sel = clipboard.mixer_sel_.get ();
          break;
        case Clipboard::Type::TracklistSelections:
          tracklist_sel = clipboard.tracklist_sel_.get ();
          break;
        default:
          z_warning ("Unexpected clipboard type");
        }

      bool incompatible = false;
      if (sel)
        {
          sel->post_deserialize ();
          if (sel->can_be_pasted ())
            {
              sel->paste_to_pos (PLAYHEAD, true);
            }
          else
            {
              z_info ("Can't paste arranger selections");
              incompatible = true;
            }
        }
      else if (mixer_sel)
        {
          auto &slot = MW_MIXER->paste_slot;
          mixer_sel->init_loaded ();
          if (mixer_sel->can_be_pasted (
                *slot->track->channel_, slot->type, slot->slot_index))
            {
              mixer_sel->paste_to_slot (
                *slot->track->channel_, slot->type, slot->slot_index);
            }
          else
            {
              z_info ("Can't paste mixer selections");
              incompatible = true;
            }
        }
      else if (tracklist_sel)
        {
          tracklist_sel->paste_to_pos (TRACKLIST->tracks_.size ());
        }

      if (incompatible)
        {
          ui_show_notification (_ ("Can't paste incompatible data"));
        }
    }
  catch (const ZrythmException &e)
    {
      ui_show_notification_idle_printf (
        _ ("invalid clipboard data received: %s"), e.what ());
    }
}

void
activate_paste (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  z_debug ("paste");

  gdk_clipboard_read_text_async (DEFAULT_CLIPBOARD, nullptr, paste_cb, nullptr);
}

void
activate_delete (
  GSimpleAction * simple_action,
  GVariant *      variant,
  gpointer        user_data)
{
  z_info ("{}", __func__);

  ArrangerSelections * sel =
    PROJECT->get_arranger_selections_for_last_selection ();

  switch (PROJECT->last_selection_)
    {
    case Project::SelectionType::Tracklist:
      z_info ("activating delete selected tracks");
      g_action_group_activate_action (
        G_ACTION_GROUP (MAIN_WINDOW), "delete-selected-tracks", nullptr);
      break;
    case Project::SelectionType::Insert:
    case Project::SelectionType::MidiFX:
      z_info ("activating delete mixer selections");
      g_action_group_activate_action (
        G_ACTION_GROUP (MAIN_WINDOW), "delete-mixer-selections", nullptr);
      break;
    case Project::SelectionType::Timeline:
    case Project::SelectionType::Editor:
      if (sel && sel->has_any () && !sel->contains_undeletable_object ())
        {
          try
            {
              UNDO_MANAGER->perform (
                std::make_unique<DeleteArrangerSelectionsAction> (*sel));
            }
          catch (const ZrythmException &e)
            {
              e.handle (_ ("Failed to delete selections"));
            }
        }
      break;
    default:
      z_warning ("not implemented yet");
      break;
    }
}

void
activate_duplicate (GSimpleAction * action, GVariant * variant, gpointer user_data)
{
  ArrangerSelections * sel =
    PROJECT->get_arranger_selections_for_last_selection ();

  if (sel && sel->has_any ())
    {
      double length = sel->get_length_in_ticks ();

      try
        {
          UNDO_MANAGER->perform (
            std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateAction> (
              *sel, false, length, 0, 0, 0, 0, 0, nullptr, false));
        }
      catch (const ZrythmException &e)
        {
          e.handle (_ ("Failed to duplicate selections"));
        }
    }
}

void
activate_clear_selection (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  ArrangerSelections * sel =
    PROJECT->get_arranger_selections_for_last_selection ();

  switch (PROJECT->last_selection_)
    {
    case Project::SelectionType::Timeline:
    case Project::SelectionType::Editor:
      if (sel)
        {
          sel->clear (true);
        }
      break;
    case Project::SelectionType::Tracklist:
      TRACKLIST->select_all (false, true);
      break;
    case Project::SelectionType::Insert:
    case Project::SelectionType::MidiFX:
      {
        Track * track = TRACKLIST_SELECTIONS->get_lowest_track ();
        z_return_if_fail (track);
        if (track->has_channel ())
          {
            auto           ch_track = dynamic_cast<ChannelTrack *> (track);
            auto           ch = ch_track->get_channel ();
            zrythm::plugins::PluginSlotType slot_type =
              plugins::PluginSlotType::Insert;
            if (PROJECT->last_selection_ == Project::SelectionType::MidiFX)
              {
                slot_type = plugins::PluginSlotType::MidiFx;
              }
            ch->select_all (slot_type, false);
          }
      }
      break;
    default:
      z_debug ("doing nothing");
    }
}

void
activate_select_all (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  ArrangerSelections * sel =
    PROJECT->get_arranger_selections_for_last_selection ();

  switch (PROJECT->last_selection_)
    {
    case Project::SelectionType::Timeline:
    case Project::SelectionType::Editor:
      if (sel)
        {
          sel->select_all (true);
        }
      break;
    case Project::SelectionType::Tracklist:
      TRACKLIST->select_all (true, true);
      break;
    case Project::SelectionType::Insert:
    case Project::SelectionType::MidiFX:
      {
        Track * track = TRACKLIST_SELECTIONS->get_lowest_track ();
        z_return_if_fail (track);
        if (track->has_channel ())
          {
            auto           ch_track = dynamic_cast<ChannelTrack *> (track);
            auto           ch = ch_track->get_channel ();
            zrythm::plugins::PluginSlotType slot_type =
              plugins::PluginSlotType::Insert;
            if (PROJECT->last_selection_ == Project::SelectionType::MidiFX)
              {
                slot_type = plugins::PluginSlotType::MidiFx;
              }
            ch->select_all (slot_type, F_SELECT);
          }
      }
      break;
    default:
      z_debug ("{}: doing nothing", __func__);
    }
}

void
activate_toggle_left_panel (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  z_debug ("left panel toggle");
  z_return_if_fail (MW_CENTER_DOCK);
  panel_dock_set_reveal_start (
    MW_CENTER_DOCK->dock, !panel_dock_get_reveal_start (MW_CENTER_DOCK->dock));
}

void
activate_toggle_right_panel (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  z_debug ("right panel toggle");
  z_return_if_fail (MW_CENTER_DOCK);
  panel_dock_set_reveal_end (
    MW_CENTER_DOCK->dock, !panel_dock_get_reveal_end (MW_CENTER_DOCK->dock));
}

void
activate_toggle_bot_panel (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  z_debug ("bot panel toggle");
  z_return_if_fail (MW_CENTER_DOCK);
  panel_dock_set_reveal_bottom (
    MW_CENTER_DOCK->dock, !panel_dock_get_reveal_bottom (MW_CENTER_DOCK->dock));
}

/**
 * Toggle timeline visibility.
 */
void
activate_toggle_top_panel (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  z_return_if_fail (MW_CENTER_DOCK);
  panel_dock_set_reveal_top (
    MW_CENTER_DOCK->dock, !panel_dock_get_reveal_top (MW_CENTER_DOCK->dock));
}

void
activate_toggle_status_bar (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  gtk_widget_set_visible (
    GTK_WIDGET (MW_BOT_BAR), !gtk_widget_get_visible (GTK_WIDGET (MW_BOT_BAR)));
}

DEFINE_SIMPLE (activate_toggle_drum_mode)
{
  auto tr = dynamic_cast<PianoRollTrack *> (CLIP_EDITOR->get_track ());
  z_return_if_fail (tr);
  tr->drum_mode_ = !tr->drum_mode_;

  EVENTS_PUSH (EventType::ET_DRUM_MODE_CHANGED, tr);
}

void
activate_fullscreen ()
{
  if (gtk_window_is_fullscreen (GTK_WINDOW (MAIN_WINDOW)))
    {
      z_debug ("unfullscreening");
      gtk_window_unfullscreen (GTK_WINDOW (MAIN_WINDOW));
    }
  else
    {
      z_debug ("fullscreening");
      gtk_window_fullscreen (GTK_WINDOW (MAIN_WINDOW));
    }
}

/**
 * All purpose menuitem callback for binding MIDI
 * CC to a port.
 *
 * An action will be performed if bound.
 */
void
activate_bind_midi_cc (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  z_return_if_fail (_variant);

  gsize        size;
  const char * variant = g_variant_get_string (_variant, &size);
  Port *       port = NULL;
  sscanf (variant, "%p", &port);
  z_return_if_fail (IS_PORT_AND_NONNULL (port));

  BindCcDialogWidget * dialog = bind_cc_dialog_widget_new (port, true);

  z_gtk_dialog_run (GTK_DIALOG (dialog), true);
}

void
activate_delete_midi_cc_bindings (
  GSimpleAction * simple_action,
  GVariant *      _variant,
  gpointer        user_data)
{
  GtkSelectionModel * sel_model =
    gtk_column_view_get_model (MW_CC_BINDINGS->bindings_tree->column_view);
  guint n_items = g_list_model_get_n_items (G_LIST_MODEL (sel_model));
  std::vector<MidiMapping *> mappings;
  for (guint i = 0; i < n_items; i++)
    {
      if (gtk_selection_model_is_selected (sel_model, i))
        {
          WrappedObjectWithChangeSignal * wobj =
            Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
              g_list_model_get_item (G_LIST_MODEL (sel_model), i));
          MidiMapping * mm = std::get<MidiMapping *> (wobj->obj);
          mappings.push_back (mm);
        }
    }

  try
    {
      int count = 0;
      for (auto mm : mappings)
        {
          int idx = MIDI_MAPPINGS->get_mapping_index (*mm);
          UNDO_MANAGER->perform (std::make_unique<MidiMappingAction> (idx));
          auto ua = UNDO_MANAGER->get_last_action ();
          ua->num_actions_ = ++count;
        }
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to unbind"));
    }
}

void
activate_snap_to_grid (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  z_return_if_fail (_variant);

  gsize        size;
  const char * variant = g_variant_get_string (_variant, &size);
  if (string_is_equal (variant, "timeline"))
    {
      SNAP_GRID_TIMELINE->snap_to_grid_ = !SNAP_GRID_TIMELINE->snap_to_grid_;
      EVENTS_PUSH (
        EventType::ET_SNAP_GRID_OPTIONS_CHANGED, SNAP_GRID_TIMELINE.get ());
    }
  else if (string_is_equal (variant, "editor"))
    {
      SNAP_GRID_EDITOR->snap_to_grid_ = !SNAP_GRID_EDITOR->snap_to_grid_;
      EVENTS_PUSH (
        EventType::ET_SNAP_GRID_OPTIONS_CHANGED, SNAP_GRID_EDITOR.get ());
    }
  else if (string_is_equal (variant, "global"))
    {
      if (PROJECT->last_selection_ == Project::SelectionType::Timeline)
        {
          try
            {
              UNDO_MANAGER->perform (
                std::make_unique<ArrangerSelectionsAction::QuantizeAction> (
                  *TL_SELECTIONS, *QUANTIZE_OPTIONS_TIMELINE));
            }
          catch (const ZrythmException &e)
            {
              e.handle (_ ("Failed to quantize"));
            }
        }
    }
  else
    {
      z_return_if_reached ();
    }
}

void
activate_snap_keep_offset (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  z_return_if_fail (_variant);
  gsize        size;
  const char * variant = g_variant_get_string (_variant, &size);

  if (string_is_equal (variant, "timeline"))
    {
      SNAP_GRID_TIMELINE->snap_to_grid_keep_offset_ =
        !SNAP_GRID_TIMELINE->snap_to_grid_keep_offset_;
      EVENTS_PUSH (
        EventType::ET_SNAP_GRID_OPTIONS_CHANGED, SNAP_GRID_TIMELINE.get ());
    }
  else if (string_is_equal (variant, "editor"))
    {
      SNAP_GRID_EDITOR->snap_to_grid_keep_offset_ =
        !SNAP_GRID_EDITOR->snap_to_grid_keep_offset_;
      EVENTS_PUSH (
        EventType::ET_SNAP_GRID_OPTIONS_CHANGED, SNAP_GRID_EDITOR.get ());
    }
  else
    z_return_if_reached ();
}

void
activate_snap_events (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  z_return_if_fail (_variant);
  gsize        size;
  const char * variant = g_variant_get_string (_variant, &size);

  if (string_is_equal (variant, "timeline"))
    {
      SNAP_GRID_TIMELINE->snap_to_events_ = !SNAP_GRID_TIMELINE->snap_to_events_;
      EVENTS_PUSH (
        EventType::ET_SNAP_GRID_OPTIONS_CHANGED, SNAP_GRID_TIMELINE.get ());
    }
  else if (string_is_equal (variant, "editor"))
    {
      SNAP_GRID_EDITOR->snap_to_events_ = !SNAP_GRID_EDITOR->snap_to_events_;
      EVENTS_PUSH (
        EventType::ET_SNAP_GRID_OPTIONS_CHANGED, SNAP_GRID_EDITOR.get ());
    }
  else
    z_return_if_reached ();
}

static void
create_empty_track (Track::Type type)
{
  if (zrythm_app->check_and_show_trial_limit_error ())
    return;

  try
    {
      Track::create_empty_with_action (type);
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to create track"));
    }
}

static void
open_files_ready_cb (GtkFileDialog * dialog, GAsyncResult * res, void * user_data)
{
  GError *     err = NULL;
  GListModel * file_list =
    gtk_file_dialog_open_multiple_finish (dialog, res, &err);
  if (!file_list)
    {
      z_info ("Failed to retrieve file list: {}", err->message);
      g_error_free (err);
      return;
    }

  GStrvBuilder * uris_builder = g_strv_builder_new ();
  const guint    num_files = g_list_model_get_n_items (file_list);
  for (guint i = 0; i < num_files; i++)
    {
      GFile * file = G_FILE (g_list_model_get_item (file_list, i));
      char *  path = g_file_get_path (file);
      z_return_if_fail (path);
      g_object_unref (file);
      z_info ("Preparing to import file: {}...", path);
      g_free (path);
      char * uri = g_file_get_uri (file);
      g_strv_builder_add (uris_builder, uri);
    }

  char ** uris = g_strv_builder_end (uris_builder);
  if (!zrythm_app->check_and_show_trial_limit_error ())
    {
      try
        {
          StringArray files (uris);
          TRACKLIST->import_files (
            &files, nullptr, nullptr, nullptr, -1, nullptr, nullptr);
        }
      catch (const ZrythmException &e)
        {
          HANDLE_ERROR_LITERAL (err, _ ("Failed to import files"));
        }
    }
  g_strfreev (uris);
}

DEFINE_SIMPLE (activate_import_file)
{
  GtkFileDialog * dialog = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (dialog, _ ("Select File(s)"));
  gtk_file_dialog_set_modal (dialog, true);
  GtkFileFilter * filter = gtk_file_filter_new ();
  /* note: this handles MIDI as well (audio/midi, audio/x-midi) */
  gtk_file_filter_add_mime_type (filter, "audio/*");
  GListStore * list_store = g_list_store_new (GTK_TYPE_FILE_FILTER);
  g_list_store_append (list_store, filter);
  gtk_file_dialog_set_filters (dialog, G_LIST_MODEL (list_store));
  g_object_unref (list_store);
  gtk_file_dialog_open_multiple (
    dialog, GTK_WINDOW (MAIN_WINDOW), nullptr,
    (GAsyncReadyCallback) open_files_ready_cb, nullptr);
}

DEFINE_SIMPLE (activate_create_audio_track)
{
  create_empty_track (Track::Type::Audio);
}

DEFINE_SIMPLE (activate_create_midi_track)
{
  create_empty_track (Track::Type::Midi);
}

DEFINE_SIMPLE (activate_create_audio_bus_track)
{
  create_empty_track (Track::Type::AudioBus);
}

DEFINE_SIMPLE (activate_create_midi_bus_track)
{
  create_empty_track (Track::Type::MidiBus);
}

DEFINE_SIMPLE (activate_create_audio_group_track)
{
  create_empty_track (Track::Type::AudioGroup);
}

DEFINE_SIMPLE (activate_create_midi_group_track)
{
  create_empty_track (Track::Type::MidiGroup);
}

DEFINE_SIMPLE (activate_create_folder_track)
{
  create_empty_track (Track::Type::Folder);
}

DEFINE_SIMPLE (activate_duplicate_selected_tracks)
{
  if (zrythm_app->check_and_show_trial_limit_error ())
    return;

  TRACKLIST_SELECTIONS->select_foldable_children ();

  try
    {
      UNDO_MANAGER->perform (std::make_unique<CopyTracksAction> (
        *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
        *PORT_CONNECTIONS_MGR,
        TRACKLIST_SELECTIONS->get_lowest_track ()->pos_ + 1));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to duplicate tracks"));
    }
}

DEFINE_SIMPLE (activate_change_track_color)
{
  object_color_chooser_dialog_widget_run (
    GTK_WINDOW (MAIN_WINDOW), nullptr, TRACKLIST_SELECTIONS.get (), nullptr);
}

DEFINE_SIMPLE (activate_goto_start_marker)
{
  TRANSPORT->goto_start_marker ();
}

DEFINE_SIMPLE (activate_goto_end_marker)
{
  TRANSPORT->goto_end_marker ();
}

DEFINE_SIMPLE (activate_goto_prev_marker)
{
  TRANSPORT->goto_prev_marker ();
}

DEFINE_SIMPLE (activate_goto_next_marker)
{
  TRANSPORT->goto_next_marker ();
}

DEFINE_SIMPLE (activate_play_pause)
{
  if (TRANSPORT->is_rolling ())
    {
      TRANSPORT->request_pause (true);
      MidiEvents::panic_all ();
    }
  else if (TRANSPORT->is_paused ())
    {
      TRANSPORT->request_roll (true);
    }
}

DEFINE_SIMPLE (activate_record_play)
{
  if (TRANSPORT->is_rolling ())
    {
      TRANSPORT->request_pause (true);
      MidiEvents::panic_all ();
    }
  else if (TRANSPORT->is_paused ())
    {
      TRANSPORT->set_recording (true, true, true);
      TRANSPORT->request_roll (true);
    }
}

static void
on_delete_tracks_response (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           user_data)
{
  if (string_is_equal (response, "delete"))
    {
      try
        {
          UNDO_MANAGER->perform (std::make_unique<DeleteTracksAction> (
            *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
            *PORT_CONNECTIONS_MGR));
          UNDO_MANAGER->clear_stacks ();
          EVENTS_PUSH (EventType::ET_UNDO_REDO_ACTION_DONE, nullptr);
        }
      catch (const ZrythmException &e)
        {
          e.handle (_ ("Failed to delete tracks"));
        }
    }
}

void
activate_delete_selected_tracks (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  z_info ("deleting selected tracks");

  TRACKLIST_SELECTIONS->select_foldable_children ();

  if (TRACKLIST_SELECTIONS->contains_uninstantiated_plugin ())
    {
      GtkWidget * dialog = adw_message_dialog_new (
        GTK_WINDOW (MAIN_WINDOW), _ ("Delete Tracks?"),
        _ ("The selected tracks contain uninstantiated plugins. Deleting them "
           "will not be undoable and the undo history will be cleared. "
           "Continue with deletion?"));
      adw_message_dialog_add_responses (
        ADW_MESSAGE_DIALOG (dialog), "cancel", _ ("_Cancel"), "delete",
        _ ("_Delete"), nullptr);
      adw_message_dialog_set_response_appearance (
        ADW_MESSAGE_DIALOG (dialog), "delete", ADW_RESPONSE_DESTRUCTIVE);
      adw_message_dialog_set_default_response (
        ADW_MESSAGE_DIALOG (dialog), "cancel");
      adw_message_dialog_set_close_response (
        ADW_MESSAGE_DIALOG (dialog), "cancel");
      g_signal_connect (
        dialog, "response", G_CALLBACK (on_delete_tracks_response), nullptr);
      gtk_window_present (GTK_WINDOW (dialog));
      return;
    }

  try
    {
      UNDO_MANAGER->perform (std::make_unique<DeleteTracksAction> (
        *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
        *PORT_CONNECTIONS_MGR));
      EVENTS_PUSH (EventType::ET_UNDO_REDO_ACTION_DONE, nullptr);
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to delete tracks"));
    }
}

DEFINE_SIMPLE (activate_hide_selected_tracks)
{
  z_info ("hiding selected tracks");
  TRACKLIST_SELECTIONS->toggle_visibility ();
}

DEFINE_SIMPLE (activate_pin_selected_tracks)
{
  z_info ("pin/unpinning selected tracks");

  auto track = TRACKLIST_SELECTIONS->get_highest_track ();
  bool pin = !track->is_pinned ();

  try
    {
      if (pin)
        {
          UNDO_MANAGER->perform (std::make_unique<PinTracksAction> (
            *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
            *PORT_CONNECTIONS_MGR));
        }
      else
        {
          UNDO_MANAGER->perform (std::make_unique<UnpinTracksAction> (
            *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
            *PORT_CONNECTIONS_MGR));
        }
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to pin/unpin tracks"));
    }
}

// Helper function to perform action and handle exceptions
template <typename Action>
void
perform_tracks_bool_action (const char * err_msg, bool value)
{
  try
    {
      UNDO_MANAGER->perform (std::make_unique<Action> (
        *TRACKLIST_SELECTIONS->gen_tracklist_selections (), value));
    }
  catch (const ZrythmException &e)
    {
      e.handle (err_msg);
    }
}

DEFINE_SIMPLE (activate_solo_selected_tracks)
{
  perform_tracks_bool_action<SoloTracksAction> (
    _ ("Failed to solo tracks"), true);
}

DEFINE_SIMPLE (activate_unsolo_selected_tracks)
{
  perform_tracks_bool_action<SoloTracksAction> (
    _ ("Failed to unsolo tracks"), false);
}

DEFINE_SIMPLE (activate_mute_selected_tracks)
{
  perform_tracks_bool_action<MuteTracksAction> (
    _ ("Failed to mute tracks"), true);
}

DEFINE_SIMPLE (activate_unmute_selected_tracks)
{
  perform_tracks_bool_action<MuteTracksAction> (
    _ ("Failed to unmute tracks"), false);
}

DEFINE_SIMPLE (activate_listen_selected_tracks)
{
  perform_tracks_bool_action<ListenTracksAction> (
    _ ("Failed to listen tracks"), true);
}

DEFINE_SIMPLE (activate_unlisten_selected_tracks)
{
  perform_tracks_bool_action<ListenTracksAction> (
    _ ("Failed to unlisten tracks"), false);
}

DEFINE_SIMPLE (activate_enable_selected_tracks)
{
  perform_tracks_bool_action<EnableTracksAction> (
    _ ("Failed to enable tracks"), true);
}

DEFINE_SIMPLE (activate_disable_selected_tracks)
{
  perform_tracks_bool_action<EnableTracksAction> (
    _ ("Failed to disable tracks"), false);
}

void
change_state_dim_output (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int dim = g_variant_get_boolean (value);

  z_info ("dim is {}", dim);

  g_simple_action_set_state (action, value);
}

void
change_state_loop (GSimpleAction * action, GVariant * value, gpointer user_data)
{
  int enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  TRANSPORT->set_loop (enabled, true);
}

void
change_state_metronome (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  bool enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  TRANSPORT->set_metronome_enabled (enabled);
}

void
change_state_musical_mode (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  g_settings_set_boolean (S_UI, "musical-mode", enabled);
}

void
change_state_listen_notes (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  g_settings_set_boolean (S_UI, "listen-notes", enabled);
}

void
change_state_ghost_notes (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  g_settings_set_boolean (S_UI, "ghost-notes", enabled);
}

static void
do_quantize (const char * variant, bool quick)
{
  z_debug ("quantize opts: {}", variant);

  try
    {
      if (
        string_is_equal (variant, "timeline")
        || (string_is_equal (variant, "global") && PROJECT->last_selection_ == Project::SelectionType::Timeline))
        {
          if (quick)
            {
              UNDO_MANAGER->perform (
                std::make_unique<ArrangerSelectionsAction::QuantizeAction> (
                  *TL_SELECTIONS, *QUANTIZE_OPTIONS_TIMELINE));
            }
          else
            {
              QuantizeDialogWidget * quant =
                quantize_dialog_widget_new (QUANTIZE_OPTIONS_TIMELINE.get ());
              gtk_window_set_transient_for (
                GTK_WINDOW (quant), GTK_WINDOW (MAIN_WINDOW));
              z_gtk_dialog_run (GTK_DIALOG (quant), true);
            }
        }
      else if (
        string_is_equal (variant, "editor")
        || (string_is_equal (variant, "global") && PROJECT->last_selection_ == Project::SelectionType::Editor))
        {
          if (quick)
            {
              ArrangerSelections * sel = CLIP_EDITOR->get_arranger_selections ();
              z_return_if_fail (sel);

              UNDO_MANAGER->perform (
                std::make_unique<ArrangerSelectionsAction::QuantizeAction> (
                  *sel, *QUANTIZE_OPTIONS_EDITOR));
            }
          else
            {
              QuantizeDialogWidget * quant =
                quantize_dialog_widget_new (QUANTIZE_OPTIONS_EDITOR.get ());
              gtk_window_set_transient_for (
                GTK_WINDOW (quant), GTK_WINDOW (MAIN_WINDOW));
              z_gtk_dialog_run (GTK_DIALOG (quant), true);
            }
        }
      else
        {
          ui_show_message_printf (
            _ ("Invalid Selection For Quantize"),
            _ ("Must select either the timeline or the editor first. The current selection is %s"),
            ENUM_NAME (PROJECT->last_selection_));
        }
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to quantize"));
    }
}

void
activate_quick_quantize (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  z_return_if_fail (_variant);
  gsize        size;
  const char * variant = g_variant_get_string (_variant, &size);
  do_quantize (variant, true);
}

void
activate_quantize_options (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  z_return_if_fail (_variant);
  gsize        size;
  const char * variant = g_variant_get_string (_variant, &size);
  do_quantize (variant, false);
}

void
activate_mute_selection (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  z_return_if_fail (_variant);

  gsize                size;
  const char *         variant = g_variant_get_string (_variant, &size);
  ArrangerSelections * sel = NULL;
  if (string_is_equal (variant, "timeline"))
    {
      sel = TL_SELECTIONS.get ();
    }
  else if (string_is_equal (variant, "editor"))
    {
      sel = MIDI_SELECTIONS.get ();
    }
  else if (string_is_equal (variant, "global"))
    {
      sel = PROJECT->get_arranger_selections_for_last_selection ();
    }
  else
    {
      z_return_if_reached ();
    }

  if (sel)
    {
      try
        {
          UNDO_MANAGER->perform (std::make_unique<EditArrangerSelectionsAction> (
            *sel, nullptr, ArrangerSelectionsAction::EditType::Mute, false));
        }
      catch (const ZrythmException &e)
        {
          e.handle (_ ("Failed to mute selections"));
        }
    }

  z_debug ("mute/unmute selections");
}

DEFINE_SIMPLE (activate_merge_selection)
{
  z_info ("merge selections activated");

  if (
    TL_SELECTIONS->contains_only_regions ()
    && TL_SELECTIONS->get_num_objects () > 0)
    {
      ui_show_error_message (_ ("Merge Failed"), _ ("No regions selected"));
      return;
    }
  if (!TL_SELECTIONS->all_on_same_lane ())
    {
      ui_show_error_message (
        _ ("Merge Failed"), _ ("Selections must be on the same lane"));
      return;
    }
  if (TL_SELECTIONS->contains_looped ())
    {
      ui_show_error_message (
        _ ("Merge Failed"), _ ("Cannot merge looped regions"));
      return;
    }
  if (TL_SELECTIONS->get_num_objects () == 1)
    {
      /* nothing to do */
      return;
    }

  try
    {
      UNDO_MANAGER->perform (
        std::make_unique<ArrangerSelectionsAction::MergeAction> (*TL_SELECTIONS));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to merge selections"));
    }
}

void
activate_set_timebase_master (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  z_info ("set time base master");
}

void
activate_set_transport_client (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  z_info ("set transport client");
}

void
activate_unlink_jack_transport (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  z_info ("unlink jack transport");
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
    !gtk_widget_get_visible (GTK_WIDGET (MW_TIMELINE_EVENT_VIEWER));

  g_settings_set_boolean (S_UI, "timeline-event-viewer-visible", new_visibility);
  gtk_widget_set_visible (GTK_WIDGET (MW_TIMELINE_EVENT_VIEWER), new_visibility);
}

void
activate_toggle_editor_event_viewer (
  GSimpleAction * action,
  GVariant *      variant,
  gpointer        user_data)
{
  if (!MW_EDITOR_EVENT_VIEWER_STACK)
    return;

  int new_visibility =
    !gtk_widget_get_visible (GTK_WIDGET (MW_EDITOR_EVENT_VIEWER_STACK));

  g_settings_set_boolean (S_UI, "editor-event-viewer-visible", new_visibility);
  gtk_widget_set_visible (
    GTK_WIDGET (MW_EDITOR_EVENT_VIEWER_STACK), new_visibility);
  bot_dock_edge_widget_update_event_viewer_stack_page (MW_BOT_DOCK_EDGE);
}

DEFINE_SIMPLE (activate_insert_silence)
{
  if (!TRANSPORT->has_range_)
    return;

  auto [start, end] = TRANSPORT->get_range_positions ();

  try
    {
      UNDO_MANAGER->perform (
        std::make_unique<RangeInsertSilenceAction> (start, end));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to insert silence"));
    }
}

DEFINE_SIMPLE (activate_remove_range)
{
  if (!TRANSPORT->has_range_)
    return;

  auto [start, end] = TRANSPORT->get_range_positions ();

  try
    {
      UNDO_MANAGER->perform (std::make_unique<RangeRemoveAction> (start, end));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to remove silence"));
    }
}

DEFINE_SIMPLE (change_state_timeline_playhead_scroll_edges)
{
  int enabled = g_variant_get_boolean (variant);

  g_simple_action_set_state (action, variant);

  g_settings_set_boolean (S_UI, "timeline-playhead-scroll-edges", enabled);

  EVENTS_PUSH (EventType::ET_PLAYHEAD_SCROLL_MODE_CHANGED, nullptr);
}

DEFINE_SIMPLE (change_state_timeline_playhead_follow)
{
  int enabled = g_variant_get_boolean (variant);

  g_simple_action_set_state (action, variant);

  g_settings_set_boolean (S_UI, "timeline-playhead-follow", enabled);

  EVENTS_PUSH (EventType::ET_PLAYHEAD_SCROLL_MODE_CHANGED, nullptr);
}

DEFINE_SIMPLE (change_state_editor_playhead_scroll_edges)
{
  int enabled = g_variant_get_boolean (variant);

  g_simple_action_set_state (action, variant);

  g_settings_set_boolean (S_UI, "editor-playhead-scroll-edges", enabled);

  EVENTS_PUSH (EventType::ET_PLAYHEAD_SCROLL_MODE_CHANGED, nullptr);
}

DEFINE_SIMPLE (change_state_editor_playhead_follow)
{
  int enabled = g_variant_get_boolean (variant);

  g_simple_action_set_state (action, variant);

  g_settings_set_boolean (S_UI, "editor-playhead-follow", enabled);

  EVENTS_PUSH (EventType::ET_PLAYHEAD_SCROLL_MODE_CHANGED, nullptr);
}

/**
 * Common routine for applying undoable MIDI
 * functions.
 */
static void
do_midi_func (const MidiFunctionType type, const MidiFunctionOpts opts)
{
  auto sel = MIDI_SELECTIONS.get ();
  if (!sel->has_any ())
    {
      z_info ("no selections, doing nothing");
      return;
    }

  switch (type)
    {
      /* no options needed for these */
    case MidiFunctionType::FlipHorizontal:
    case MidiFunctionType::FlipVertical:
    case MidiFunctionType::Legato:
    case MidiFunctionType::Portato:
    case MidiFunctionType::Staccato:
      {
        try
          {
            UNDO_MANAGER->perform (
              EditArrangerSelectionsAction::create (*sel, type, opts));
          }
        catch (const ZrythmException &e)
          {
            e.handle (_ ("Failed to apply MIDI function"));
          }
      }
      break;
    default:
      {
        MidiFunctionDialogWidget * dialog =
          midi_function_dialog_widget_new (GTK_WINDOW (MAIN_WINDOW), type);
        gtk_window_present (GTK_WINDOW (dialog));
      }
    }
}

/**
 * Common routine for applying undoable automation functions.
 */
static void
do_automation_func (AutomationFunctionType type)
{
  auto sel = AUTOMATION_SELECTIONS.get ();
  if (!sel->has_any ())
    {
      z_info ("no selections, doing nothing");
      return;
    }

  try
    {
      UNDO_MANAGER->perform (EditArrangerSelectionsAction::create (*sel, type));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to apply automation function"));
    }
}

/**
 * Common routine for applying undoable audio functions.
 *
 * @param uri Plugin URI, if applying plugin.
 */
static void
do_audio_func (
  const AudioFunctionType type,
  const AudioFunctionOpts opts,
  const std::string *     uri)
{
  auto project_r = CLIP_EDITOR->get_region<AudioRegion> ();
  z_return_if_fail (project_r);
  // z_return_if_fail (Region::find (&CLIP_EDITOR->region_id));
  AUDIO_SELECTIONS->region_id_ = CLIP_EDITOR->region_id_;
  if (!AUDIO_SELECTIONS->has_any ())
    {
      z_info ("no selections, doing nothing");
      return;
    }

  auto sel = AUDIO_SELECTIONS->clone_unique ();

  if (!sel->validate ())
    {
      return;
    }

  try
    {
      SemaphoreRAII sem (PROJECT->save_sem_);
      UNDO_MANAGER->perform (
        EditArrangerSelectionsAction::create (*sel, type, opts, uri));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to apply audio function"));
    }
}

DEFINE_SIMPLE (activate_editor_function)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);

  Region * region = CLIP_EDITOR->get_region ();
  if (!region)
    return;

  MidiFunctionOpts mopts = {};

  switch (region->get_type ())
    {
    case RegionType::Midi:
      {
        if (string_is_equal (str, "crescendo"))
          {
            do_midi_func (MidiFunctionType::Crescendo, mopts);
          }
        else if (string_is_equal (str, "current"))
          {
            do_midi_func (
              ENUM_INT_TO_VALUE (
                MidiFunctionType, g_settings_get_int (S_UI, "midi-function")),
              mopts);
          }
        else if (string_is_equal (str, "flam"))
          {
            do_midi_func (MidiFunctionType::Flam, mopts);
          }
        else if (string_is_equal (str, "flip-horizontal"))
          {
            do_midi_func (MidiFunctionType::FlipHorizontal, mopts);
          }
        else if (string_is_equal (str, "flip-vertical"))
          {
            do_midi_func (MidiFunctionType::FlipVertical, mopts);
          }
        else if (string_is_equal (str, "legato"))
          {
            do_midi_func (MidiFunctionType::Legato, mopts);
          }
        else if (string_is_equal (str, "portato"))
          {
            do_midi_func (MidiFunctionType::Portato, mopts);
          }
        else if (string_is_equal (str, "staccato"))
          {
            do_midi_func (MidiFunctionType::Staccato, mopts);
          }
        else if (string_is_equal (str, "strum"))
          {
            do_midi_func (MidiFunctionType::Strum, mopts);
          }
        else
          {
            z_return_if_reached ();
          }
      }
      break;
    case RegionType::Automation:
      {
        if (string_is_equal (str, "current"))
          {
            do_automation_func (ENUM_INT_TO_VALUE (
              AutomationFunctionType,
              g_settings_get_int (S_UI, "automation-function")));
          }
        else if (string_is_equal (str, "flip-horizontal"))
          {
            do_automation_func (AutomationFunctionType::FlipHorizontal);
          }
        else if (string_is_equal (str, "flip-vertical"))
          {
            do_automation_func (AutomationFunctionType::FlipVertical);
          }
        else if (string_is_equal (str, "flatten"))
          {
            do_automation_func (AutomationFunctionType::Flatten);
          }
        else
          {
            z_return_if_reached ();
          }
      }
      break;
    case RegionType::Audio:
      {
        bool done = false;
        if (string_is_equal (str, "current"))
          {
            AudioFunctionOpts aopts = {};
            AudioFunctionType type = ENUM_INT_TO_VALUE (
              AudioFunctionType, g_settings_get_int (S_UI, "audio-function"));
            switch (type)
              {
              case AudioFunctionType::PitchShift:
                aopts.amount_ = g_settings_get_double (
                  S_UI, "audio-function-pitch-shift-ratio");
                break;
              default:
                break;
              }
            do_audio_func (type, aopts, nullptr);
            done = true;
          }

        for (
          size_t i = ENUM_VALUE_TO_INT (AudioFunctionType::Invert);
          i < ENUM_VALUE_TO_INT (AudioFunctionType::CustomPlugin); i++)
          {
            AudioFunctionType cur = ENUM_INT_TO_VALUE (AudioFunctionType, i);
            auto              audio_func_target =
              audio_function_get_action_target_for_type (cur);
            if (audio_func_target == str)
              {
                switch (cur)
                  {
                  case AudioFunctionType::PitchShift:
                    {
                      StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
                        _ ("Pitch Ratio"),
                        [] () {
                          return fmt::format (
                            "{:f}",
                            g_settings_get_double (
                              S_UI, "audio-function-pitch-shift-ratio"));
                        },
                        [] (auto &&ratio_str) {
                          double ratio = strtod (ratio_str.c_str (), nullptr);
                          if (ratio < 0.0001 || ratio > 100.0)
                            {
                              ui_show_error_message (
                                _ ("Invalid Pitch Ratio"),
                                _ ("Please enter a ratio between 0.0001 and 100."));
                              return;
                            }

                          AudioFunctionOpts opts;
                          opts.amount_ = ratio;
                          do_audio_func (
                            AudioFunctionType::PitchShift, opts, nullptr);
                        });
                      gtk_window_present (GTK_WINDOW (dialog));
                    }
                    return;
                  default:
                    {
                      AudioFunctionOpts aopts = {};
                      do_audio_func (cur, aopts, nullptr);
                      break;
                    }
                  }
              }
            done = true;
          }

        z_return_if_fail (done);
      }
      break;
    default:
      break;
    }
}

DEFINE_SIMPLE (activate_editor_function_lv2)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);

  Region * region = CLIP_EDITOR->get_region ();
  if (!region)
    return;

  auto descr = PLUGIN_MANAGER->find_plugin_from_uri (str);
  z_return_if_fail (descr);

  switch (region->get_type ())
    {
    case RegionType::Midi:
      {
      }
      break;
    case RegionType::Audio:
      {
        AudioFunctionOpts aopts = {};
        std::string       str_wrap = str;
        do_audio_func (AudioFunctionType::CustomPlugin, aopts, &str_wrap);
      }
      break;
    default:
      z_return_if_reached ();
      break;
    }
}

DEFINE_SIMPLE (activate_midi_editor_highlighting)
{
  const auto str = g_variant_get_string (variant, nullptr);

  const std::unordered_map<std::string_view, PianoRoll::Highlighting>
    highlightMap = {
      { "none",  PianoRoll::Highlighting::None  },
      { "chord", PianoRoll::Highlighting::Chord },
      { "scale", PianoRoll::Highlighting::Scale },
      { "both",  PianoRoll::Highlighting::Both  }
  };

  if (const auto it = highlightMap.find (str); it != highlightMap.end ())
    {
      PIANO_ROLL->set_highlighting (it->second);
    }
  else
    {
      z_warning ("Unknown highlighting option: {}", str);
    }
}

DEFINE_SIMPLE (activate_rename_track)
{
  z_return_if_fail (TRACKLIST_SELECTIONS->get_num_tracks () == 1);
  auto track = TRACKLIST_SELECTIONS->get_highest_track ();
  z_return_if_fail (track);
  StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
    _ ("Track name"), bind_member_function (*track, &Track::get_name),
    bind_member_function (*track, &Track::set_name_with_action));
  gtk_window_present (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_rename_arranger_object)
{
  z_debug ("rename arranger object");

  if (PROJECT->last_selection_ == Project::SelectionType::Timeline)
    {
      auto sel = arranger_widget_get_selections (MW_TIMELINE);
      if (sel->get_num_objects () == 1)
        {
          auto [obj, pos] = sel->get_first_object_and_pos (true);
          if (obj->has_name ())
            {
              auto named_obj = dynamic_cast<NameableObject *> (obj);
              z_return_if_fail (named_obj);
              StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
                _ ("Object name"),
                bind_member_function (*named_obj, &NameableObject::get_name),
                bind_member_function (
                  *named_obj, &NameableObject::set_name_with_action));
              gtk_window_present (GTK_WINDOW (dialog));
            }
        }
    }
  else if (PROJECT->last_selection_ == Project::SelectionType::Editor)
    {
      /* nothing can be renamed yet */
    }
}

void
activate_create_arranger_object (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  double       x, y;
  const char * str;
  g_variant_get (value, "(sdd)", &str, &x, &y);
  ArrangerWidget * arranger;
  sscanf (str, "%p", &arranger);
  z_debug ("called {:f} {:f}", x, y);
  arranger_widget_create_item (arranger, x, y, false);
  bool action_performed =
    arranger_widget_finish_creating_item_from_action (arranger, x, y);
  if (!action_performed)
    {
      ui_show_notification (
        _ ("An object cannot be created at the selected location."));
    }
}

static void
on_region_color_dialog_response (
  GtkDialog * dialog,
  gint        response_id,
  gpointer    user_data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      /* clone the selections before the change */
      auto sel_before = TL_SELECTIONS->clone_unique ();

      GdkRGBA color;
      gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (dialog), &color);

      for (auto r : TL_SELECTIONS->objects_ | type_is<Region> ())
        {
          /* change */
          r->use_color_ = true;
          r->color_ = color;
        }

      /* perform action */
      try
        {
          UNDO_MANAGER->perform (std::make_unique<EditArrangerSelectionsAction> (
            *sel_before, TL_SELECTIONS.get (),
            ArrangerSelectionsAction::EditType::Primitive, true));
        }
      catch (const ZrythmException &e)
        {
          e.handle (_ ("Failed to set region color"));
        }
    }

  gtk_window_destroy (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_change_region_color)
{
  if (!TL_SELECTIONS->contains_only_regions ())
    return;

  auto  r = dynamic_cast<Region *> (TL_SELECTIONS->objects_[0].get ());
  Color color;
  if (r->use_color_)
    {
      color = r->color_;
    }
  else
    {
      Track * track = r->get_track ();
      color = track->color_;
    }

  GtkColorChooserDialog * dialog =
    GTK_COLOR_CHOOSER_DIALOG (gtk_color_chooser_dialog_new (
      _ ("Region Color"), GTK_WINDOW (UI_ACTIVE_WINDOW_OR_NULL)));
  auto color_rgba = color.to_gdk_rgba ();
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (dialog), &color_rgba);

  g_signal_connect_after (
    G_OBJECT (dialog), "response", G_CALLBACK (on_region_color_dialog_response),
    nullptr);
  gtk_window_present (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_reset_region_color)
{
  if (!TL_SELECTIONS->contains_only_regions ())
    return;

  /* clone the selections before the change */
  auto sel_before = TL_SELECTIONS->clone_unique ();

  for (auto r : TL_SELECTIONS->objects_ | type_is<Region> ())
    {
      /* change */
      r->use_color_ = false;
    }

  /* perform action */
  try
    {
      UNDO_MANAGER->perform (std::make_unique<EditArrangerSelectionsAction> (
        *sel_before, TL_SELECTIONS.get (),
        ArrangerSelectionsAction::EditType::Primitive, true));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to reset region color"));
    }
}

DEFINE_SIMPLE (activate_move_automation_regions)
{
  gsize        size;
  const char * _str = g_variant_get_string (variant, &size);
  Port *       port = NULL;
  sscanf (_str, "%p", &port);
  z_return_if_fail (IS_PORT_AND_NONNULL (port));

  try
    {
      UNDO_MANAGER->perform (
        std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateTimelineAction> (
          *TL_SELECTIONS, true, 0, 0, 0, &port->id_, false));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to move automation regions"));
    }
}

DEFINE_SIMPLE (activate_add_region)
{
  if (TRACKLIST_SELECTIONS->get_num_tracks () == 0)
    return;

  /*Track * track = TRACKLIST_SELECTIONS->tracks[0];*/

  /* TODO add region with default size */
}

DEFINE_SIMPLE (activate_go_to_start)
{
  Position pos;
  TRANSPORT->move_playhead (&pos, F_PANIC, F_SET_CUE_POINT, true);
}

DEFINE_SIMPLE (activate_input_bpm)
{
  StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
    _ ("Please enter a BPM"),
    bind_member_function (*P_TEMPO_TRACK, &TempoTrack::get_current_bpm_as_str),
    bind_member_function (*P_TEMPO_TRACK, &TempoTrack::set_bpm_from_str));
  gtk_window_present (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_tap_bpm)
{
  ui_show_message_literal (
    _ ("Unimplemented"), _ ("Tap BPM not implemented yet"));
}

void
change_state_show_automation_values (
  GSimpleAction * action,
  GVariant *      value,
  gpointer        user_data)
{
  int enabled = g_variant_get_boolean (value);

  g_simple_action_set_state (action, value);

  g_settings_set_boolean (S_UI, "show-automation-values", enabled);

  EVENTS_PUSH (EventType::ET_AUTOMATION_VALUE_VISIBILITY_CHANGED, nullptr);
}

DEFINE_SIMPLE (activate_nudge_selection)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);

  double ticks = ARRANGER_SELECTIONS_DEFAULT_NUDGE_TICKS;
  bool   left = string_is_equal (str, "left");
  if (left)
    ticks = -ticks;

  ArrangerSelections * sel =
    PROJECT->get_arranger_selections_for_last_selection ();
  if (!sel || !sel->has_any ())
    return;

  if (sel->type_ == ArrangerSelections::Type::Audio)
    {
      AudioFunctionOpts aopts = {};
      do_audio_func (
        left ? AudioFunctionType::NudgeLeft : AudioFunctionType::NudgeRight,
        aopts, nullptr);
      return;
    }

  auto [start_obj, start_pos] = sel->get_first_object_and_pos (true);
  if (start_pos.ticks_ + ticks < 0)
    {
      z_info ("cannot nudge left");
      return;
    }

  try
    {
      UNDO_MANAGER->perform (
        std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateAction> (
          *sel, true, ticks, 0, 0, 0, 0, 0, nullptr, false));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to move selections"));
    }
}

DEFINE_SIMPLE (activate_detect_bpm)
{
  gsize         size;
  const char *  _str = g_variant_get_string (variant, &size);
  AudioRegion * r = NULL;
  sscanf (_str, "%p", &r);
  z_return_if_fail (r && r->is_region ());

  std::vector<float> candidates;
  bpm_t              bpm = r->detect_bpm (candidates);

  auto str = format_str (
    _ ("Detected BPM: {:.2f}\n\nCandidates: {}"), bpm,
    fmt::join (candidates, ", "));

  ui_show_message_literal (_ ("Error"), str.c_str ());
}

DEFINE_SIMPLE (activate_timeline_function)
{
  AudioFunctionType type = (AudioFunctionType) g_variant_get_int32 (variant);
  AudioFunctionOpts aopts;
  /* FIXME need to show a dialog here too to ask for the
   * pitch shift amount
   * TODO refactor code to avoid duplicating the code here */
  switch (type)
    {
    case AudioFunctionType::PitchShift:
      aopts.amount_ =
        g_settings_get_double (S_UI, "audio-function-pitch-shift-ratio");
      break;
    default:
      break;
    }

  int count = 0;
  for (auto r : TL_SELECTIONS->objects_ | type_is<Region> ())
    {
      auto sel = std::make_unique<AudioSelections> ();
      sel->region_id_ = r->id_;
      sel->has_selection_ = true;

      /* timeline start pos */
      sel->sel_start_ = r->clip_start_pos_;
      sel->sel_start_.add_ticks (r->pos_.ticks_);

      /* timeline end pos */
      sel->sel_end_ = r->loop_end_pos_;
      sel->sel_end_.add_ticks (r->pos_.ticks_);
      if (sel->sel_end_ > r->end_pos_)
        {
          sel->sel_end_ = r->end_pos_;
        }

      try
        {
          UNDO_MANAGER->perform (
            EditArrangerSelectionsAction::create (*sel, type, aopts, nullptr));
          UndoableAction * ua = UNDO_MANAGER->get_last_action ();
          ua->num_actions_ = ++count;
        }
      catch (const ZrythmException &e)
        {
          e.handle (_ ("Failed to apply audio function"));
        }
    }
}

DEFINE_SIMPLE (activate_export_midi_regions)
{
  export_midi_file_dialog_widget_run (
    GTK_WINDOW (MAIN_WINDOW), TL_SELECTIONS.get ());
}

static void
bounce_progress_close_cb (std::shared_ptr<Exporter> exporter)
{
  exporter->join_generic_thread ();
  exporter->post_export ();

  if (
    exporter->progress_info_->get_completion_type ()
    == ProgressInfo::CompletionType::SUCCESS)
    {
      /* create audio track with bounced material */
      auto [first_obj, first_pos] =
        TL_SELECTIONS->get_first_object_and_pos (true);
      exporter->create_audio_track_after_bounce (first_pos);
    }
}

DEFINE_SIMPLE (activate_quick_bounce_selections)
{
  Region * r = TL_SELECTIONS->contains_single_region ();
  if (!r)
    {
      z_warning ("no selections to bounce");
      return;
    }

  Exporter::Settings settings;
  settings.mode_ = Exporter::Mode::Regions;
  settings.set_bounce_defaults (Exporter::Format::WAV, "", r->name_);
  TL_SELECTIONS->mark_for_bounce (settings.bounce_with_parents_);

  auto exporter = std::make_shared<Exporter> (settings, nullptr, nullptr);
  exporter->prepare_tracks_for_export (*AUDIO_ENGINE, *TRANSPORT);

  /* start exporting in a new thread */
  exporter->begin_generic_thread ();

  /* create a progress dialog and block */
  ExportProgressDialogWidget * progress_dialog =
    export_progress_dialog_widget_new (
      exporter, true, bounce_progress_close_cb, false, F_CANCELABLE);
  adw_dialog_present (ADW_DIALOG (progress_dialog), GTK_WIDGET (MAIN_WINDOW));
}

DEFINE_SIMPLE (activate_bounce_selections)
{
  Region * r = TL_SELECTIONS->contains_single_region ();
  if (!r)
    {
      z_warning ("no selections to bounce");
      return;
    }

  BounceDialogWidget * dialog = bounce_dialog_widget_new (
    BounceDialogWidgetType::BOUNCE_DIALOG_REGIONS, r->name_);
  z_gtk_dialog_run (GTK_DIALOG (dialog), true);
}

DEFINE_SIMPLE (activate_set_curve_algorithm)
{
  CurveOptions::Algorithm curve_algo =
    (CurveOptions::Algorithm) g_variant_get_int32 (variant);

  /* clone the selections before the change */
  auto sel_before = AUTOMATION_SELECTIONS->clone_unique ();

  /* change */
  for (auto ap : AUTOMATION_SELECTIONS->objects_ | type_is<AutomationPoint> ())
    {
      ap->curve_opts_.algo_ = curve_algo;
    }

  /* perform action */
  try
    {
      UNDO_MANAGER->perform (std::make_unique<EditArrangerSelectionsAction> (
        *sel_before, AUTOMATION_SELECTIONS.get (),
        ArrangerSelectionsAction::EditType::Primitive, true));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to set curve algorithm"));
    }
}

static void
handle_region_fade_algo_preset (const std::string &pset_id, bool fade_in)
{
  auto arr = CurveFadePreset::get_fade_presets ();
  auto res =
    std::find_if (arr.begin (), arr.end (), [&pset_id] (const auto &cur_pset) {
      return cur_pset.id_ == pset_id;
    });
  if (res == arr.end ())
    {
      z_warning ("invalid preset id {}", pset_id.c_str ());
      return;
    }

  auto pset = *res;

  CurveOptions curve_opts = pset.opts_;

  auto r = TL_SELECTIONS->contains_single_region<AudioRegion> ();
  z_return_if_fail (r);

  auto sel_before = TL_SELECTIONS->clone_unique ();
  if (fade_in)
    {
      r->fade_in_opts_.algo_ = curve_opts.algo_;
      r->fade_in_opts_.curviness_ = curve_opts.curviness_;
    }
  else
    {
      r->fade_out_opts_.algo_ = curve_opts.algo_;
      r->fade_out_opts_.curviness_ = curve_opts.curviness_;
    }

  try
    {
      UNDO_MANAGER->perform (std::make_unique<EditArrangerSelectionsAction> (
        *sel_before, TL_SELECTIONS.get (),
        ArrangerSelectionsAction::EditType::Fades, true));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to edit fades"));
    }
}

DEFINE_SIMPLE (activate_set_region_fade_in_algorithm_preset)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);

  handle_region_fade_algo_preset (str, true);
}

DEFINE_SIMPLE (activate_set_region_fade_out_algorithm_preset)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);

  handle_region_fade_algo_preset (str, false);
}

DEFINE_SIMPLE (activate_arranger_object_view_info)
{
  gsize            size;
  const char *     str = g_variant_get_string (variant, &size);
  ArrangerObject * obj = NULL;
  sscanf (str, "%p", &obj);
  z_return_if_fail (IS_ARRANGER_OBJECT_AND_NONNULL (obj));

  ArrangerObjectInfoDialogWidget * dialog =
    arranger_object_info_dialog_widget_new (obj);
  gtk_window_present (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_save_chord_preset)
{
  z_debug ("save chord preset");

  bool have_custom = std::any_of (
    CHORD_PRESET_PACK_MANAGER->packs_.begin (),
    CHORD_PRESET_PACK_MANAGER->packs_.end (),
    [] (const auto &pack) { return !pack->is_standard_; });
  if (have_custom)
    {
      auto dialog =
        save_chord_preset_dialog_widget_new (GTK_WINDOW (MAIN_WINDOW));
      gtk_window_present (GTK_WINDOW (dialog));
    }
  else
    {
      ui_show_error_message (
        _ ("Cannot Save Chord Preset"),
        _ ("No custom preset packs found. "
           "Please create a preset pack first from the chord preset browser."));
    }
}

DEFINE_SIMPLE (activate_load_chord_preset)
{
  z_debug ("load chord preset");
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  int          pack_idx, pset_idx;
  sscanf (str, "%d,%d", &pack_idx, &pset_idx);

  int num_packs = CHORD_PRESET_PACK_MANAGER->get_num_packs ();
  z_return_if_fail_cmp (pack_idx, <, num_packs);

  auto * pack = CHORD_PRESET_PACK_MANAGER->get_pack_at (pack_idx);
  z_return_if_fail_cmp (pset_idx, <, static_cast<int> (pack->presets_.size ()));

  const auto &pset = pack->presets_[pset_idx];
  CHORD_EDITOR->apply_preset (pset, true);
}

DEFINE_SIMPLE (activate_load_chord_preset_from_scale)
{
  z_debug ("load chord preset from scale");
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  int          scale, root_note;
  sscanf (str, "%d,%d", &scale, &root_note);

  CHORD_EDITOR->apply_preset_from_scale (
    (MusicalScale::Type) scale, (MusicalNote) root_note, true);
}

DEFINE_SIMPLE (activate_transpose_chord_pad)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  z_debug ("transpose chord pad {}", str);

  if (string_is_equal (str, "up"))
    {
      CHORD_EDITOR->transpose_chords (true, true);
    }
  else if (string_is_equal (str, "down"))
    {
      CHORD_EDITOR->transpose_chords (false, true);
    }
  else
    {
      z_error ("invalid parameter {}", str);
    }
}

static void
on_chord_preset_pack_add_response (
  AdwMessageDialog * dialog,
  char *             response,
  ChordPresetPack *  pack)
{
  if (string_is_equal (response, "ok"))
    {
      if (!pack->name_.empty ())
        {
          CHORD_PRESET_PACK_MANAGER->add_pack (*pack, true);
        }
      else
        {
          ui_show_error_message (
            _ ("Add Pack Failed"), _ ("Failed to create pack"));
        }
    }

  delete pack;
}

DEFINE_SIMPLE (activate_add_chord_preset_pack)
{
  auto pack = new ChordPresetPack ("", false);

  StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
    _ ("Preset Pack Name"),
    bind_member_function (*pack, &ChordPresetPack::get_name),
    bind_member_function (*pack, &ChordPresetPack::set_name));
  g_signal_connect_after (
    G_OBJECT (dialog), "response",
    G_CALLBACK (on_chord_preset_pack_add_response), pack);
  gtk_window_present (GTK_WINDOW (dialog));
}

static void
on_delete_preset_chord_pack_response (
  AdwMessageDialog * dialog,
  char *             response,
  ChordPresetPack *  pack)
{
  if (string_is_equal (response, "delete"))
    {
      CHORD_PRESET_PACK_MANAGER->delete_pack (*pack, true);
    }
}

DEFINE_SIMPLE (activate_delete_chord_preset_pack)
{
  gsize             size;
  const char *      str = g_variant_get_string (variant, &size);
  ChordPresetPack * pack = NULL;
  sscanf (str, "%p", &pack);
  z_return_if_fail (pack);

  GtkWidget * dialog = adw_message_dialog_new (
    GTK_WINDOW (MAIN_WINDOW), _ ("Delete?"),
    _ ("Are you sure you want to delete this chord preset pack?"));
  adw_message_dialog_add_responses (
    ADW_MESSAGE_DIALOG (dialog), "cancel", _ ("_Cancel"), "delete",
    _ ("_Delete"), nullptr);
  adw_message_dialog_set_response_appearance (
    ADW_MESSAGE_DIALOG (dialog), "delete", ADW_RESPONSE_DESTRUCTIVE);
  adw_message_dialog_set_default_response (
    ADW_MESSAGE_DIALOG (dialog), "cancel");
  adw_message_dialog_set_close_response (ADW_MESSAGE_DIALOG (dialog), "cancel");
  g_signal_connect (
    dialog, "response", G_CALLBACK (on_delete_preset_chord_pack_response), pack);
  gtk_window_present (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_rename_chord_preset_pack)
{
  gsize             size;
  const char *      str = g_variant_get_string (variant, &size);
  ChordPresetPack * pack = NULL;
  sscanf (str, "%p", &pack);
  z_return_if_fail (pack);

  StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
    _ ("Preset Pack Name"),
    bind_member_function (*pack, &ChordPresetPack::get_name),
    bind_member_function (*pack, &ChordPresetPack::set_name));
  gtk_window_present (GTK_WINDOW (dialog));
}

static void
on_delete_chord_preset_response (
  AdwMessageDialog * dialog,
  char *             response,
  ChordPreset *      pset)
{
  if (string_is_equal (response, "delete"))
    {
      CHORD_PRESET_PACK_MANAGER->delete_preset (*pset, true);
    }
}

DEFINE_SIMPLE (activate_delete_chord_preset)
{
  gsize         size;
  const char *  str = g_variant_get_string (variant, &size);
  ChordPreset * pset = NULL;
  sscanf (str, "%p", &pset);
  z_return_if_fail (pset);

  GtkWidget * dialog = adw_message_dialog_new (
    GTK_WINDOW (MAIN_WINDOW), _ ("Delete?"),
    _ ("Are you sure you want to delete this chord preset?"));
  adw_message_dialog_add_responses (
    ADW_MESSAGE_DIALOG (dialog), "cancel", _ ("_Cancel"), "delete",
    _ ("_Delete"), nullptr);
  adw_message_dialog_set_response_appearance (
    ADW_MESSAGE_DIALOG (dialog), "delete", ADW_RESPONSE_DESTRUCTIVE);
  adw_message_dialog_set_default_response (
    ADW_MESSAGE_DIALOG (dialog), "cancel");
  adw_message_dialog_set_close_response (ADW_MESSAGE_DIALOG (dialog), "cancel");
  g_signal_connect (
    dialog, "response", G_CALLBACK (on_delete_chord_preset_response), pset);
  gtk_window_present (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_rename_chord_preset)
{
  gsize         size;
  const char *  str = g_variant_get_string (variant, &size);
  ChordPreset * pset = NULL;
  sscanf (str, "%p", &pset);
  z_return_if_fail (pset);

  StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
    _ ("Preset Name"), bind_member_function (*pset, &ChordPreset::get_name),
    bind_member_function (*pset, &ChordPreset::set_name));
  gtk_window_present (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_reset_stereo_balance)
{
  gsize         size;
  const char *  str = g_variant_get_string (variant, &size);
  ControlPort * port = NULL;
  sscanf (str, "%p", &port);
  z_return_if_fail (port);

  Track * track = port->get_track (true);
  z_return_if_fail (track);
  try
    {
      UNDO_MANAGER->perform (std::make_unique<SingleTrackFloatAction> (
        EditTracksAction::EditType::Pan, track, port->control_, 0.5f, false));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to reset balance"));
    }
}

DEFINE_SIMPLE (activate_plugin_toggle_enabled)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  zrythm::plugins::Plugin * pl = NULL;
  sscanf (str, "%p", &pl);
  z_return_if_fail (pl);

  bool new_val = !pl->is_enabled (false);
  if (!pl->is_selected ())
    {
      pl->select (true, true);
    }

  try
    {
      UNDO_MANAGER->perform (std::make_unique<MixerSelectionsChangeStatusAction> (
        *MIXER_SELECTIONS->gen_full_from_this (), new_val));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to change plugin states"));
    }
}

DEFINE_SIMPLE (activate_plugin_change_load_behavior)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  zrythm::plugins::Plugin * pl = NULL;
  char         new_behavior[600];
  sscanf (str, "%p,%s", &pl, new_behavior);
  z_return_if_fail (pl);

  pl->select (true, true);

  plugins::CarlaBridgeMode new_bridge_mode = plugins::CarlaBridgeMode::None;
  if (string_is_equal (new_behavior, "normal"))
    {
      new_bridge_mode = plugins::CarlaBridgeMode::None;
    }
  else if (string_is_equal (new_behavior, "ui"))
    {
      new_bridge_mode = plugins::CarlaBridgeMode::UI;
    }
  else if (string_is_equal (new_behavior, "full"))
    {
      new_bridge_mode = plugins::CarlaBridgeMode::Full;
    }

  try
    {
      UNDO_MANAGER->perform (
        std::make_unique<MixerSelectionsChangeLoadBehaviorAction> (
          *MIXER_SELECTIONS->gen_full_from_this (), new_bridge_mode));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to change plugin load behavior"));
    }
}

DEFINE_SIMPLE (activate_plugin_inspect)
{
  left_dock_edge_widget_refresh_with_page (
    MW_LEFT_DOCK_EDGE, LeftDockEdgeTab::LEFT_DOCK_EDGE_TAB_PLUGIN);
}

static void
delete_plugins (bool clear_stacks)
{
  try
    {
      UNDO_MANAGER->perform (std::make_unique<MixerSelectionsDeleteAction> (
        *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR));
      if (clear_stacks)
        {
          UNDO_MANAGER->clear_stacks ();
          EVENTS_PUSH (EventType::ET_UNDO_REDO_ACTION_DONE, nullptr);
        }
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to delete plugins"));
    }
}

static void
on_delete_plugins_response (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           user_data)
{
  if (string_is_equal (response, "delete"))
    {
      delete_plugins (true);
    }
}

DEFINE_SIMPLE (activate_mixer_selections_delete)
{

  if (MIXER_SELECTIONS->contains_uninstantiated_plugin ())
    {
      GtkWidget * dialog = adw_message_dialog_new (
        GTK_WINDOW (MAIN_WINDOW), _ ("Delete Plugins?"),
        _ ("The selection contains uninstantiated plugins. Deleting them "
           "will not be undoable and the undo history will be cleared."));
      adw_message_dialog_add_responses (
        ADW_MESSAGE_DIALOG (dialog), "cancel", _ ("_Cancel"), "delete",
        _ ("_Delete"), nullptr);
      adw_message_dialog_set_response_appearance (
        ADW_MESSAGE_DIALOG (dialog), "delete", ADW_RESPONSE_DESTRUCTIVE);
      adw_message_dialog_set_default_response (
        ADW_MESSAGE_DIALOG (dialog), "cancel");
      adw_message_dialog_set_close_response (
        ADW_MESSAGE_DIALOG (dialog), "cancel");
      g_signal_connect (
        dialog, "response", G_CALLBACK (on_delete_plugins_response), nullptr);
      gtk_window_present (GTK_WINDOW (dialog));
      return;
    }

  delete_plugins (false);
}

DEFINE_SIMPLE (activate_reset_fader)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  Fader *      fader = NULL;
  sscanf (str, "%p", &fader);
  z_return_if_fail (fader);

  if (fader->type_ == Fader::Type::AudioChannel)
    {
      Channel * ch = fader->get_channel ();
      ch->reset_fader (true);
    }
  else
    {
      fader->set_amp (1.0);
    }
}

DEFINE_SIMPLE (activate_reset_control)
{
  gsize         size;
  const char *  str = g_variant_get_string (variant, &size);
  ControlPort * port = NULL;
  sscanf (str, "%p", &port);
  z_return_if_fail (IS_PORT_AND_NONNULL (port));

  try
    {
      UNDO_MANAGER->perform (std::make_unique<PortActionResetControl> (*port));
    }
  catch (const ZrythmException &e)
    {
      e.handle (
        format_str (_ ("Failed to reset control {}"), port->get_label ()));
    }
}

DEFINE_SIMPLE (activate_port_view_info)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  Port *       port = NULL;
  sscanf (str, "%p", &port);
  z_return_if_fail (IS_PORT_AND_NONNULL (port));

  PortInfoDialogWidget * dialog = port_info_dialog_widget_new (port);
  gtk_window_present (GTK_WINDOW (dialog));
}

DEFINE_SIMPLE (activate_port_connection_remove)
{
  try
    {
      UNDO_MANAGER->perform (std::make_unique<PortConnectionDisconnectAction> (
        MW_PORT_CONNECTIONS_TREE->src_port->id_,
        MW_PORT_CONNECTIONS_TREE->dest_port->id_));
    }
  catch (const ZrythmException &e)
    {
      e.handle (format_str (
        _ ("Failed to disconnect {} from {}"),
        MW_PORT_CONNECTIONS_TREE->src_port->get_label ().c_str (),
        MW_PORT_CONNECTIONS_TREE->src_port->get_label (),
        MW_PORT_CONNECTIONS_TREE->dest_port->get_label ()));
    }
}

DEFINE_SIMPLE (activate_panel_file_browser_add_bookmark)
{
  gsize            size;
  const char *     str = g_variant_get_string (variant, &size);
  FileDescriptor * sfile = NULL;
  sscanf (str, "%p", &sfile);
  z_return_if_fail (sfile != nullptr);

  gZrythm->get_file_manager ().add_location_and_save (sfile->abs_path_.c_str ());

  EVENTS_PUSH (EventType::ET_FILE_BROWSER_BOOKMARK_ADDED, nullptr);
}

static void
on_bookmark_delete_response (
  AdwMessageDialog * dialog,
  char *             response,
  const char *       path)
{
  if (string_is_equal (response, "delete"))
    {
      gZrythm->get_file_manager ().remove_location_and_save (path, true);

      EVENTS_PUSH (EventType::ET_FILE_BROWSER_BOOKMARK_DELETED, nullptr);
    }
}

DEFINE_SIMPLE (activate_panel_file_browser_delete_bookmark)
{
  FileBrowserLocation * loc =
    panel_file_browser_widget_get_selected_bookmark (MW_PANEL_FILE_BROWSER);
  z_return_if_fail (loc);

  if (loc->special_location_ > FileManagerSpecialLocation::FILE_MANAGER_NONE)
    {
      ui_show_error_message (
        _ ("Cannot Delete Bookmark"), _ ("Cannot delete standard bookmark"));
      return;
    }

  GtkWidget * dialog = adw_message_dialog_new (
    GTK_WINDOW (MAIN_WINDOW), _ ("Delete?"),
    _ ("Are you sure you want to delete this bookmark?"));
  adw_message_dialog_add_responses (
    ADW_MESSAGE_DIALOG (dialog), "cancel", _ ("_Cancel"), "delete",
    _ ("_Delete"), nullptr);
  adw_message_dialog_set_response_appearance (
    ADW_MESSAGE_DIALOG (dialog), "delete", ADW_RESPONSE_DESTRUCTIVE);
  adw_message_dialog_set_default_response (
    ADW_MESSAGE_DIALOG (dialog), "cancel");
  adw_message_dialog_set_close_response (ADW_MESSAGE_DIALOG (dialog), "cancel");
  g_signal_connect (
    dialog, "response", G_CALLBACK (on_bookmark_delete_response),
    (gpointer) (loc->path_.c_str ()));
  gtk_window_present (GTK_WINDOW (dialog));
}

static void
activate_plugin_setting (const PluginSetting &setting)
{
  if (zrythm_app->check_and_show_trial_limit_error ())
    return;

  setting.activate ();
}

DEFINE_SIMPLE (activate_plugin_browser_add_to_project)
{
  gsize              size;
  const char *       str = g_variant_get_string (variant, &size);
  plugins::PluginDescriptor * descr = NULL;
  sscanf (str, "%p", &descr);
  z_return_if_fail (descr != nullptr);

  activate_plugin_setting (PluginSetting (*descr));
}

DEFINE_SIMPLE (activate_plugin_browser_add_to_project_carla)
{
  gsize              size;
  const char *       str = g_variant_get_string (variant, &size);
  zrythm::plugins::PluginDescriptor * descr = NULL;
  sscanf (str, "%p", &descr);
  z_return_if_fail (descr != nullptr);

  PluginSetting setting (*descr);
  setting.open_with_carla_ = true;
  setting.bridge_mode_ = plugins::CarlaBridgeMode::None;
  activate_plugin_setting (setting);
}

DEFINE_SIMPLE (activate_plugin_browser_add_to_project_bridged_ui)
{
  gsize              size;
  const char *       str = g_variant_get_string (variant, &size);
  zrythm::plugins::PluginDescriptor * descr = NULL;
  sscanf (str, "%p", &descr);
  z_return_if_fail (descr != nullptr);

  PluginSetting setting (*descr);
  setting.open_with_carla_ = true;
  setting.bridge_mode_ = zrythm::plugins::CarlaBridgeMode::UI;
  activate_plugin_setting (setting);
}

DEFINE_SIMPLE (activate_plugin_browser_add_to_project_bridged_full)
{
  gsize              size;
  const char *       str = g_variant_get_string (variant, &size);
  zrythm::plugins::PluginDescriptor * descr = NULL;
  sscanf (str, "%p", &descr);
  z_return_if_fail (descr != nullptr);

  PluginSetting setting (*descr);
  setting.open_with_carla_ = true;
  setting.bridge_mode_ = zrythm::plugins::CarlaBridgeMode::Full;
  activate_plugin_setting (setting);
}

DEFINE_SIMPLE (change_state_plugin_browser_toggle_generic_ui) { }

DEFINE_SIMPLE (activate_plugin_browser_add_to_collection)
{
  gsize                    size;
  const char *             str = g_variant_get_string (variant, &size);
  zrythm::plugins::PluginCollection * collection = NULL;
  const zrythm::plugins::PluginDescriptor * descr = NULL;
  sscanf (str, "%p,%p", &collection, &descr);
  z_return_if_fail (collection != nullptr);
  z_return_if_fail (descr != nullptr);

  collection->add_descriptor (*descr);
  PLUGIN_MANAGER->collections_->serialize_to_file ();

  EVENTS_PUSH (EventType::ET_PLUGIN_COLLECTIONS_CHANGED, nullptr);
}

DEFINE_SIMPLE (activate_plugin_browser_remove_from_collection)
{
  gsize                    size;
  const char *             str = g_variant_get_string (variant, &size);
  zrythm::plugins::PluginCollection * collection = NULL;
  const zrythm::plugins::PluginDescriptor * descr = NULL;
  sscanf (str, "%p,%p", &collection, &descr);
  z_return_if_fail (collection != nullptr);
  z_return_if_fail (descr != nullptr);

  collection->remove_descriptor (*descr);
  PLUGIN_MANAGER->collections_->serialize_to_file ();

  EVENTS_PUSH (EventType::ET_PLUGIN_COLLECTIONS_CHANGED, nullptr);
}

DEFINE_SIMPLE (activate_plugin_browser_reset)
{
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);

  GtkListView * list_view = NULL;
  if (string_is_equal (str, "category"))
    {
      list_view = MW_PLUGIN_BROWSER->category_list_view;
    }
  else if (string_is_equal (str, "author"))
    {
      list_view = MW_PLUGIN_BROWSER->author_list_view;
    }
  else if (string_is_equal (str, "protocol"))
    {
      list_view = MW_PLUGIN_BROWSER->protocol_list_view;
    }
  else if (string_is_equal (str, "collection"))
    {
      list_view = MW_PLUGIN_BROWSER->collection_list_view;
    }

  if (list_view)
    {
      GtkSelectionModel * model = gtk_list_view_get_model (list_view);
      gtk_selection_model_unselect_all (model);
    }
}

static void
on_plugin_collection_add_response (
  AdwMessageDialog *                  dialog,
  char *                              response,
  zrythm::plugins::PluginCollection * collection)
{
  if (string_is_equal (response, "ok"))
    {
      if (!collection->name_.empty ())
        {
          z_debug ("accept collection");
          PLUGIN_MANAGER->collections_->add (*collection, F_SERIALIZE);
          EVENTS_PUSH (EventType::ET_PLUGIN_COLLECTIONS_CHANGED, nullptr);
        }
      else
        {
          z_info ("invalid collection name (empty)");
        }
    }

  delete collection;
}

DEFINE_SIMPLE (activate_plugin_collection_add)
{
  auto * collection = new zrythm::plugins::PluginCollection ();

  StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
    _ ("Collection name"),
    bind_member_function (
      *collection, &zrythm::plugins::PluginCollection::get_name),
    bind_member_function (
      *collection, &zrythm::plugins::PluginCollection::set_name));
  g_signal_connect_after (
    G_OBJECT (dialog), "response",
    G_CALLBACK (on_plugin_collection_add_response), collection);
  gtk_window_present (GTK_WINDOW (dialog));
}

static void
on_plugin_collection_rename_response (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           user_data)
{
  PLUGIN_MANAGER->collections_->serialize_to_file ();

  EVENTS_PUSH (EventType::ET_PLUGIN_COLLECTIONS_CHANGED, nullptr);
}

DEFINE_SIMPLE (activate_plugin_collection_rename)
{
  if (MW_PLUGIN_BROWSER->selected_collections.size () != 1)
    {
      z_warning ("should not be allowed");
      return;
    }
  auto collection = MW_PLUGIN_BROWSER->selected_collections.front ();

  StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
    _ ("Collection name"),
    bind_member_function (
      *collection, &zrythm::plugins::PluginCollection::get_name),
    bind_member_function (
      *collection, &zrythm::plugins::PluginCollection::set_name));
  g_signal_connect_after (
    G_OBJECT (dialog), "response",
    G_CALLBACK (on_plugin_collection_rename_response), collection);
  gtk_window_present (GTK_WINDOW (dialog));
}

static void
on_delete_plugin_collection_response (
  AdwMessageDialog *                  dialog,
  char *                              response,
  zrythm::plugins::PluginCollection * collection)
{
  if (string_is_equal (response, "delete"))
    {
      PLUGIN_MANAGER->collections_->remove (*collection, true);

      EVENTS_PUSH (EventType::ET_PLUGIN_COLLECTIONS_CHANGED, nullptr);
    }
}

DEFINE_SIMPLE (activate_plugin_collection_remove)
{
  if (MW_PLUGIN_BROWSER->selected_collections.empty ())
    {
      z_warning ("should not be allowed");
      return;
    }

  auto * collection = MW_PLUGIN_BROWSER->selected_collections.front ();

  int result = GTK_RESPONSE_YES;
  if (!collection->descriptors_.empty ())
    {
      GtkWidget * dialog = adw_message_dialog_new (
        GTK_WINDOW (MAIN_WINDOW), _ ("Delete?"), nullptr);
      adw_message_dialog_format_body_markup (
        ADW_MESSAGE_DIALOG (dialog),
        _ ("The collection '%s' contains %zu plugins. "
           "Are you sure you want to remove it?"),
        collection->name_.c_str (), collection->descriptors_.size ());
      adw_message_dialog_add_responses (
        ADW_MESSAGE_DIALOG (dialog), "cancel", _ ("_Cancel"), "delete",
        _ ("_Delete"), nullptr);
      adw_message_dialog_set_response_appearance (
        ADW_MESSAGE_DIALOG (dialog), "delete", ADW_RESPONSE_DESTRUCTIVE);
      adw_message_dialog_set_default_response (
        ADW_MESSAGE_DIALOG (dialog), "cancel");
      adw_message_dialog_set_close_response (
        ADW_MESSAGE_DIALOG (dialog), "cancel");
      g_signal_connect (
        dialog, "response", G_CALLBACK (on_delete_plugin_collection_response),
        collection);
      gtk_window_present (GTK_WINDOW (dialog));
      return;
    }

  if (result == GTK_RESPONSE_YES)
    {
      PLUGIN_MANAGER->collections_->remove (*collection, F_SERIALIZE);

      EVENTS_PUSH (EventType::ET_PLUGIN_COLLECTIONS_CHANGED, nullptr);
    }
}

DEFINE_SIMPLE (activate_track_set_midi_channel)
{
  /* "{track index},{lane index or -1},"
   * "{midi channel 1-16 or 0 for lane to inherit}"
   */
  gsize        size;
  const char * str = g_variant_get_string (variant, &size);
  int          track_idx, lane_idx, midi_ch;
  sscanf (str, "%d,%d,%d", &track_idx, &lane_idx, &midi_ch);

  auto track =
    dynamic_cast<PianoRollTrack *> (TRACKLIST->tracks_[track_idx].get ());
  z_return_if_fail (track != nullptr);
  if (lane_idx >= 0)
    {
      auto &lane = track->lanes_[lane_idx];
      z_info (
        "setting lane '%s' (%d) midi channel to %d", lane->name_, lane_idx,
        midi_ch);
      lane->midi_ch_ = (midi_byte_t) midi_ch;
    }
  else
    {
      z_info (
        "setting track '%s' (%d) midi channel to %d", track->name_, track_idx,
        midi_ch);
      track->midi_ch_ = (midi_byte_t) midi_ch;
    }
}

static void
bounce_selected_tracks_progress_close_cb (std::shared_ptr<Exporter> exporter)
{
  exporter->join_generic_thread ();

  exporter->post_export ();

  const ProgressInfo &pinfo = *exporter->progress_info_;

  if (pinfo.get_completion_type () == ProgressInfo::CompletionType::SUCCESS)
    {
      /* create audio track with bounced material */
      auto m = P_MARKER_TRACK->get_start_marker ();
      exporter->create_audio_track_after_bounce (m->pos_);
    }
}

DEFINE_SIMPLE (activate_quick_bounce_selected_tracks)
{
  auto track = TRACKLIST_SELECTIONS->get_highest_track ();

  Exporter::Settings settings{};
  settings.mode_ = Exporter::Mode::Tracks;
  settings.set_bounce_defaults (Exporter::Format::WAV, "", track->name_);
  TRACKLIST_SELECTIONS->mark_for_bounce (settings.bounce_with_parents_, false);
  auto exporter = std::make_shared<Exporter> (settings);
  exporter->prepare_tracks_for_export (*AUDIO_ENGINE, *TRANSPORT);

  /* start exporting in a new thread */
  exporter->begin_generic_thread ();

  /* create a progress dialog and block */
  ExportProgressDialogWidget * progress_dialog =
    export_progress_dialog_widget_new (
      exporter, true, bounce_selected_tracks_progress_close_cb, false,
      F_CANCELABLE);
  adw_dialog_present (ADW_DIALOG (progress_dialog), GTK_WIDGET (MAIN_WINDOW));
}

DEFINE_SIMPLE (activate_bounce_selected_tracks)
{
  auto                 track = TRACKLIST_SELECTIONS->get_highest_track ();
  BounceDialogWidget * dialog = bounce_dialog_widget_new (
    BounceDialogWidgetType::BOUNCE_DIALOG_TRACKS, track->name_);
  z_gtk_dialog_run (GTK_DIALOG (dialog), true);
}

static void
handle_direct_out_change (int direct_out_idx, bool new_group)
{
  auto sel_before = TRACKLIST_SELECTIONS->gen_tracklist_selections ();

  Track * direct_out = nullptr;
  if (new_group)
    {
      direct_out =
        add_tracks_to_group_dialog_widget_get_track (sel_before.get ());
      if (!direct_out)
        return;
    }
  else
    {
      direct_out = TRACKLIST->get_track (direct_out_idx);
    }
  z_return_if_fail (direct_out);

  /* skip if all selected tracks already connected to direct out */
  bool need_change = false;
  for (auto &track : sel_before->tracks_)
    {
      auto &prj_track = TRACKLIST->tracks_[track->pos_];
      if (!prj_track->has_channel ())
        return;

      if (prj_track->out_signal_type_ != direct_out->in_signal_type_)
        {
          z_debug ("mismatching signal type");
          return;
        }

      auto prj_ch_track = dynamic_cast<ChannelTrack *> (prj_track.get ());
      auto ch = prj_ch_track->get_channel ();
      z_return_if_fail (ch);
      if (ch->get_output_track () != direct_out)
        {
          need_change = true;
          break;
        }
    }

  if (!need_change)
    {
      z_debug ("no direct out change needed");
      return;
    }

  if (new_group)
    {
      /* reset the selections */
      TRACKLIST_SELECTIONS->clear (true);
      for (auto &track : sel_before->tracks_)
        {
          auto &prj_track = TRACKLIST->tracks_[track->pos_];
          prj_track->select (true, false, false);
        }
    }

  auto prev_action = UNDO_MANAGER->get_last_action ();

  try
    {
      UNDO_MANAGER->perform (std::make_unique<ChangeTracksDirectOutAction> (
        *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
        *PORT_CONNECTIONS_MGR, *direct_out));
      auto ua = UNDO_MANAGER->get_last_action ();
      if (new_group)
        {
          /* see add_tracks_to_group_dialog for prev action */
          ua->num_actions_ = prev_action->num_actions_ + 1;
        }
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to change direct output"));
    }
}

DEFINE_SIMPLE (activate_selected_tracks_direct_out_to)
{
  int direct_out_pos = g_variant_get_int32 (variant);
  handle_direct_out_change (direct_out_pos, false);
}

DEFINE_SIMPLE (activate_selected_tracks_direct_out_new)
{
  handle_direct_out_change (-1, true);
}

DEFINE_SIMPLE (activate_toggle_track_passthrough_input)
{
  auto track =
    dynamic_cast<PianoRollTrack *> (TRACKLIST_SELECTIONS->get_highest_track ());
  z_return_if_fail (track);
  z_debug (
    "setting track '%s' passthrough MIDI input to %d", track->name_,
    !track->passthrough_midi_input_);
  track->passthrough_midi_input_ = !track->passthrough_midi_input_;
}

DEFINE_SIMPLE (activate_show_used_automation_lanes_on_selected_tracks)
{
  for (const auto &track_name : TRACKLIST_SELECTIONS->track_names_)
    {
      auto track = TRACKLIST->find_track_by_name<AutomatableTrack> (track_name);
      if (!track)
        continue;

      auto &atl = track->get_automation_tracklist ();

      bool automation_vis_changed = false;
      for (auto &at : atl.ats_)
        {
          if (at->contains_automation () && !at->visible_)
            {
              atl.set_at_visible (*at, true);
              automation_vis_changed = true;
            }
        }

      if (!track->automation_visible_)
        {
          automation_vis_changed = true;
          track->automation_visible_ = true;

          if (track->is_tempo ())
            {
              ui_show_warning_for_tempo_track_experimental_feature ();
            }
        }

      if (automation_vis_changed)
        {
          EVENTS_PUSH (EventType::ET_TRACK_AUTOMATION_VISIBILITY_CHANGED, track);
        }
    }
}

DEFINE_SIMPLE (activate_hide_unused_automation_lanes_on_selected_tracks)
{
  for (const auto &track_name : TRACKLIST_SELECTIONS->track_names_)
    {
      auto track = TRACKLIST->find_track_by_name<AutomatableTrack> (track_name);
      if (!track)
        continue;

      auto &atl = track->get_automation_tracklist ();

      bool automation_vis_changed = false;
      for (auto &at : atl.ats_)
        {
          if (!at->contains_automation () && at->visible_)
            {
              atl.set_at_visible (*at, false);
              automation_vis_changed = true;
            }
        }

      if (automation_vis_changed)
        {
          EVENTS_PUSH (EventType::ET_TRACK_AUTOMATION_VISIBILITY_CHANGED, track);
        }
    }
}

DEFINE_SIMPLE (activate_append_track_objects_to_selection)
{
  int track_pos = g_variant_get_int32 (variant);

  auto track = TRACKLIST->get_track<LanedTrack> (track_pos);
  z_return_if_fail (track);

  auto track_variant = convert_to_variant<LanedTrackPtrVariant> (track);
  std::visit (
    [&] (auto &&casted_track) {
      for (auto &lane : casted_track->lanes_)
        {
          for (auto &r : lane->regions_)
            {
              r->select (true, true, false);
            }
        }
    },
    track_variant);
}

DEFINE_SIMPLE (activate_append_lane_objects_to_selection)
{
  int track_pos, lane_pos;
  g_variant_get (variant, "(ii)", &track_pos, &lane_pos);

  auto track = TRACKLIST->get_track<LanedTrack> (track_pos);
  z_return_if_fail (track);

  auto track_variant = convert_to_variant<LanedTrackPtrVariant> (track);
  std::visit (
    [&] (auto &&casted_track) {
      auto &lane = casted_track->lanes_[lane_pos];
      for (auto &r : lane->regions_)
        {
          r->select (true, true, false);
        }
    },
    track_variant);
}

DEFINE_SIMPLE (activate_append_lane_automation_regions_to_selection)
{
  int track_pos, at_index;
  g_variant_get (variant, "(ii)", &track_pos, &at_index);
  auto track = TRACKLIST->get_track<AutomatableTrack> (track_pos);
  z_return_if_fail (track);

  auto &atl = track->get_automation_tracklist ();
  auto &at = atl.ats_[at_index];
  for (auto &r : at->regions_)
    {
      r->select (true, true, false);
    }
}

/**
 * Used as a workaround for GTK bug 4422.
 */
DEFINE_SIMPLE (activate_app_action_wrapper)
{
  const char * action_name = g_action_get_name (G_ACTION (action));
  zrythm_app->activate_action (action_name, Glib::wrap (variant));
}
