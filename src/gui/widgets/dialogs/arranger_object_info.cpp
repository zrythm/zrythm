// clang-format off
// SPDX-FileCopyrightText: © 2020-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

#include "gui/backend/arranger_object.h"
#include "gui/widgets/dialogs/arranger_object_info.h"
#include "project.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "utils/ui.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  ArrangerObjectInfoDialogWidget,
  arranger_object_info_dialog_widget,
  GTK_TYPE_WINDOW)

static void
set_basic_info (
  ArrangerObjectInfoDialogWidget * self,
  AdwPreferencesPage *             pref_page,
  ArrangerObject *                 obj)
{
  self->obj = obj;

  AdwPreferencesGroup * pref_group =
    ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (pref_group, _ ("Basic Info"));
  adw_preferences_page_add (pref_page, pref_group);

  AdwActionRow * row;
  GtkLabel *     lbl;

  /* name */
  row = ADW_ACTION_ROW (adw_action_row_new ());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), _ ("Name"));
  lbl = GTK_LABEL (gtk_label_new (NULL));
  char         tmp[600];
  const char * name = arranger_object_get_name (obj);
  if (name)
    {
      gtk_label_set_text (lbl, name);
    }
  else
    {
      gtk_label_set_text (lbl, "");
    }
  adw_action_row_add_suffix (row, GTK_WIDGET (lbl));
  adw_preferences_group_add (pref_group, GTK_WIDGET (row));

  row = ADW_ACTION_ROW (adw_action_row_new ());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), _ ("Owner"));
  lbl = GTK_LABEL (gtk_label_new (NULL));
  if (arranger_object_owned_by_region (obj))
    {
      ZRegion * region = arranger_object_get_region (obj);
      g_return_if_fail (region);
      sprintf (
        tmp, "%s [tr %u, ln %d, at %d, idx %d]", region->name,
        region->id.track_name_hash, region->id.lane_pos, region->id.at_idx,
        region->id.idx);
      gtk_label_set_text (lbl, tmp);
    }
  else
    {
      Track * track = arranger_object_get_track (obj);
      g_return_if_fail (IS_TRACK_AND_NONNULL (track));
      gtk_label_set_text (lbl, track->name);
    }
  adw_action_row_add_suffix (row, GTK_WIDGET (lbl));
  adw_preferences_group_add (pref_group, GTK_WIDGET (row));
}

/**
 * Creates a new arranger_object_info dialog.
 */
ArrangerObjectInfoDialogWidget *
arranger_object_info_dialog_widget_new (ArrangerObject * object)
{
  ArrangerObjectInfoDialogWidget * self = Z_ARRANGER_OBJECT_INFO_DIALOG_WIDGET (
    g_object_new (ARRANGER_OBJECT_INFO_DIALOG_WIDGET_TYPE, NULL));

  AdwPreferencesPage * pref_page =
    ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  gtk_window_set_child (GTK_WINDOW (self), GTK_WIDGET (pref_page));
  set_basic_info (self, pref_page, object);

  return self;
}

static void
arranger_object_info_dialog_widget_class_init (
  ArrangerObjectInfoDialogWidgetClass * _klass)
{
}

static void
arranger_object_info_dialog_widget_init (ArrangerObjectInfoDialogWidget * self)
{
  gtk_window_set_title (GTK_WINDOW (self), _ ("Arranger Object Info"));
  gtk_window_set_icon_name (GTK_WINDOW (self), "zrythm");
}
