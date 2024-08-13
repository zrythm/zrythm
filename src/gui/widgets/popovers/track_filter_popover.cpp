// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/item_factory.h"
#include "gui/widgets/multi_selection.h"
#include "gui/widgets/popovers/track_filter_popover.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "utils/flags.h"
#include "utils/rt_thread_id.h"
#include "utils/string.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"
#include "libadwaita_wrapper.h"

G_DEFINE_TYPE (
  TrackFilterPopoverWidget,
  track_filter_popover_widget,
  GTK_TYPE_POPOVER)

static void
on_track_types_changed (
  MultiSelectionWidget * multi_selection,
  const GArray *         selection_indices,
  void *                 user_data)
{
  TrackFilterPopoverWidget * self = Z_TRACK_FILTER_POPOVER_WIDGET (user_data);

  z_debug (
    "selected filter track types changed: {} selected", selection_indices->len);

  GVariantBuilder builder;
  g_variant_builder_init (&builder, (const GVariantType *) "au");
  for (guint i = 0; i < selection_indices->len; i++)
    {
      guint idx = g_array_index (selection_indices, guint, i);
      g_variant_builder_add (&builder, "u", idx);
    }
  GVariant * variant = g_variant_builder_end (&builder);
  g_settings_set_value (S_UI, "track-filter-type", variant);
  g_variant_ref_sink (variant);
  g_variant_unref (variant);

  gtk_filter_changed (
    GTK_FILTER (self->custom_filter), GTK_FILTER_CHANGE_DIFFERENT);
}

static void
on_track_name_changed (GtkEditable * editable, TrackFilterPopoverWidget * self)
{
  const char * str = gtk_editable_get_text (editable);
  g_settings_set_string (S_UI, "track-filter-name", str);
  gtk_filter_changed (
    GTK_FILTER (self->custom_filter), GTK_FILTER_CHANGE_DIFFERENT);
}

static void
on_show_disabled_tracks_active_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  TrackFilterPopoverWidget * self = Z_TRACK_FILTER_POPOVER_WIDGET (user_data);
  bool active = gtk_switch_get_active (GTK_SWITCH (gobject));
  g_settings_set_boolean (S_UI, "track-filter-show-disabled", active);
  gtk_filter_changed (
    GTK_FILTER (self->custom_filter), GTK_FILTER_CHANGE_DIFFERENT);
}

static int
filter_func (void * gobj, void * user_data)
{
#if 0
  TrackFilterPopoverWidget * self =
    Z_TRACK_FILTER_POPOVER_WIDGET (user_data);
#endif

  char * name = g_settings_get_string (S_UI, "track-filter-name");

  WrappedObjectWithChangeSignal * wrapped_track =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  Track * track = (Track *) wrapped_track->obj;
  z_return_val_if_fail (IS_TRACK_AND_NONNULL (track), false);

  bool filtered =
    !string_contains_substr_case_insensitive (track->name_.c_str (), name);
  g_free (name);

  if (!filtered)
    {
      GVariant *    variant = g_settings_get_value (S_UI, "track-filter-type");
      gsize         n_elements;
      gconstpointer arr_ptr =
        g_variant_get_fixed_array (variant, &n_elements, sizeof (guint));
      const guint * elements = (const guint *) arr_ptr;
      bool          track_type_matched = n_elements == 0;
      for (gsize i = 0; i < n_elements; i++)
        {
          Track::Type cur_track_type =
            ENUM_INT_TO_VALUE (Track::Type, elements[i]);
          if (track->type_ == cur_track_type)
            {
              track_type_matched = true;
              break;
            }
        }
      g_variant_unref (variant);

      filtered = !track_type_matched;
    }

  if (!filtered)
    {
      if (
        !track->enabled_
        && !g_settings_get_boolean (S_UI, "track-filter-show-disabled"))
        {
          filtered = true;
        }
    }

  if (track->filtered_ == !filtered)
    {
      track->filtered_ = filtered;
      EVENTS_PUSH (EventType::ET_TRACK_VISIBILITY_CHANGED, track);
    }

  return !filtered;
}

static void
refresh_track_col_view_items (TrackFilterPopoverWidget * self)
{
  for (auto &track : TRACKLIST->tracks_)
    {
      WrappedObjectWithChangeSignal * wrapped_track =
        wrapped_object_with_change_signal_new (
          track.get (), WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK);

      g_list_store_append (self->track_list_store, wrapped_track);
    }
}

static void
setup_col_view (TrackFilterPopoverWidget * self, GtkColumnView * col_view)
{
  self->track_col_view = col_view;

  GListStore * store = g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
  self->track_list_store = store;

  /* make sortable */
  GtkSorter * sorter = gtk_column_view_get_sorter (col_view);
  sorter = g_object_ref (sorter);
  GtkSortListModel * sort_list_model =
    gtk_sort_list_model_new (G_LIST_MODEL (store), sorter);

  /* make filterable */
  self->custom_filter = gtk_custom_filter_new (filter_func, self, nullptr);
  GtkFilterListModel * filter_model = gtk_filter_list_model_new (
    G_LIST_MODEL (sort_list_model), GTK_FILTER (self->custom_filter));

  /* only allow single selection */
  GtkSingleSelection * sel = GTK_SINGLE_SELECTION (
    gtk_single_selection_new (G_LIST_MODEL (filter_model)));

  /* set model */
  gtk_column_view_set_model (col_view, GTK_SELECTION_MODEL (sel));

  /* add columns */
  ItemFactory::generate_and_append_column (
    self->track_col_view, self->item_factories, ItemFactory::Type::Text,
    Z_F_NOT_EDITABLE, Z_F_RESIZABLE, nullptr, _ ("Name"));
  ItemFactory::generate_and_append_column (
    self->track_col_view, self->item_factories, ItemFactory::Type::Toggle,
    Z_F_EDITABLE, Z_F_RESIZABLE, nullptr, _ ("Visibility"));

  /* refresh to add items */
  refresh_track_col_view_items (self);

  /* set style */
  gtk_widget_add_css_class (GTK_WIDGET (self->track_col_view), "data-table");
}

