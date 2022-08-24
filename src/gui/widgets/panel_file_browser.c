// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * File browser.
 */

#include "actions/tracklist_selections.h"
#include "gui/backend/file_manager.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/file_auditioner_controls.h"
#include "gui/widgets/file_browser_filters.h"
#include "gui/widgets/item_factory.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/panel_file_browser.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/tracklist.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  PanelFileBrowserWidget,
  panel_file_browser_widget,
  GTK_TYPE_WIDGET)

enum
{
  COLUMN_ICON,
  COLUMN_NAME,
  COLUMN_DESCR,
  NUM_COLUMNS
};

static GtkTreeModel *
create_model_for_locations (PanelFileBrowserWidget * self)
{
  GtkListStore * list_store;
  /*GtkTreePath *path;*/
  GtkTreeIter iter;

  /* icon, file name, index */
  list_store = gtk_list_store_new (
    3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

  for (guint i = 0; i < FILE_MANAGER->locations->len; i++)
    {
      FileBrowserLocation * loc =
        g_ptr_array_index (FILE_MANAGER->locations, i);

      char icon_name[600];
      switch (loc->special_location)
        {
        case FILE_MANAGER_NONE:
          strcpy (icon_name, "folder");
          break;
        case FILE_MANAGER_HOME:
          strcpy (icon_name, "user-home");
          break;
        case FILE_MANAGER_DESKTOP:
          strcpy (icon_name, "desktop");
          break;
        }

      /* Add a new row to the model */
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter, 0, icon_name, 1, loc->label, 2,
        loc, -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static void
show_bookmarks_context_menu (
  PanelFileBrowserWidget *    self,
  const FileBrowserLocation * loc,
  double                      x,
  double                      y)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  self->cur_loc = loc;

  menuitem = z_gtk_create_menu_item (
    _ ("Delete"), "edit-delete",
    "app.panel-file-browser-delete-bookmark");
  g_menu_append_item (menu, menuitem);

  z_gtk_show_context_menu_from_g_menu (
    self->popover_menu, x, y, menu);
}

static void
on_bookmark_right_click (
  GtkGestureClick *        gesture,
  gint                     n_press,
  gdouble                  x_dbl,
  gdouble                  y_dbl,
  PanelFileBrowserWidget * self)
{
  if (n_press != 1)
    return;

  GtkTreePath *       path;
  GtkTreeViewColumn * column;

  int x, y;
  gtk_tree_view_convert_widget_to_bin_window_coords (
    GTK_TREE_VIEW (self->bookmarks_tree_view), (int) x_dbl,
    (int) y_dbl, &x, &y);

  GtkTreeSelection * selection =
    gtk_tree_view_get_selection ((self->bookmarks_tree_view));
  if (!gtk_tree_view_get_path_at_pos (
        GTK_TREE_VIEW (self->bookmarks_tree_view), x, y,
        &path, &column, NULL, NULL))
    {
      g_message ("no path at position %d %d", x, y);
      // if we can't find path at pos, we surely don't
      // want to pop up the menu
      return;
    }

  gtk_tree_selection_unselect_all (selection);
  gtk_tree_selection_select_path (selection, path);
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    GTK_TREE_MODEL (self->bookmarks_tree_model), &iter, path);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    GTK_TREE_MODEL (self->bookmarks_tree_model), &iter,
    COLUMN_DESCR, &value);
  gtk_tree_path_free (path);

  FileBrowserLocation * loc = g_value_get_pointer (&value);

  show_bookmarks_context_menu (self, loc, x, y);
}

void
panel_file_browser_refresh_bookmarks (
  PanelFileBrowserWidget * self)
{
  self->bookmarks_tree_model =
    GTK_TREE_MODEL (create_model_for_locations (self));
  gtk_tree_view_set_model (
    self->bookmarks_tree_view,
    GTK_TREE_MODEL (self->bookmarks_tree_model));
}

/**
 * Visible function for file tree model.
 */
