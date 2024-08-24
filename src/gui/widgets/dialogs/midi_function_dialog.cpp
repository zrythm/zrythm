// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_selections.h"
#include "gui/widgets/dialogs/midi_function_dialog.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "zrythm.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (
  MidiFunctionDialogWidget,
  midi_function_dialog_widget,
  ADW_TYPE_WINDOW)

static char *
get_g_settings_schema_id (MidiFunctionType type)
{
  char * type_id = midi_function_type_to_string_id (type);
  char * ret = g_strdup_printf (
    "%s%s", GSETTINGS_ZRYTHM_PREFIX ".ui.midi-functions.", type_id);
  g_free (type_id);
  return ret;
}

/**
 * Fills in @ref opts with the current options in the dialog
 * (fetched from gsettings).
 */
void
midi_function_dialog_widget_get_opts (
  MidiFunctionDialogWidget * self,
  MidiFunctionOpts *         opts)
{
  memset (opts, 0, sizeof (MidiFunctionOpts));

  switch (self->type)
    {
    case MidiFunctionType::Crescendo:
      opts->start_vel_ = g_settings_get_int (self->settings, "start-velocity");
      opts->end_vel_ = g_settings_get_int (self->settings, "end-velocity");
      opts->curve_algo_ = ENUM_INT_TO_VALUE (
        CurveOptions::Algorithm,
        g_settings_get_enum (self->settings, "curve-algo"));
      opts->curviness_ =
        ((double) g_settings_get_int (self->settings, "curviness") / 100.0) * 2.0
        - 1.0;
      break;
    case MidiFunctionType::Flam:
      opts->time_ = g_settings_get_double (self->settings, "time");
      opts->vel_ = g_settings_get_int (self->settings, "velocity");
      break;
    case MidiFunctionType::Strum:
      opts->ascending_ = g_settings_get_boolean (self->settings, "ascending");
      opts->time_ = g_settings_get_double (self->settings, "total-time");
      opts->curve_algo_ = ENUM_INT_TO_VALUE (
        CurveOptions::Algorithm,
        g_settings_get_enum (self->settings, "curve-algo"));
      opts->curviness_ =
        ((double) g_settings_get_int (self->settings, "curviness") / 100.0) * 2.0
        - 1.0;
      break;
    default:
      z_warning ("unimplemented");
      break;
    }
}

