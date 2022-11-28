// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/snap_grid.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/snap_grid_popover.h"
#include "project.h"
#include "utils/gtk.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  SnapGridPopoverWidget,
  snap_grid_popover_widget,
  GTK_TYPE_POPOVER)

static void
on_closed (SnapGridPopoverWidget * self, gpointer user_data)
{
  snap_grid_widget_refresh (self->owner);
}

static void
refresh (SnapGridPopoverWidget * self)
{
  SnapGrid * sg = self->owner->snap_grid;
  bool       snap_controls_visible = !sg->snap_adaptive;
  gtk_widget_set_visible (
    GTK_WIDGET (self->snap_length), snap_controls_visible);

  bool object_length_visible =
    sg->length_type == NOTE_LENGTH_CUSTOM;
  gtk_widget_set_visible (
    GTK_WIDGET (self->object_length), object_length_visible);
  gtk_widget_set_visible (
    GTK_WIDGET (self->object_length_type_custom),
    object_length_visible);

  /* set sensitivity */
  bool snap_sensitive = sg->snap_to_grid;
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->adaptive_snap_row), snap_sensitive);
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->snap_length), snap_sensitive);
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->snap_type), snap_sensitive);
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->keep_offset_row), snap_sensitive);
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->snap_to_events_row), snap_sensitive);
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->object_length_type), snap_sensitive);
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->object_length), snap_sensitive);
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->object_length_type_custom),
    snap_sensitive);
}

static void
on_snap_to_grid_active_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  SnapGridPopoverWidget * self =
    Z_SNAP_GRID_POPOVER_WIDGET (user_data);
  SnapGrid *  sg = self->owner->snap_grid;
  GtkSwitch * sw = GTK_SWITCH (gobject);

  sg->snap_to_grid = gtk_switch_get_active (sw);

  refresh (self);
}

static void
on_adaptive_snap_active_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  SnapGridPopoverWidget * self =
    Z_SNAP_GRID_POPOVER_WIDGET (user_data);
  SnapGrid *  sg = self->owner->snap_grid;
  GtkSwitch * sw = GTK_SWITCH (gobject);

  sg->snap_adaptive = gtk_switch_get_active (sw);

  refresh (self);
}

static void
on_keep_offset_active_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  SnapGridPopoverWidget * self =
    Z_SNAP_GRID_POPOVER_WIDGET (user_data);
  SnapGrid *  sg = self->owner->snap_grid;
  GtkSwitch * sw = GTK_SWITCH (gobject);

  sg->snap_to_grid_keep_offset = gtk_switch_get_active (sw);

  refresh (self);
}

static void
on_snap_to_events_active_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  SnapGridPopoverWidget * self =
    Z_SNAP_GRID_POPOVER_WIDGET (user_data);
  SnapGrid *  sg = self->owner->snap_grid;
  GtkSwitch * sw = GTK_SWITCH (gobject);

  sg->snap_to_events = gtk_switch_get_active (sw);

  refresh (self);
}

static void
on_snap_length_selection_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  SnapGridPopoverWidget * self =
    Z_SNAP_GRID_POPOVER_WIDGET (user_data);
  SnapGrid *    sg = self->owner->snap_grid;
  AdwComboRow * cr = ADW_COMBO_ROW (gobject);

  sg->snap_note_length = adw_combo_row_get_selected (cr);

  refresh (self);
}

static void
on_snap_type_selection_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  SnapGridPopoverWidget * self =
    Z_SNAP_GRID_POPOVER_WIDGET (user_data);
  SnapGrid *    sg = self->owner->snap_grid;
  AdwComboRow * cr = ADW_COMBO_ROW (gobject);

  sg->snap_note_type = adw_combo_row_get_selected (cr);

  refresh (self);
}

