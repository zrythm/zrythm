// SPDX-FileCopyrightText: © 2020-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/project.h"
#include "gui/cpp/gtk_widgets/dialogs/arranger_object_info.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#include <glib/gi18n.h>

#include "common/dsp/arranger_object.h"

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
  lbl = GTK_LABEL (gtk_label_new (nullptr));
  if (auto nameable_obj = dynamic_cast<NameableObject *> (obj))
    {
      gtk_label_set_text (lbl, nameable_obj->get_name ().c_str ());
    }
  else
    {
      gtk_label_set_text (lbl, "");
    }
  adw_action_row_add_suffix (row, GTK_WIDGET (lbl));
  adw_preferences_group_add (pref_group, GTK_WIDGET (row));

  row = ADW_ACTION_ROW (adw_action_row_new ());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), _ ("Owner"));
  lbl = GTK_LABEL (gtk_label_new (nullptr));
  if (obj->owned_by_region ())
    {
      std::visit (
        [&] (auto &&region_owned_obj) {
          auto region = region_owned_obj->get_region ();
          z_return_if_fail (region);
          auto tmp = fmt::format (
            "{} [tr {}, ln {}, at {}, idx {}]", region->name_,
            region->id_.track_name_hash_, region->id_.lane_pos_,
            region->id_.at_idx_, region->id_.idx_);
          gtk_label_set_text (lbl, tmp.c_str ());
        },
        convert_to_variant<RegionOwnedObjectPtrVariant> (obj));
    }
  else
    {
      Track * track = obj->get_track ();
      z_return_if_fail (track);
      gtk_label_set_text (lbl, track->name_.c_str ());
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
    g_object_new (ARRANGER_OBJECT_INFO_DIALOG_WIDGET_TYPE, nullptr));

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