static gboolean
files_filter_func (GObject * item, gpointer user_data)
{
  PanelFileBrowserWidget * self =
    Z_PANEL_FILE_BROWSER_WIDGET (user_data);
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (item);
  SupportedFile * descr = (SupportedFile *) wrapped_obj->obj;

  bool show_audio = gtk_toggle_button_get_active (
    self->filters_toolbar->toggle_audio);
  bool show_midi = gtk_toggle_button_get_active (
    self->filters_toolbar->toggle_midi);
  bool show_presets = gtk_toggle_button_get_active (
    self->filters_toolbar->toggle_presets);
  bool all_toggles_off =
    !show_audio && !show_midi && !show_presets;

  /* filter by name */
  const char * text = gtk_editable_get_text (
    GTK_EDITABLE (self->file_search_entry));
  if (
    text && strlen (text) > 0
    && !string_contains_substr_case_insensitive (
      descr->label, text))
    {
      return false;
    }

  bool visible = false;
  switch (descr->type)
    {
    case FILE_TYPE_MIDI:
      visible = show_midi || all_toggles_off;
      break;
    case FILE_TYPE_DIR:
    case FILE_TYPE_PARENT_DIR:
      return true;
    case FILE_TYPE_MP3:
    case FILE_TYPE_FLAC:
    case FILE_TYPE_OGG:
    case FILE_TYPE_WAV:
      visible = show_audio || all_toggles_off;
      break;
    case FILE_TYPE_OTHER:
      visible =
        all_toggles_off
        && g_settings_get_boolean (
          S_UI_FILE_BROWSER, "show-unsupported-files");
      break;
    default:
      break;
    }

  if (!visible)
    return false;

  if (
    !g_settings_get_boolean (
      S_UI_FILE_BROWSER, "show-hidden-files")
    && descr->hidden)
    return false;

  return visible;
}

static int
update_file_info_label (
  PanelFileBrowserWidget * self,
  const char *             label)
{
  gtk_label_set_markup (
    self->file_info, label ? label : _ ("No file selected"));

  return G_SOURCE_REMOVE;
}

static WrappedObjectWithChangeSignal *
get_selected_file (GtkWidget * widget)
{
  PanelFileBrowserWidget * self =
    Z_PANEL_FILE_BROWSER_WIDGET (widget);
  GObject * gobj = gtk_single_selection_get_selected_item (
    self->files_selection_model);
  if (!gobj)
    return NULL;

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);

  return wrapped_obj;
}

static GListModel *
create_model_for_files (PanelFileBrowserWidget * self)
{
  /* file name, index */
  GListStore * store =
    g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);

  for (size_t i = 0; i < FILE_MANAGER->files->len; i++)
    {
      SupportedFile * descr = (SupportedFile *)
        g_ptr_array_index (FILE_MANAGER->files, i);

      WrappedObjectWithChangeSignal * wrapped_descr =
        wrapped_object_with_change_signal_new (
          descr, WRAPPED_OBJECT_TYPE_SUPPORTED_FILE);

      g_list_store_append (store, wrapped_descr);
    }

  self->files_filter = gtk_custom_filter_new (
    (GtkCustomFilterFunc) files_filter_func, self, NULL);
  self->files_filter_model = gtk_filter_list_model_new (
    G_LIST_MODEL (store), GTK_FILTER (self->files_filter));
  self->files_selection_model = gtk_single_selection_new (
    G_LIST_MODEL (self->files_filter_model));

  return G_LIST_MODEL (self->files_selection_model);
}

static void
on_files_selection_changed (
  GtkSelectionModel * selection_model,
  guint               position,
  guint               n_items,
  gpointer            user_data)
{
  PanelFileBrowserWidget * self =
    Z_PANEL_FILE_BROWSER_WIDGET (user_data);

  sample_processor_stop_file_playback (SAMPLE_PROCESSOR);

  GObject * gobj = gtk_single_selection_get_selected_item (
    GTK_SINGLE_SELECTION (self->files_selection_model));
  if (!gobj)
    {
      self->cur_file = NULL;
      return;
    }

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  SupportedFile * descr = (SupportedFile *) wrapped_obj->obj;
  self->cur_file = descr;

  g_ptr_array_remove_range (
    self->selected_files, 0, self->selected_files->len);
  g_ptr_array_add (self->selected_files, descr);

  /* return if file does not exist */
  if (!g_file_test (descr->abs_path, G_FILE_TEST_EXISTS))
    return;

  char * label =
    supported_file_get_info_text_for_label (descr);
  g_message ("selected file: %s", descr->abs_path);
  update_file_info_label (self, label);

  if (
    g_settings_get_boolean (S_UI_FILE_BROWSER, "autoplay")
    && supported_file_should_autoplay (descr))
    {
      sample_processor_queue_file (SAMPLE_PROCESSOR, descr);
    }
}

static void
files_list_view_setup (
  PanelFileBrowserWidget * self,
  GtkListView *            list_view,
  GtkSelectionModel *      selection_model)
{
  gtk_list_view_set_model (list_view, selection_model);
  ItemFactory * prev_factory = self->files_item_factory;
  self->files_item_factory = item_factory_new (
    ITEM_FACTORY_ICON_AND_TEXT, false, NULL);
  gtk_list_view_set_factory (
    list_view, self->files_item_factory->list_item_factory);

  g_signal_connect (
    G_OBJECT (selection_model), "selection-changed",
    G_CALLBACK (on_files_selection_changed), self);

  if (prev_factory)
    {
      item_factory_free (prev_factory);
    }
}