static int
get_track_type_selections (guint * selections)
{
  GVariant *    variant = g_settings_get_value (S_UI, "track-filter-type");
  gsize         n_elements;
  gconstpointer arr_ptr =
    g_variant_get_fixed_array (variant, &n_elements, sizeof (guint));
  const guint * elements = (const guint *) arr_ptr;
  for (gsize i = 0; i < n_elements; i++)
    {
      selections[i] = elements[i];
    }
  g_variant_unref (variant);

  return (int) n_elements;
}

TrackFilterPopoverWidget *
track_filter_popover_widget_new (void)
{
  TrackFilterPopoverWidget * self = Z_TRACK_FILTER_POPOVER_WIDGET (
    g_object_new (TRACK_FILTER_POPOVER_WIDGET_TYPE, nullptr));

  return self;
}

static void
track_filter_popover_finalize (TrackFilterPopoverWidget * self)
{
  std::destroy_at (&self->item_factories);

  G_OBJECT_CLASS (track_filter_popover_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
track_filter_popover_widget_class_init (TrackFilterPopoverWidgetClass * _klass)
{
  /*GtkWidgetClass * wklass = GTK_WIDGET_CLASS (_klass);*/

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = (GObjectFinalizeFunc) track_filter_popover_finalize;
}

static void
track_filter_popover_widget_init (TrackFilterPopoverWidget * self)
{
  std::construct_at (&self->item_factories);

  AdwPreferencesPage * ppage =
    ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  /*adw_preferences_page_set_title (ppage, _("Track Filtering & Visibility"));*/

  AdwPreferencesGroup * pgroup;
  AdwEntryRow *         entry_row;

  pgroup = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (pgroup, _ ("Filter"));
  adw_preferences_page_add (ppage, pgroup);

  entry_row = ADW_ENTRY_ROW (adw_entry_row_new ());
  {
    char * track_name = g_settings_get_string (S_UI, "track-filter-name");
    gtk_editable_set_text (GTK_EDITABLE (entry_row), track_name);
    g_free (track_name);
  }
  g_signal_connect (
    entry_row, "changed", G_CALLBACK (on_track_name_changed), self);
  adw_preferences_row_set_title (
    ADW_PREFERENCES_ROW (entry_row), _ ("Track name"));
  adw_preferences_group_add (pgroup, GTK_WIDGET (entry_row));

  AdwExpanderRow * exp_row = ADW_EXPANDER_ROW (adw_expander_row_new ());
  adw_preferences_row_set_title (
    ADW_PREFERENCES_ROW (exp_row), _ ("Track types"));
  MultiSelectionWidget * multi_select = multi_selection_widget_new ();
  guint                  selections[100];
  int num_selections = get_track_type_selections (selections);
  multi_selection_widget_setup (
    multi_select, (const char **) Track_Type_get_strings (),
    ENUM_VALUE_TO_INT (Track::Type::Folder) + 1, on_track_types_changed,
    selections, num_selections, self);
  GtkListBoxRow * list_box_row = GTK_LIST_BOX_ROW (gtk_list_box_row_new ());
  gtk_list_box_row_set_child (list_box_row, GTK_WIDGET (multi_select));
  adw_expander_row_add_row (exp_row, GTK_WIDGET (list_box_row));
  adw_expander_row_set_expanded (exp_row, true);
  adw_preferences_group_add (pgroup, GTK_WIDGET (exp_row));

  AdwActionRow * show_disabled_tracks_row =
    ADW_ACTION_ROW (adw_action_row_new ());
  adw_preferences_row_set_title (
    ADW_PREFERENCES_ROW (show_disabled_tracks_row), _ ("Disabled Tracks"));
  GtkSwitch * show_disabled_tracks_switch = GTK_SWITCH (gtk_switch_new ());
  gtk_switch_set_active (
    show_disabled_tracks_switch,
    g_settings_get_boolean (S_UI, "track-filter-show-disabled"));
  gtk_widget_set_valign (
    GTK_WIDGET (show_disabled_tracks_switch), GTK_ALIGN_CENTER);
  adw_action_row_add_suffix (
    show_disabled_tracks_row, GTK_WIDGET (show_disabled_tracks_switch));
  g_signal_connect (
    show_disabled_tracks_switch, "notify::active",
    G_CALLBACK (on_show_disabled_tracks_active_changed), self);
  adw_preferences_group_add (pgroup, GTK_WIDGET (show_disabled_tracks_row));

  pgroup = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (pgroup, _ ("Visibility"));
  adw_preferences_page_add (ppage, pgroup);

  list_box_row = GTK_LIST_BOX_ROW (gtk_list_box_row_new ());
  GtkColumnView * col_view = GTK_COLUMN_VIEW (gtk_column_view_new (nullptr));
  setup_col_view (self, col_view);
  gtk_list_box_row_set_child (list_box_row, GTK_WIDGET (col_view));
  adw_preferences_group_add (pgroup, GTK_WIDGET (list_box_row));

  gtk_popover_set_child (GTK_POPOVER (self), GTK_WIDGET (ppage));

  gtk_widget_set_size_request (GTK_WIDGET (self), -1, 600);
}