static void
init_fields (MidiFunctionDialogWidget * self)
{
  AdwPreferencesGroup * pgroup =
    ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (pgroup, _ ("Options"));
  adw_preferences_page_add (self->page, pgroup);

  AdwActionRow * row;

  switch (self->type)
    {
    case MidiFunctionType::Crescendo:
      {
        row = ADW_ACTION_ROW (adw_spin_row_new_with_range (1, 127, 1));
        adw_preferences_row_set_title (
          ADW_PREFERENCES_ROW (row), _ ("Start Velocity"));
        adw_spin_row_set_digits (ADW_SPIN_ROW (row), 0);
        adw_spin_row_set_value (
          ADW_SPIN_ROW (row),
          g_settings_get_int (self->settings, "start-velocity"));
        g_settings_bind (
          self->settings, "start-velocity", row, "value",
          G_SETTINGS_BIND_DEFAULT);
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));

        row = ADW_ACTION_ROW (adw_spin_row_new_with_range (1, 127, 1));
        adw_preferences_row_set_title (
          ADW_PREFERENCES_ROW (row), _ ("End Velocity"));
        adw_spin_row_set_digits (ADW_SPIN_ROW (row), 0);
        adw_spin_row_set_value (
          ADW_SPIN_ROW (row),
          g_settings_get_int (self->settings, "end-velocity"));
        g_settings_bind (
          self->settings, "end-velocity", row, "value", G_SETTINGS_BIND_DEFAULT);
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));

        row = ADW_ACTION_ROW (adw_combo_row_new ());
        adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), _ ("Curve"));
        GtkStringList * slist = gtk_string_list_new (nullptr);
        for (
          unsigned int i = ENUM_VALUE_TO_INT (CurveOptions::Algorithm::Exponent);
          i < ENUM_COUNT (CurveOptions::Algorithm); i++)
          {
            CurveOptions::Algorithm algo =
              ENUM_INT_TO_VALUE (CurveOptions::Algorithm, i);
            auto buf = CurveOptions_Algorithm_to_string (algo, true);
            gtk_string_list_append (slist, buf.c_str ());
          }
        adw_combo_row_set_model (ADW_COMBO_ROW (row), G_LIST_MODEL (slist));
        adw_combo_row_set_selected (
          ADW_COMBO_ROW (row),
          (guint) g_settings_get_enum (self->settings, "curve-algo"));
        g_settings_bind_with_mapping (
          self->settings, "curve-algo", row, "selected", G_SETTINGS_BIND_DEFAULT,
          (GSettingsBindGetMapping) curve_algorithm_get_g_settings_mapping,
          (GSettingsBindSetMapping) curve_algorithm_set_g_settings_mapping,
          nullptr, nullptr);
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));

        row = ADW_ACTION_ROW (adw_spin_row_new_with_range (0, 100, 1));
        adw_preferences_row_set_title (
          ADW_PREFERENCES_ROW (row), _ ("Curviness %"));
        adw_spin_row_set_digits (ADW_SPIN_ROW (row), 0);
        adw_spin_row_set_value (
          ADW_SPIN_ROW (row), g_settings_get_int (self->settings, "curviness"));
        g_settings_bind (
          self->settings, "curviness", row, "value", G_SETTINGS_BIND_DEFAULT);
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));
      }
      break;
    case MidiFunctionType::Flam:
      {
        adw_preferences_group_set_description (
          pgroup, _ ("Note: Flam is currently unimplemented"));

        row = ADW_ACTION_ROW (adw_spin_row_new_with_range (-100, 100, 1));
        adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), _ ("Time"));
        adw_spin_row_set_digits (ADW_SPIN_ROW (row), 0);
        adw_spin_row_set_value (
          ADW_SPIN_ROW (row), g_settings_get_double (self->settings, "time"));
        g_settings_bind (
          self->settings, "time", row, "value", G_SETTINGS_BIND_DEFAULT);
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));

        row = ADW_ACTION_ROW (adw_spin_row_new_with_range (1, 127, 1));
        adw_preferences_row_set_title (
          ADW_PREFERENCES_ROW (row), _ ("Velocity"));
        adw_spin_row_set_digits (ADW_SPIN_ROW (row), 0);
        adw_spin_row_set_value (
          ADW_SPIN_ROW (row), g_settings_get_int (self->settings, "velocity"));
        g_settings_bind (
          self->settings, "velocity", row, "value", G_SETTINGS_BIND_DEFAULT);
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));
      }
      break;
    case MidiFunctionType::Strum:
      {
        row = ADW_ACTION_ROW (adw_switch_row_new ());
        adw_preferences_row_set_title (
          ADW_PREFERENCES_ROW (row), _ ("Ascending"));
        adw_switch_row_set_active (
          ADW_SWITCH_ROW (row),
          g_settings_get_boolean (self->settings, "ascending"));
        g_settings_bind (
          self->settings, "ascending", row, "active", G_SETTINGS_BIND_DEFAULT);
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));

        row = ADW_ACTION_ROW (adw_combo_row_new ());
        adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), _ ("Curve"));
        GtkStringList * slist = gtk_string_list_new (nullptr);
        for (
          unsigned int i = ENUM_VALUE_TO_INT (CurveOptions::Algorithm::Exponent);
          i < ENUM_COUNT (CurveOptions::Algorithm); i++)
          {
            CurveOptions::Algorithm algo =
              ENUM_INT_TO_VALUE (CurveOptions::Algorithm, i);
            gtk_string_list_append (
              slist, CurveOptions_Algorithm_to_string (algo, true).c_str ());
          }
        adw_combo_row_set_model (ADW_COMBO_ROW (row), G_LIST_MODEL (slist));
        adw_combo_row_set_selected (
          ADW_COMBO_ROW (row),
          (guint) g_settings_get_enum (self->settings, "curve-algo"));
        g_settings_bind_with_mapping (
          self->settings, "curve-algo", row, "selected", G_SETTINGS_BIND_DEFAULT,
          (GSettingsBindGetMapping) curve_algorithm_get_g_settings_mapping,
          (GSettingsBindSetMapping) curve_algorithm_set_g_settings_mapping,
          nullptr, nullptr);
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));

        row = ADW_ACTION_ROW (adw_spin_row_new_with_range (0, 100, 1));
        adw_preferences_row_set_title (
          ADW_PREFERENCES_ROW (row), _ ("Curviness %"));
        adw_spin_row_set_digits (ADW_SPIN_ROW (row), 0);
        adw_spin_row_set_value (
          ADW_SPIN_ROW (row), g_settings_get_int (self->settings, "curviness"));
        g_settings_bind (
          self->settings, "curviness", row, "value", G_SETTINGS_BIND_DEFAULT);
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));

        row = ADW_ACTION_ROW (adw_spin_row_new_with_range (1, 200, 1));
        adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), _ ("Time"));
        adw_spin_row_set_digits (ADW_SPIN_ROW (row), 0);
        adw_spin_row_set_value (
          ADW_SPIN_ROW (row),
          g_settings_get_double (self->settings, "total-time"));
        g_settings_bind (
          self->settings, "total-time", row, "value", G_SETTINGS_BIND_DEFAULT);
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));
      }
      break;
    default:
      break;
    }
}