static void
on_bookmark_row_activated (
  GtkTreeView *            tree_view,
  GtkTreePath *            tp,
  GtkTreeViewColumn *      column,
  PanelFileBrowserWidget * self)
{
  GtkTreeModel * model =
    GTK_TREE_MODEL (gtk_tree_view_get_model (tree_view));
  GtkTreeIter iter;
  gtk_tree_model_get_iter (model, &iter, tp);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (model, &iter, 2, &value);
  FileBrowserLocation * loc = g_value_get_pointer (&value);

  file_manager_set_selection (FILE_MANAGER, loc, true, true);
  self->files_selection_model =
    GTK_SINGLE_SELECTION (create_model_for_files (self));
  files_list_view_setup (
    self, self->files_list_view,
    GTK_SELECTION_MODEL (self->files_selection_model));
}

static void
on_file_search_changed (
  GtkSearchEntry *         search_entry,
  PanelFileBrowserWidget * self)
{
  gtk_filter_changed (
    GTK_FILTER (self->files_filter),
    GTK_FILTER_CHANGE_DIFFERENT);
}

static void
on_file_row_activated (
  GtkListView * list_view,
  guint         position,
  gpointer      user_data)
{
  PanelFileBrowserWidget * self =
    Z_PANEL_FILE_BROWSER_WIDGET (user_data);

  GObject * gobj = g_list_model_get_object (
    G_LIST_MODEL (self->files_selection_model), position);
  if (!gobj)
    return;

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  SupportedFile * descr = (SupportedFile *) wrapped_obj->obj;

  if (
    descr->type == FILE_TYPE_DIR
    || descr->type == FILE_TYPE_PARENT_DIR)
    {
      /* FIXME free unnecessary stuff */
      FileBrowserLocation * loc =
        object_new (FileBrowserLocation);
      loc->path = descr->abs_path;
      loc->label = g_path_get_basename (loc->path);
      file_manager_set_selection (
        FILE_MANAGER, loc, true, true);
      self->files_selection_model =
        GTK_SINGLE_SELECTION (create_model_for_files (self));
      files_list_view_setup (
        self, self->files_list_view,
        GTK_SELECTION_MODEL (self->files_selection_model));
    }
  else if (
    descr->type == FILE_TYPE_WAV || descr->type == FILE_TYPE_OGG
    || descr->type == FILE_TYPE_FLAC
    || descr->type == FILE_TYPE_MP3)
    {
      GError * err = NULL;
      bool     ret = track_create_with_action (
            TRACK_TYPE_AUDIO, NULL, descr, PLAYHEAD,
            TRACKLIST->num_tracks, 1, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Failed to create track"));
        }
    }
  else if (descr->type == FILE_TYPE_MIDI)
    {
      GError * err = NULL;
      bool     ret = track_create_with_action (
            TRACK_TYPE_MIDI, NULL, descr, PLAYHEAD,
            TRACKLIST->num_tracks, 1, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Failed to create track"));
        }
    }
}

static void
tree_view_setup (
  PanelFileBrowserWidget * self,
  GtkTreeView *            tree_view,
  GtkTreeModel *           model,
  bool                     allow_multi,
  bool                     dnd)
{
  /* instantiate tree view using model */
  gtk_tree_view_set_model (tree_view, GTK_TREE_MODEL (model));

  /* init tree view */
  GtkCellRenderer *   renderer;
  GtkTreeViewColumn * column;
  if (GTK_TREE_MODEL (self->bookmarks_tree_model) == model)
    {
      /* column for icon */
      renderer = gtk_cell_renderer_pixbuf_new ();
      column = gtk_tree_view_column_new_with_attributes (
        "icon", renderer, "icon-name", COLUMN_ICON, NULL);
      gtk_tree_view_append_column (
        GTK_TREE_VIEW (tree_view), column);

      /* column for name */
      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes (
        "name", renderer, "text", COLUMN_NAME, NULL);
      gtk_tree_view_append_column (
        GTK_TREE_VIEW (tree_view), column);

      /* set search column */
      gtk_tree_view_set_search_column (
        GTK_TREE_VIEW (tree_view), COLUMN_NAME);
    }
  else
    {
      /* column for name */
      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes (
        "name", renderer, "text", 0, NULL);
      gtk_tree_view_append_column (
        GTK_TREE_VIEW (tree_view), column);
    }

  /* hide headers and allow multi-selection */
  gtk_tree_view_set_headers_visible (
    GTK_TREE_VIEW (tree_view), false);

  if (allow_multi)
    {
      gtk_tree_selection_set_mode (
        gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view)),
        GTK_SELECTION_MULTIPLE);
    }
}

