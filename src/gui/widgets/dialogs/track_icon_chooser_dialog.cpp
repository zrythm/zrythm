// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/region.h"
#include "dsp/track.h"
#include "gui/widgets/dialogs/track_icon_chooser_dialog.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

enum
{
  COL_LABEL,
  COL_PIXBUF,
};

/**
 * Runs the widget and processes the result, then
 * closes the dialog.
 *
 * @return Whether the color was set or not.
 */
bool
track_icon_chooser_dialog_widget_run (TrackIconChooserDialogWidget * self)
{
  int  res = z_gtk_dialog_run (GTK_DIALOG (self->dialog), false);
  bool icon_set = false;
  switch (res)
    {
    case GTK_RESPONSE_ACCEPT:
      icon_set = true;
      break;
    default:
      break;
    }

  if (icon_set && self->selected_icon)
    {
      /* if changed, apply the change */
      if (std::string (self->selected_icon) != self->track->icon_name_)
        {
          self->track->set_icon (
            self->selected_icon, F_UNDOABLE, F_PUBLISH_EVENTS);
        }
    }
  gtk_window_destroy (GTK_WINDOW (self->dialog));

  g_free_and_null (self->selected_icon);

  return icon_set;
}

static void
on_item_activated (
  GtkIconView *                  icon_view,
  GtkTreePath *                  path,
  TrackIconChooserDialogWidget * self)
{
  if (self->selected_icon)
    {
      g_free (self->selected_icon);
      self->selected_icon = NULL;
    }

  GtkTreeIter iter;
  gtk_tree_model_get_iter (self->icon_model, &iter, path);
  gtk_tree_model_get (
    GTK_TREE_MODEL (self->icon_model), &iter, COL_LABEL, &self->selected_icon,
    -1);
}

static GtkListStore *
create_list_store (void)
{
  GtkListStore * store = gtk_list_store_new (2, G_TYPE_STRING, GDK_TYPE_PIXBUF);

  GtkIconTheme * icon_theme = z_gtk_icon_theme_get_default ();

  GtkTreeIter iter;
  char **     list = gtk_icon_theme_get_icon_names (icon_theme);
  char *      icon_name;
  for (int i = 0; (icon_name = list[i]) != NULL; i++)
    {
      int                size = 16;
      GtkIconPaintable * paintable = gtk_icon_theme_lookup_icon (
        icon_theme, icon_name, nullptr, size, 1, GTK_TEXT_DIR_NONE,
        GTK_ICON_LOOKUP_FORCE_SYMBOLIC);
      GdkPixbuf * pixbuf = NULL;
      bool        is_track_type_icon = false;
      if (paintable)
        {
          GFile * file = gtk_icon_paintable_get_file (paintable);
          if (file)
            {
              char * path = g_file_get_path (file);
              /* FIXME GTK4 doesn't have API to return icons of
               * a given context and it also doesn't return
               * icons in non-standard subdirectories like
               * "tracktypes" - for now, just show all 'zrythm'
               * icons */
              if (
                path && string_contains_substr_case_insensitive (path, "zrythm"))
                {
                  g_debug ("found track type icon path: %s", path);
                  is_track_type_icon = true;
                  GError * err = NULL;
                  pixbuf = gdk_pixbuf_new_from_file_at_scale (
                    path, size, size, true, &err);
                  if (!pixbuf)
                    {
                      g_warning ("failed to get pixbuf: %s", err->message);
                      g_error_free (err);
                    }
                  g_free (path);
                }
              g_object_unref (file);
            }
          g_object_unref (paintable);
        }

      if (pixbuf)
        {
          gtk_list_store_append (store, &iter);
          gtk_list_store_set (
            store, &iter, COL_LABEL, icon_name, COL_PIXBUF, pixbuf, -1);
        }
      else if (is_track_type_icon)
        {
          g_message ("no pixbuf loaded for %s", icon_name);
        }
    }
  g_strfreev (list);

  return store;
}

/**
 * Creates a new dialog.
 */
TrackIconChooserDialogWidget *
track_icon_chooser_dialog_widget_new (Track * track)
{
  g_return_val_if_fail (IS_TRACK (track), nullptr);

  auto                           str = format_str (_ ("%s icon"), track->name_);
  TrackIconChooserDialogWidget * self =
    object_new (TrackIconChooserDialogWidget);
  self->dialog = GTK_DIALOG (gtk_dialog_new_with_buttons (
    str.c_str (), GTK_WINDOW (MAIN_WINDOW),
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, _ ("_Cancel"),
    GTK_RESPONSE_REJECT, _ ("_Select"), GTK_RESPONSE_ACCEPT, nullptr));
  gtk_widget_add_css_class (
    GTK_WIDGET (self->dialog), "track-icon-chooser-dialog");

  /* add icon view */
  self->icon_model = GTK_TREE_MODEL (create_list_store ());
  self->icon_view =
    GTK_ICON_VIEW (gtk_icon_view_new_with_model (self->icon_model));
  gtk_widget_set_visible (GTK_WIDGET (self->icon_view), true);
  gtk_icon_view_set_text_column (self->icon_view, COL_LABEL);
  gtk_icon_view_set_pixbuf_column (self->icon_view, COL_PIXBUF);
  gtk_icon_view_set_activate_on_single_click (self->icon_view, true);
  g_signal_connect (
    self->icon_view, "item-activated", G_CALLBACK (on_item_activated), self);
  GtkBox * content_area =
    GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self->dialog)));
  GtkScrolledWindow * scroll = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
  gtk_scrolled_window_set_min_content_width (scroll, 480);
  gtk_scrolled_window_set_min_content_height (scroll, 360);
  gtk_widget_set_visible (GTK_WIDGET (scroll), true);
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (scroll), GTK_WIDGET (self->icon_view));
  gtk_box_append (content_area, GTK_WIDGET (scroll));

  self->track = track;

  return self;
}
