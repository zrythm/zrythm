// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#ifdef HAVE_GUILE

#  include "gui/widgets/dialogs/scripting_dialog.h"
#  include "project.h"
#  include "settings/settings.h"
#  include "utils/gtk.h"
#  include "utils/io.h"
#  include "utils/resources.h"
#  include "utils/ui.h"

#  include <glib/gi18n.h>
#  include <gtk/gtk.h>

#  include "guile/guile.h"

G_DEFINE_TYPE (
  ScriptingDialogWidget,
  scripting_dialog_widget,
  GTK_TYPE_DIALOG)

static void
on_execute_clicked (
  GtkButton *             spin,
  ScriptingDialogWidget * self)
{
  GtkTextIter start_iter, end_iter;
  gtk_text_buffer_get_start_iter (
    GTK_TEXT_BUFFER (self->buffer), &start_iter);
  gtk_text_buffer_get_end_iter (
    GTK_TEXT_BUFFER (self->buffer), &end_iter);
  char * script_content = gtk_text_buffer_get_text (
    GTK_TEXT_BUFFER (self->buffer), &start_iter,
    &end_iter, false);

  GuileScriptLanguage lang = (GuileScriptLanguage)
    adw_combo_row_get_selected (
      self->lang_select_combo_row);

  char * markup =
    guile_run_script (script_content, lang);
  g_free (script_content);

  gtk_label_set_markup (self->output, markup);
  g_free (markup);
}

static void
on_lang_selection_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  void *       data)
{
  ScriptingDialogWidget * self =
    Z_SCRIPTING_DIALOG_WIDGET (data);
  AdwComboRow * combo_row = ADW_COMBO_ROW (gobject);

  const char *        source_lang_str = NULL;
  GuileScriptLanguage script_lang =
    (GuileScriptLanguage)
      adw_combo_row_get_selected (combo_row);
  switch (script_lang)
    {
    case GUILE_SCRIPT_LANGUAGE_SCHEME:
      source_lang_str = "scheme";
      break;
    case GUILE_SCRIPT_LANGUAGE_ECMASCRIPT:
      source_lang_str = "js";
      break;
    case GUILE_SCRIPT_LANGUAGE_ELISP:
      source_lang_str = "commonlisp";
      break;
    default:
      g_return_if_reached ();
    }

  GtkSourceLanguageManager * manager =
    z_gtk_source_language_manager_get ();
  GtkSourceLanguage * lang =
    gtk_source_language_manager_get_language (
      manager, source_lang_str);
  g_return_if_fail (lang);
  gtk_source_buffer_set_language (
    self->buffer, lang);
}

/**
 * Creates a bounce dialog.
 */
ScriptingDialogWidget *
scripting_dialog_widget_new ()
{
  ScriptingDialogWidget * self = g_object_new (
    SCRIPTING_DIALOG_WIDGET_TYPE, "title",
    _ ("Scripting Interface"), "icon-name",
    "zrythm", NULL);

  return self;
}

static void
setup_lang_selection (ScriptingDialogWidget * self)
{
  adw_preferences_row_set_title (
    ADW_PREFERENCES_ROW (self->lang_select_combo_row),
    _ ("Language"));
  adw_action_row_set_subtitle (
    ADW_ACTION_ROW (self->lang_select_combo_row),
    _ ("Select the language of the script"));

  GtkStringList * string_list =
    gtk_string_list_new (NULL);
  for (int i = 0; i < NUM_GUILE_SCRIPT_LANGUAGES; i++)
    {
      gtk_string_list_append (
        string_list,
        guile_get_script_language_str (
          (GuileScriptLanguage) i));
    }
  adw_combo_row_set_model (
    self->lang_select_combo_row,
    G_LIST_MODEL (string_list));

  g_signal_connect (
    self->lang_select_combo_row, "notify::selected",
    G_CALLBACK (on_lang_selection_changed), self);
}

static void
scripting_dialog_widget_class_init (
  ScriptingDialogWidgetClass * _klass)
{
}