static void
on_position_change (
  GtkStack *               stack,
  GParamSpec *             pspec,
  PanelFileBrowserWidget * self)
{
  if (self->first_draw)
    {
      return;
    }

  int divider_pos = gtk_paned_get_position (self->paned);
  g_settings_set_int (
    S_UI, "browser-divider-position", divider_pos);
  g_message (
    "set browser divider position to %d", divider_pos);
  /*g_warning ("pos %d", divider_pos);*/
}

static void
on_map (GtkWidget * widget, PanelFileBrowserWidget * self)
{
  if (self->first_draw)
    {
      self->first_draw = false;

      /* set divider position */
      int divider_pos =
        g_settings_get_int (S_UI, "browser-divider-position");
      /*g_warning ("pos %d", divider_pos);*/
      gtk_paned_set_position (self->paned, divider_pos);
    }
}

static void
refilter_files (PanelFileBrowserWidget * self)
{
  gtk_filter_changed (
    GTK_FILTER (self->files_filter),
    GTK_FILTER_CHANGE_DIFFERENT);
}

PanelFileBrowserWidget *
panel_file_browser_widget_new ()
{
  PanelFileBrowserWidget * self =
    g_object_new (PANEL_FILE_BROWSER_WIDGET_TYPE, NULL);

  g_message ("Instantiating panel_file_browser widget...");

  self->first_draw = true;

  gtk_label_set_xalign (self->file_info, 0);

  file_auditioner_controls_widget_setup (
    self->auditioner_controls, GTK_WIDGET (self), true,
    get_selected_file, (GenericCallback) refilter_files);
  file_browser_filters_widget_setup (
    self->filters_toolbar, GTK_WIDGET (self),
    (GenericCallback) refilter_files);

  /* populate bookmarks */
  self->bookmarks_tree_model =
    GTK_TREE_MODEL (create_model_for_locations (self));
  tree_view_setup (
    self, self->bookmarks_tree_view,
    GTK_TREE_MODEL (self->bookmarks_tree_model), false, true);
  g_signal_connect (
    self->bookmarks_tree_view, "row-activated",
    G_CALLBACK (on_bookmark_row_activated), self);

  /* populate files */
  self->files_selection_model =
    GTK_SINGLE_SELECTION (create_model_for_files (self));
  files_list_view_setup (
    self, self->files_list_view,
    GTK_SELECTION_MODEL (self->files_selection_model));
  g_signal_connect (
    self->files_list_view, "activate",
    G_CALLBACK (on_file_row_activated), self);

  /* setup file search entry */
  gtk_search_entry_set_key_capture_widget (
    self->file_search_entry,
    GTK_WIDGET (self->files_list_view));
  g_object_set (
    G_OBJECT (self->file_search_entry), "placeholder-text",
    _ ("Search..."), NULL);
  g_signal_connect (
    G_OBJECT (self->file_search_entry), "search-changed",
    G_CALLBACK (on_file_search_changed), self);

  g_signal_connect (
    G_OBJECT (self), "map", G_CALLBACK (on_map), self);
  g_signal_connect (
    G_OBJECT (self), "notify::position",
    G_CALLBACK (on_position_change), self);

  /* connect right click handlers */
  GtkGestureClick * mp =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (on_bookmark_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->bookmarks_tree_view),
    GTK_EVENT_CONTROLLER (mp));

  return self;
}
static void
dispose (PanelFileBrowserWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));

  G_OBJECT_CLASS (panel_file_browser_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
finalize (PanelFileBrowserWidget * self)
{

  G_OBJECT_CLASS (panel_file_browser_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
panel_file_browser_widget_class_init (
  PanelFileBrowserWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "panel_file_browser.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, PanelFileBrowserWidget, x)

  BIND_CHILD (paned);
  BIND_CHILD (browser_top);
  BIND_CHILD (browser_bot);
  BIND_CHILD (file_info);
  BIND_CHILD (file_search_entry);
  BIND_CHILD (files_list_view);
  BIND_CHILD (filters_toolbar);
  BIND_CHILD (bookmarks_tree_view);
  BIND_CHILD (auditioner_controls);

#undef BIND_CHILD

  gtk_widget_class_set_layout_manager_type (
    klass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * goklass = G_OBJECT_CLASS (_klass);
  goklass->dispose = (GObjectFinalizeFunc) dispose;
  goklass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
panel_file_browser_widget_init (PanelFileBrowserWidget * self)
{
  g_type_ensure (FILE_AUDITIONER_CONTROLS_WIDGET_TYPE);
  g_type_ensure (FILE_BROWSER_FILTERS_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (NULL));
  gtk_widget_set_parent (
    GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  gtk_widget_set_hexpand (GTK_WIDGET (self->paned), true);

  self->selected_files = g_ptr_array_new ();

  gtk_widget_add_css_class (GTK_WIDGET (self), "file-browser");
}