static void
on_object_length_type_selection_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  SnapGridPopoverWidget * self =
    Z_SNAP_GRID_POPOVER_WIDGET (user_data);
  SnapGrid *    sg = self->owner->snap_grid;
  AdwComboRow * cr = ADW_COMBO_ROW (gobject);

  sg->length_type = adw_combo_row_get_selected (cr);

  refresh (self);
}

static void
on_object_length_selection_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  SnapGridPopoverWidget * self =
    Z_SNAP_GRID_POPOVER_WIDGET (user_data);
  SnapGrid *    sg = self->owner->snap_grid;
  AdwComboRow * cr = ADW_COMBO_ROW (gobject);

  sg->default_note_length = adw_combo_row_get_selected (cr);

  refresh (self);
}

static void
on_object_length_type_custom_selection_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  SnapGridPopoverWidget * self =
    Z_SNAP_GRID_POPOVER_WIDGET (user_data);
  SnapGrid *    sg = self->owner->snap_grid;
  AdwComboRow * cr = ADW_COMBO_ROW (gobject);

  sg->default_note_type = adw_combo_row_get_selected (cr);

  refresh (self);
}

/**
 * Creates a digital meter with the given type (bpm or position).
 */
SnapGridPopoverWidget *
snap_grid_popover_widget_new (SnapGridWidget * owner)
{
  SnapGridPopoverWidget * self =
    g_object_new (SNAP_GRID_POPOVER_WIDGET_TYPE, NULL);

  self->owner = owner;
  SnapGrid * sg = self->owner->snap_grid;

  self->pref_page =
    ADW_PREFERENCES_PAGE (adw_preferences_page_new ());

  self->snap_position_group =
    ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_page_add (
    self->pref_page, self->snap_position_group);
  adw_preferences_group_set_title (
    self->snap_position_group, _ ("Snap"));
  adw_preferences_group_set_description (
    self->snap_position_group,
    _ ("Position snapping options"));
  self->snap_to_grid = GTK_SWITCH (gtk_switch_new ());
  gtk_switch_set_active (self->snap_to_grid, sg->snap_to_grid);
  gtk_widget_set_valign (
    GTK_WIDGET (self->snap_to_grid), GTK_ALIGN_CENTER);
  adw_preferences_group_set_header_suffix (
    self->snap_position_group,
    GTK_WIDGET (self->snap_to_grid));

  /* adjustive snap */
  self->adaptive_snap = GTK_SWITCH (gtk_switch_new ());
  gtk_switch_set_active (
    self->adaptive_snap, sg->snap_adaptive);
  gtk_widget_set_valign (
    GTK_WIDGET (self->adaptive_snap), GTK_ALIGN_CENTER);
  self->adaptive_snap_row =
    ADW_ACTION_ROW (adw_action_row_new ());
  adw_action_row_add_suffix (
    self->adaptive_snap_row, GTK_WIDGET (self->adaptive_snap));
  adw_action_row_set_activatable_widget (
    self->adaptive_snap_row, GTK_WIDGET (self->adaptive_snap));
  adw_preferences_row_set_title (
    ADW_PREFERENCES_ROW (self->adaptive_snap_row),
    _ ("Adaptive Snap"));
  adw_action_row_set_subtitle (
    ADW_ACTION_ROW (self->adaptive_snap_row),
    _ ("Adjust snapping automatically based on the current zoom level. The note type still applies."));
  adw_preferences_group_add (
    self->snap_position_group,
    GTK_WIDGET (self->adaptive_snap_row));

  /* note length */
  GtkStringList * strlist =
    z_gtk_string_list_new_from_cyaml_strvals (
      note_length_strings, G_N_ELEMENTS (note_length_strings),
      true);
  self->snap_length = ADW_COMBO_ROW (adw_combo_row_new ());
  adw_combo_row_set_model (
    self->snap_length, G_LIST_MODEL (strlist));
  adw_combo_row_set_selected (
    self->snap_length, sg->snap_note_length);
  adw_preferences_row_set_title (
    ADW_PREFERENCES_ROW (self->snap_length), _ ("Snap to"));
  adw_action_row_set_subtitle (
    ADW_ACTION_ROW (self->snap_length),
    _ ("Note size to snap to"));
  adw_preferences_group_add (
    self->snap_position_group, GTK_WIDGET (self->snap_length));

  /* note type */
  strlist = z_gtk_string_list_new_from_cyaml_strvals (
    note_type_strings, G_N_ELEMENTS (note_type_strings), true);
  self->snap_type = ADW_COMBO_ROW (adw_combo_row_new ());
  adw_combo_row_set_model (
    self->snap_type, G_LIST_MODEL (strlist));
  adw_combo_row_set_selected (
    self->snap_type, sg->snap_note_type);
  adw_preferences_row_set_title (
    ADW_PREFERENCES_ROW (self->snap_type), _ ("Note Type"));
  adw_preferences_group_add (
    self->snap_position_group, GTK_WIDGET (self->snap_type));

  /* keep offset */
  self->keep_offset = GTK_SWITCH (gtk_switch_new ());
  gtk_switch_set_active (
    self->keep_offset, sg->snap_to_grid_keep_offset);
  gtk_widget_set_valign (
    GTK_WIDGET (self->keep_offset), GTK_ALIGN_CENTER);
  self->keep_offset_row =
    ADW_ACTION_ROW (adw_action_row_new ());
  adw_action_row_add_suffix (
    self->keep_offset_row, GTK_WIDGET (self->keep_offset));
  adw_action_row_set_activatable_widget (
    self->keep_offset_row, GTK_WIDGET (self->keep_offset));
  adw_preferences_row_set_title (
    ADW_PREFERENCES_ROW (self->keep_offset_row),
    _ ("Keep Offset"));
  adw_action_row_set_subtitle (
    ADW_ACTION_ROW (self->keep_offset_row),
    _ ("Keep offset of objects from the snap point when moving to a new position"));
  adw_preferences_group_add (
    self->snap_position_group,
    GTK_WIDGET (self->keep_offset_row));

  /* snap to events */
  self->snap_to_events = GTK_SWITCH (gtk_switch_new ());
  gtk_switch_set_active (
    self->snap_to_events, sg->snap_to_events);
  gtk_widget_set_valign (
    GTK_WIDGET (self->snap_to_events), GTK_ALIGN_CENTER);
  self->snap_to_events_row =
    ADW_ACTION_ROW (adw_action_row_new ());
  adw_action_row_add_suffix (
    self->snap_to_events_row,
    GTK_WIDGET (self->snap_to_events));
  adw_action_row_set_activatable_widget (
    self->snap_to_events_row,
    GTK_WIDGET (self->snap_to_events));
  adw_preferences_row_set_title (
    ADW_PREFERENCES_ROW (self->snap_to_events_row),
    _ ("Snap to Events"));
  adw_action_row_set_subtitle (
    ADW_ACTION_ROW (self->snap_to_events_row),
    _ ("Snap to other events"));
  adw_preferences_group_add (
    self->snap_position_group,
    GTK_WIDGET (self->snap_to_events_row));

  /* --- object lengths --- */
  self->object_length_group =
    ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_page_add (
    self->pref_page, self->object_length_group);
  adw_preferences_group_set_title (
    self->object_length_group, _ ("Object Length"));
  adw_preferences_group_set_description (
    self->object_length_group,
    _ ("Default object length options"));

  /* object length type */
  const char * strings[] = {
    _ ("Link to snap"),
    _ ("Last object"),
    _ ("Custom"),
    NULL,
  };
  strlist = gtk_string_list_new (strings);
  self->object_length_type =
    ADW_COMBO_ROW (adw_combo_row_new ());
  adw_combo_row_set_model (
    self->object_length_type, G_LIST_MODEL (strlist));
  adw_combo_row_set_selected (
    self->object_length_type, sg->length_type);
  adw_preferences_row_set_title (
    ADW_PREFERENCES_ROW (self->object_length_type),
    _ ("Object Length Lype"));
  adw_action_row_set_subtitle (
    ADW_ACTION_ROW (self->object_length_type),
    _ ("Link to snap settings, use the length of the last created object, or specify a custom length"));
  adw_preferences_group_add (
    self->object_length_group,
    GTK_WIDGET (self->object_length_type));

  /* note length */
  strlist = z_gtk_string_list_new_from_cyaml_strvals (
    note_length_strings, G_N_ELEMENTS (note_length_strings),
    true);
  self->object_length = ADW_COMBO_ROW (adw_combo_row_new ());
  adw_combo_row_set_model (
    self->object_length, G_LIST_MODEL (strlist));
  adw_combo_row_set_selected (
    self->object_length, sg->default_note_length);
  adw_preferences_row_set_title (
    ADW_PREFERENCES_ROW (self->object_length), _ ("Length"));
  adw_preferences_group_add (
    self->object_length_group,
    GTK_WIDGET (self->object_length));

  /* note type */
  strlist = z_gtk_string_list_new_from_cyaml_strvals (
    note_type_strings, G_N_ELEMENTS (note_type_strings), true);
  self->object_length_type_custom =
    ADW_COMBO_ROW (adw_combo_row_new ());
  adw_combo_row_set_model (
    self->object_length_type_custom, G_LIST_MODEL (strlist));
  adw_combo_row_set_selected (
    self->object_length_type_custom, sg->default_note_type);
  adw_preferences_row_set_title (
    ADW_PREFERENCES_ROW (self->object_length_type_custom),
    _ ("Note Type"));
  adw_preferences_group_add (
    self->object_length_group,
    GTK_WIDGET (self->object_length_type_custom));

  gtk_popover_set_child (
    GTK_POPOVER (self), GTK_WIDGET (self->pref_page));

  g_signal_connect (
    GTK_WIDGET (self->snap_to_grid), "notify::active",
    G_CALLBACK (on_snap_to_grid_active_changed), self);
  g_signal_connect (
    GTK_WIDGET (self->adaptive_snap), "notify::active",
    G_CALLBACK (on_adaptive_snap_active_changed), self);
  g_signal_connect (
    GTK_WIDGET (self->snap_length), "notify::selected",
    G_CALLBACK (on_snap_length_selection_changed), self);
  g_signal_connect (
    GTK_WIDGET (self->snap_type), "notify::selected",
    G_CALLBACK (on_snap_type_selection_changed), self);
  g_signal_connect (
    GTK_WIDGET (self->keep_offset), "notify::active",
    G_CALLBACK (on_keep_offset_active_changed), self);
  g_signal_connect (
    GTK_WIDGET (self->snap_to_events), "notify::active",
    G_CALLBACK (on_snap_to_events_active_changed), self);
  g_signal_connect (
    GTK_WIDGET (self->object_length_type), "notify::selected",
    G_CALLBACK (on_object_length_type_selection_changed),
    self);
  g_signal_connect (
    GTK_WIDGET (self->object_length), "notify::selected",
    G_CALLBACK (on_object_length_selection_changed), self);
  g_signal_connect (
    GTK_WIDGET (self->object_length_type_custom),
    "notify::selected",
    G_CALLBACK (on_object_length_type_custom_selection_changed),
    self);

  refresh (self);

  return self;
}

static void
snap_grid_popover_widget_class_init (
  SnapGridPopoverWidgetClass * _klass)
{
}

static void
snap_grid_popover_widget_init (SnapGridPopoverWidget * self)
{
  gtk_widget_set_size_request (GTK_WIDGET (self), 520, -1);

  g_signal_connect (
    G_OBJECT (self), "closed", G_CALLBACK (on_closed), self);
}