static void
scripting_dialog_widget_init (
  ScriptingDialogWidget * self)
{
  gtk_window_set_default_size (
    GTK_WINDOW (self), 784, 578);

  GtkWidget * content_area =
    gtk_dialog_get_content_area (GTK_DIALOG (self));

  GtkPaned * paned = GTK_PANED (
    gtk_paned_new (GTK_ORIENTATION_VERTICAL));
  gtk_paned_set_shrink_start_child (paned, false);
  gtk_paned_set_shrink_end_child (paned, false);
  gtk_widget_set_hexpand (GTK_WIDGET (paned), true);
  gtk_widget_set_vexpand (GTK_WIDGET (paned), true);
  gtk_box_append (
    GTK_BOX (content_area), GTK_WIDGET (paned));

  GtkBox * top_box = GTK_BOX (
    gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_paned_set_start_child (
    paned, GTK_WIDGET (top_box));
  GtkScrolledWindow * scroll = GTK_SCROLLED_WINDOW (
    gtk_scrolled_window_new ());
  gtk_box_append (top_box, GTK_WIDGET (scroll));
  gtk_widget_set_size_request (
    GTK_WIDGET (scroll), 596, 320);
  gtk_widget_set_hexpand (GTK_WIDGET (scroll), true);
  gtk_widget_set_vexpand (GTK_WIDGET (scroll), true);
  self->source_viewport =
    GTK_VIEWPORT (gtk_viewport_new (NULL, NULL));
  gtk_scrolled_window_set_child (
    scroll, GTK_WIDGET (self->source_viewport));

  char * filename = g_settings_get_string (
    S_P_SCRIPTING_GENERAL, "default-script");
  char * dir =
    zrythm_get_dir (ZRYTHM_DIR_USER_SCRIPTS);
  char * full_path =
    g_build_filename (dir, filename, NULL);
  if (!g_file_test (full_path, G_FILE_TEST_EXISTS))
    {
      g_free (dir);
      g_free (full_path);
      dir = zrythm_get_dir (
        ZRYTHM_DIR_SYSTEM_SCRIPTSDIR);
      full_path =
        g_build_filename (dir, filename, NULL);

      if (!g_file_test (
            full_path, G_FILE_TEST_EXISTS))
        {
          g_free (full_path);
          full_path = g_build_filename (
            dir, "print-all-tracks", NULL);

          if (!g_file_test (
                full_path, G_FILE_TEST_EXISTS))
            {
              g_critical (
                "File not found: %s", full_path);
              return;
            }
        }
    }

  char *   script_contents = NULL;
  GError * err = NULL;
  bool     ret = g_file_get_contents (
        full_path, &script_contents, NULL, &err);
  if (!ret)
    {
      g_critical (
        "error reading <%s> contents: %s",
        full_path, err->message);
      return;
    }

  g_free (full_path);
  g_free (dir);
  g_free (filename);

  GtkSourceLanguageManager * manager =
    z_gtk_source_language_manager_get ();
  GtkSourceLanguage * lang =
    gtk_source_language_manager_get_language (
      manager, "scheme");
  g_return_if_fail (lang);
  self->buffer =
    gtk_source_buffer_new_with_language (lang);
  self->editor = GTK_SOURCE_VIEW (
    gtk_source_view_new_with_buffer (self->buffer));
  gtk_viewport_set_child (
    GTK_VIEWPORT (self->source_viewport),
    GTK_WIDGET (self->editor));
  gtk_text_buffer_set_text (
    GTK_TEXT_BUFFER (self->buffer), script_contents,
    -1);
  gtk_source_view_set_show_line_numbers (
    self->editor, true);
  gtk_source_view_set_auto_indent (
    self->editor, true);
  gtk_source_view_set_indent_on_tab (
    self->editor, true);
  gtk_source_view_set_tab_width (self->editor, 2);
  gtk_source_view_set_indent_width (self->editor, 2);
  gtk_source_view_set_insert_spaces_instead_of_tabs (
    self->editor, true);
  gtk_source_view_set_smart_backspace (
    self->editor, true);

  g_free (script_contents);

  /* set style */
  GtkSourceStyleSchemeManager * style_mgr =
    gtk_source_style_scheme_manager_get_default ();
  gtk_source_style_scheme_manager_prepend_search_path (
    style_mgr, CONFIGURE_SOURCEVIEW_STYLES_DIR);
  gtk_source_style_scheme_manager_force_rescan (
    style_mgr);
  GtkSourceStyleScheme * scheme =
    gtk_source_style_scheme_manager_get_scheme (
      style_mgr, "monokai-extended-zrythm");
  gtk_source_buffer_set_style_scheme (
    self->buffer, scheme);

  /* setup language selection */
  GtkListBox * list_box =
    GTK_LIST_BOX (gtk_list_box_new ());
  gtk_box_append (top_box, GTK_WIDGET (list_box));
  gtk_list_box_set_selection_mode (
    list_box, GTK_SELECTION_NONE);
  self->lang_select_combo_row =
    ADW_COMBO_ROW (adw_combo_row_new ());
  gtk_list_box_append (
    list_box,
    GTK_WIDGET (self->lang_select_combo_row));
  setup_lang_selection (self);

  self->execute_btn = GTK_BUTTON (
    gtk_button_new_with_label (_ ("Execute")));
  gtk_widget_set_receives_default (
    GTK_WIDGET (self->execute_btn), true);
  gtk_widget_set_focusable (
    GTK_WIDGET (self->execute_btn), true);
  gtk_box_append (
    top_box, GTK_WIDGET (self->execute_btn));
  g_signal_connect (
    G_OBJECT (self->execute_btn), "clicked",
    G_CALLBACK (on_execute_clicked), self);

  GtkBox * bot_box = GTK_BOX (
    gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_paned_set_end_child (
    paned, GTK_WIDGET (bot_box));
  scroll = GTK_SCROLLED_WINDOW (
    gtk_scrolled_window_new ());
  gtk_box_append (bot_box, GTK_WIDGET (scroll));
  gtk_widget_set_size_request (
    GTK_WIDGET (scroll), -1, 105);
  gtk_widget_set_hexpand (GTK_WIDGET (scroll), true);
  gtk_widget_set_vexpand (GTK_WIDGET (scroll), true);
  self->output = GTK_LABEL (gtk_label_new (
    _ ("The result will be printed here")));
  gtk_label_set_selectable (self->output, true);
  gtk_scrolled_window_set_child (
    scroll, GTK_WIDGET (self->output));
}
#endif
