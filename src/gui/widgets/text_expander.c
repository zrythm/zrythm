// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "gui/widgets/main_window.h"
#include "gui/widgets/text_expander.h"
#include "project.h"
#include "utils/gtk.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#define TEXT_EXPANDER_WIDGET_TYPE (text_expander_widget_get_type ())
G_DEFINE_TYPE (TextExpanderWidget, text_expander_widget, EXPANDER_BOX_WIDGET_TYPE)

static void
on_focus_enter (GtkEventControllerFocus * focus_controller, gpointer user_data)
{
  TextExpanderWidget * self = Z_TEXT_EXPANDER_WIDGET (user_data);

  g_message ("text focused");
  self->has_focus = true;
}

static void
on_focus_leave (GtkEventControllerFocus * focus_controller, gpointer user_data)
{
  TextExpanderWidget * self = Z_TEXT_EXPANDER_WIDGET (user_data);

  g_message ("text focus out");
  self->has_focus = false;

  if (self->setter && self->obj)
    {
      GtkTextIter start_iter, end_iter;
      gtk_text_buffer_get_start_iter (
        GTK_TEXT_BUFFER (self->buffer), &start_iter);
      gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (self->buffer), &end_iter);
      char * content = gtk_text_buffer_get_text (
        GTK_TEXT_BUFFER (self->buffer), &start_iter, &end_iter, false);
      self->setter (self->obj, content);
      text_expander_widget_refresh (self);
    }
}

/**
 * Refreshes the text.
 */
void
text_expander_widget_refresh (TextExpanderWidget * self)
{
  if (self->getter && self->obj)
    {
      g_return_if_fail (self->buffer);
      gtk_text_buffer_set_text (
        GTK_TEXT_BUFFER (self->buffer), self->getter (self->obj), -1);
      gtk_label_set_text (self->label, self->getter (self->obj));
    }
}

/**
 * Sets up the TextExpanderWidget.
 */
void
text_expander_widget_setup (
  TextExpanderWidget * self,
  bool                 wrap_text,
  GenericStringGetter  getter,
  GenericStringSetter  setter,
  void *               obj)
{
  self->getter = getter;
  self->setter = setter;
  self->obj = obj;

  gtk_label_set_wrap (self->label, wrap_text);
  if (wrap_text)
    gtk_label_set_wrap_mode (self->label, PANGO_WRAP_WORD_CHAR);

  text_expander_widget_refresh (self);
}

static void
text_expander_widget_class_init (TextExpanderWidgetClass * klass)
{
}

static void
text_expander_widget_init (TextExpanderWidget * self)
{
  GtkWidget * box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  self->scroll = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
  gtk_widget_set_vexpand (GTK_WIDGET (self->scroll), 1);
  gtk_widget_set_visible (GTK_WIDGET (self->scroll), 1);
  gtk_widget_set_size_request (GTK_WIDGET (self->scroll), -1, 124);
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->scroll));

  self->viewport = GTK_VIEWPORT (gtk_viewport_new (NULL, NULL));
  gtk_widget_set_visible (GTK_WIDGET (self->viewport), 1);
  gtk_scrolled_window_set_child (self->scroll, GTK_WIDGET (self->viewport));

  self->label = GTK_LABEL (gtk_label_new (""));
  gtk_viewport_set_child (self->viewport, GTK_WIDGET (self->label));
  gtk_widget_set_vexpand (GTK_WIDGET (self->label), true);

  self->edit_btn = GTK_MENU_BUTTON (gtk_menu_button_new ());
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->edit_btn));
  gtk_menu_button_set_icon_name (self->edit_btn, "pencil");

  self->popover = GTK_POPOVER (gtk_popover_new ());
  gtk_menu_button_set_popover (
    GTK_MENU_BUTTON (self->edit_btn), GTK_WIDGET (self->popover));

  GtkSourceLanguageManager * manager = z_gtk_source_language_manager_get ();
  g_return_if_fail (manager);
  GtkSourceLanguage * lang =
    gtk_source_language_manager_get_language (manager, "markdown");
  g_return_if_fail (lang);
  self->buffer = gtk_source_buffer_new_with_language (lang);
  g_return_if_fail (self->buffer);
  self->editor =
    GTK_SOURCE_VIEW (gtk_source_view_new_with_buffer (self->buffer));
  gtk_popover_set_child (self->popover, GTK_WIDGET (self->editor));
  gtk_widget_set_visible (GTK_WIDGET (self->editor), true);
  gtk_source_view_set_tab_width (self->editor, 2);
  gtk_source_view_set_indent_width (self->editor, 2);
  gtk_source_view_set_insert_spaces_instead_of_tabs (self->editor, true);
  gtk_source_view_set_smart_backspace (self->editor, true);

  /* set style */
  GtkSourceStyleSchemeManager * style_mgr =
    gtk_source_style_scheme_manager_get_default ();
  gtk_source_style_scheme_manager_prepend_search_path (
    style_mgr, CONFIGURE_SOURCEVIEW_STYLES_DIR);
  gtk_source_style_scheme_manager_force_rescan (style_mgr);
  GtkSourceStyleScheme * scheme = gtk_source_style_scheme_manager_get_scheme (
    style_mgr, "monokai-extended-zrythm");
  gtk_source_buffer_set_style_scheme (self->buffer, scheme);

  expander_box_widget_add_content (
    Z_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (box));

  expander_box_widget_set_icon_name (
    Z_EXPANDER_BOX_WIDGET (self), "text-bubble");

  GtkEventControllerFocus * focus_controller =
    GTK_EVENT_CONTROLLER_FOCUS (gtk_event_controller_focus_new ());
  g_signal_connect (
    G_OBJECT (focus_controller), "leave", G_CALLBACK (on_focus_leave), self);
  g_signal_connect (
    G_OBJECT (focus_controller), "enter", G_CALLBACK (on_focus_enter), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->editor), GTK_EVENT_CONTROLLER (focus_controller));
}