static void
on_ok_clicked (GtkButton * btn, MidiFunctionDialogWidget * self)
{
  if (!MIDI_SELECTIONS->has_any ())
    {
      z_debug ("no selections, doing nothing");
      return;
    }

  MidiFunctionOpts opts;
  midi_function_dialog_widget_get_opts (self, &opts);

  try
    {
      UNDO_MANAGER->perform (EditArrangerSelectionsAction::create (
        *MIDI_SELECTIONS, self->type, opts));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to apply MIDI function"));
    }

  gtk_window_close (GTK_WINDOW (self));
}

/**
 * Creates a MIDI function dialog for the given type and
 * pre-fills the values from GSettings.
 */
MidiFunctionDialogWidget *
midi_function_dialog_widget_new (GtkWindow * parent, MidiFunctionType type)
{
  MidiFunctionDialogWidget * self = Z_MIDI_FUNCTION_DIALOG_WIDGET (
    g_object_new (MIDI_FUNCTION_DIALOG_WIDGET_TYPE, nullptr));

  self->type = type;

  gtk_window_set_title (
    GTK_WINDOW (self), MidiFunctionType_to_string (type, true).c_str ());

  if (parent)
    {
      gtk_window_set_transient_for (GTK_WINDOW (self), parent);
    }

  char * schema_id = get_g_settings_schema_id (self->type);
  self->settings = g_settings_new (schema_id);
  g_free (schema_id);

  init_fields (self);

  return self;
}

static void
dispose (MidiFunctionDialogWidget * self)
{
  g_object_unref_and_null (self->settings);

  G_OBJECT_CLASS (midi_function_dialog_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
midi_function_dialog_widget_class_init (MidiFunctionDialogWidgetClass * _klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}

static void
midi_function_dialog_widget_init (MidiFunctionDialogWidget * self)
{
  gtk_window_set_icon_name (GTK_WINDOW (self), "zrythm");
  gtk_window_set_modal (GTK_WINDOW (self), true);

  self->view = ADW_TOOLBAR_VIEW (adw_toolbar_view_new ());
  adw_window_set_content (ADW_WINDOW (self), GTK_WIDGET (self->view));

  self->header = ADW_HEADER_BAR (adw_header_bar_new ());
  self->ok_btn = GTK_BUTTON (gtk_button_new_with_mnemonic (_ ("_Apply")));
  g_signal_connect (
    G_OBJECT (self->ok_btn), "clicked", G_CALLBACK (on_ok_clicked), self);
  gtk_widget_add_css_class (GTK_WIDGET (self->ok_btn), "suggested-action");
  adw_header_bar_pack_end (self->header, GTK_WIDGET (self->ok_btn));
  self->cancel_btn = GTK_BUTTON (gtk_button_new_with_mnemonic (_ ("_Cancel")));
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->cancel_btn), "window.close");
  adw_header_bar_pack_start (self->header, GTK_WIDGET (self->cancel_btn));
  adw_toolbar_view_add_top_bar (self->view, GTK_WIDGET (self->header));

  self->page = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  adw_toolbar_view_set_content (self->view, GTK_WIDGET (self->page));
  adw_header_bar_set_show_end_title_buttons (self->header, false);

  /* close on escape */
  z_gtk_window_make_escapable (GTK_WINDOW (self));
}
