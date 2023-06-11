// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/dialogs/midi_function_dialog.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/error.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/ui.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  MidiFunctionDialogWidget,
  midi_function_dialog_widget,
  ADW_TYPE_WINDOW)

static char *
get_g_settings_schema_id (MidiFunctionType type)
{
  char * type_id = midi_function_type_to_string_id (type);
  char * ret = g_strdup_printf (
    "%s%s", GSETTINGS_ZRYTHM_PREFIX ".ui.midi-functions.",
    type_id);
  g_free (type_id);
  return ret;
}

/**
 * Fills in \ref opts with the current options in the dialog
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
    case MIDI_FUNCTION_CRESCENDO:
      opts->start_vel =
        g_settings_get_int (self->settings, "start-velocity");
      opts->end_vel =
        g_settings_get_int (self->settings, "end-velocity");
      opts->curve_algo =
        g_settings_get_enum (self->settings, "curve-algo");
      opts->curviness =
        ((double) g_settings_get_int (
           self->settings, "curviness")
         / 100.0)
          * 2.0
        - 1.0;
      break;
    case MIDI_FUNCTION_FLAM:
      opts->time =
        g_settings_get_double (self->settings, "time");
      opts->vel =
        g_settings_get_int (self->settings, "velocity");
      break;
    case MIDI_FUNCTION_STRUM:
      opts->ascending =
        g_settings_get_boolean (self->settings, "ascending");
      opts->time =
        g_settings_get_double (self->settings, "total-time");
      opts->curve_algo =
        g_settings_get_enum (self->settings, "curve-algo");
      opts->curviness =
        ((double) g_settings_get_int (
           self->settings, "curviness")
         / 100.0)
          * 2.0
        - 1.0;
      break;
    default:
      g_warning ("unimplemented");
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
    case MIDI_FUNCTION_CRESCENDO:
      {
        row = ADW_ACTION_ROW (adw_action_row_new ());
        adw_preferences_row_set_title (
          ADW_PREFERENCES_ROW (row), _ ("Start Velocity"));
        GtkSpinButton * spin_btn = GTK_SPIN_BUTTON (
          gtk_spin_button_new_with_range (1, 127, 1));
        gtk_spin_button_set_digits (spin_btn, 0);
        gtk_spin_button_set_value (
          spin_btn,
          g_settings_get_int (
            self->settings, "start-velocity"));
        g_settings_bind (
          self->settings, "start-velocity", spin_btn, "value",
          G_SETTINGS_BIND_DEFAULT);
        gtk_widget_set_valign (
          GTK_WIDGET (spin_btn), GTK_ALIGN_CENTER);
        adw_action_row_add_suffix (
          ADW_ACTION_ROW (row), GTK_WIDGET (spin_btn));
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));

        row = ADW_ACTION_ROW (adw_action_row_new ());
        adw_preferences_row_set_title (
          ADW_PREFERENCES_ROW (row), _ ("End Velocity"));
        spin_btn = GTK_SPIN_BUTTON (
          gtk_spin_button_new_with_range (1, 127, 1));
        gtk_spin_button_set_digits (spin_btn, 0);
        gtk_spin_button_set_value (
          spin_btn,
          g_settings_get_int (self->settings, "end-velocity"));
        g_settings_bind (
          self->settings, "end-velocity", spin_btn, "value",
          G_SETTINGS_BIND_DEFAULT);
        gtk_widget_set_valign (
          GTK_WIDGET (spin_btn), GTK_ALIGN_CENTER);
        adw_action_row_add_suffix (
          ADW_ACTION_ROW (row), GTK_WIDGET (spin_btn));
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));

        row = ADW_ACTION_ROW (adw_combo_row_new ());
        adw_preferences_row_set_title (
          ADW_PREFERENCES_ROW (row), _ ("Curve"));
        GtkStringList * slist = gtk_string_list_new (NULL);
        for (CurveAlgorithm i = CURVE_ALGORITHM_EXPONENT;
             i < NUM_CURVE_ALGORITHMS; i++)
          {
            CurveAlgorithm algo = i;
            char           buf[600];
            curve_algorithm_get_localized_name (algo, buf);
            gtk_string_list_append (slist, buf);
          }
        adw_combo_row_set_model (
          ADW_COMBO_ROW (row), G_LIST_MODEL (slist));
        adw_combo_row_set_selected (
          ADW_COMBO_ROW (row),
          (guint) g_settings_get_enum (
            self->settings, "curve-algo"));
        g_settings_bind_with_mapping (
          self->settings, "curve-algo", row, "selected",
          G_SETTINGS_BIND_DEFAULT,
          (GSettingsBindGetMapping)
            curve_algorithm_get_g_settings_mapping,
          (GSettingsBindSetMapping)
            curve_algorithm_set_g_settings_mapping,
          NULL, NULL);
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));

        row = ADW_ACTION_ROW (adw_action_row_new ());
        adw_preferences_row_set_title (
          ADW_PREFERENCES_ROW (row), _ ("Curviness %"));
        spin_btn = GTK_SPIN_BUTTON (
          gtk_spin_button_new_with_range (0, 100, 1));
        gtk_spin_button_set_digits (spin_btn, 0);
        gtk_spin_button_set_value (
          spin_btn,
          g_settings_get_int (self->settings, "curviness"));
        g_settings_bind (
          self->settings, "curviness", spin_btn, "value",
          G_SETTINGS_BIND_DEFAULT);
        gtk_widget_set_valign (
          GTK_WIDGET (spin_btn), GTK_ALIGN_CENTER);
        adw_action_row_add_suffix (
          ADW_ACTION_ROW (row), GTK_WIDGET (spin_btn));
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));
      }
      break;
    case MIDI_FUNCTION_FLAM:
      {
        adw_preferences_group_set_description (
          pgroup, _ ("Note: Flam is currently unimplemented"));

        row = ADW_ACTION_ROW (adw_action_row_new ());
        adw_preferences_row_set_title (
          ADW_PREFERENCES_ROW (row), _ ("Time"));
        GtkSpinButton * spin_btn = GTK_SPIN_BUTTON (
          gtk_spin_button_new_with_range (-100, 100, 1));
        gtk_spin_button_set_digits (spin_btn, 0);
        gtk_spin_button_set_value (
          spin_btn,
          g_settings_get_double (self->settings, "time"));
        g_settings_bind (
          self->settings, "time", spin_btn, "value",
          G_SETTINGS_BIND_DEFAULT);
        gtk_widget_set_valign (
          GTK_WIDGET (spin_btn), GTK_ALIGN_CENTER);
        adw_action_row_add_suffix (
          ADW_ACTION_ROW (row), GTK_WIDGET (spin_btn));
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));

        row = ADW_ACTION_ROW (adw_action_row_new ());
        adw_preferences_row_set_title (
          ADW_PREFERENCES_ROW (row), _ ("Velocity"));
        spin_btn = GTK_SPIN_BUTTON (
          gtk_spin_button_new_with_range (1, 127, 1));
        gtk_spin_button_set_digits (spin_btn, 0);
        gtk_spin_button_set_value (
          spin_btn,
          g_settings_get_int (self->settings, "velocity"));
        g_settings_bind (
          self->settings, "velocity", spin_btn, "value",
          G_SETTINGS_BIND_DEFAULT);
        gtk_widget_set_valign (
          GTK_WIDGET (spin_btn), GTK_ALIGN_CENTER);
        adw_action_row_add_suffix (
          ADW_ACTION_ROW (row), GTK_WIDGET (spin_btn));
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));
      }
      break;
    case MIDI_FUNCTION_STRUM:
      {
        row = ADW_ACTION_ROW (adw_action_row_new ());
        adw_preferences_row_set_title (
          ADW_PREFERENCES_ROW (row), _ ("Ascending"));
        GtkSwitch * s = GTK_SWITCH (gtk_switch_new ());
        gtk_switch_set_active (
          s,
          g_settings_get_boolean (self->settings, "ascending"));
        g_settings_bind (
          self->settings, "ascending", s, "active",
          G_SETTINGS_BIND_DEFAULT);
        gtk_widget_set_valign (
          GTK_WIDGET (s), GTK_ALIGN_CENTER);
        adw_action_row_add_suffix (
          ADW_ACTION_ROW (row), GTK_WIDGET (s));
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));

        row = ADW_ACTION_ROW (adw_combo_row_new ());
        adw_preferences_row_set_title (
          ADW_PREFERENCES_ROW (row), _ ("Curve"));
        GtkStringList * slist = gtk_string_list_new (NULL);
        for (CurveAlgorithm i = CURVE_ALGORITHM_EXPONENT;
             i < NUM_CURVE_ALGORITHMS; i++)
          {
            CurveAlgorithm algo = i;
            char           buf[600];
            curve_algorithm_get_localized_name (algo, buf);
            gtk_string_list_append (slist, buf);
          }
        adw_combo_row_set_model (
          ADW_COMBO_ROW (row), G_LIST_MODEL (slist));
        adw_combo_row_set_selected (
          ADW_COMBO_ROW (row),
          (guint) g_settings_get_enum (
            self->settings, "curve-algo"));
        g_settings_bind_with_mapping (
          self->settings, "curve-algo", row, "selected",
          G_SETTINGS_BIND_DEFAULT,
          (GSettingsBindGetMapping)
            curve_algorithm_get_g_settings_mapping,
          (GSettingsBindSetMapping)
            curve_algorithm_set_g_settings_mapping,
          NULL, NULL);
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));

        row = ADW_ACTION_ROW (adw_action_row_new ());
        adw_preferences_row_set_title (
          ADW_PREFERENCES_ROW (row), _ ("Curviness %"));
        GtkSpinButton * spin_btn = GTK_SPIN_BUTTON (
          gtk_spin_button_new_with_range (0, 100, 1));
        gtk_spin_button_set_digits (spin_btn, 0);
        gtk_spin_button_set_value (
          spin_btn,
          g_settings_get_int (self->settings, "curviness"));
        g_settings_bind (
          self->settings, "curviness", spin_btn, "value",
          G_SETTINGS_BIND_DEFAULT);
        gtk_widget_set_valign (
          GTK_WIDGET (spin_btn), GTK_ALIGN_CENTER);
        adw_action_row_add_suffix (
          ADW_ACTION_ROW (row), GTK_WIDGET (spin_btn));
        adw_preferences_group_add (pgroup, GTK_WIDGET (row));

        row = ADW_ACTION_ROW (adw_action_row_new ());
        adw_preferences_row_set_title (
          ADW_PREFERENCES_ROW (row), _ ("Time"));
        spin_btn = GTK_SPIN_BUTTON (
          gtk_spin_button_new_with_range (1, 200, 1));
        gtk_spin_button_set_digits (spin_btn, 0);
        gtk_spin_button_set_value (
          spin_btn,
          g_settings_get_double (self->settings, "total-time"));
        g_settings_bind (
          self->settings, "total-time", spin_btn, "value",
          G_SETTINGS_BIND_DEFAULT);
        gtk_widget_set_valign (
          GTK_WIDGET (spin_btn), GTK_ALIGN_CENTER);
        adw_action_row_add_suffix (
          ADW_ACTION_ROW (row), GTK_WIDGET (spin_btn));
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
  ArrangerSelections * sel =
    (ArrangerSelections *) MA_SELECTIONS;
  if (!arranger_selections_has_any (sel))
    {
      g_message ("no selections, doing nothing");
      return;
    }

  MidiFunctionOpts opts;
  midi_function_dialog_widget_get_opts (self, &opts);

  GError * err = NULL;
  bool     ret =
    arranger_selections_action_perform_edit_midi_function (
      sel, self->type, opts, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s", _ ("Failed to apply MIDI function"));
    }

  gtk_window_close (GTK_WINDOW (self));
}

/**
 * Creates a MIDI function dialog for the given type and
 * pre-fills the values from GSettings.
 */
MidiFunctionDialogWidget *
midi_function_dialog_widget_new (
  GtkWindow *      parent,
  MidiFunctionType type)
{
  MidiFunctionDialogWidget * self =
    g_object_new (MIDI_FUNCTION_DIALOG_WIDGET_TYPE, NULL);

  self->type = type;

  gtk_window_set_title (
    GTK_WINDOW (self),
    _ (midi_function_type_to_string (type)));

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
midi_function_dialog_widget_class_init (
  MidiFunctionDialogWidgetClass * _klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}

static void
midi_function_dialog_widget_init (
  MidiFunctionDialogWidget * self)
{
  gtk_window_set_icon_name (GTK_WINDOW (self), "zrythm");
  gtk_window_set_modal (GTK_WINDOW (self), true);

  self->vbox =
    GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  adw_window_set_content (
    ADW_WINDOW (self), GTK_WIDGET (self->vbox));

  self->header = ADW_HEADER_BAR (adw_header_bar_new ());
  self->ok_btn =
    GTK_BUTTON (gtk_button_new_with_mnemonic (_ ("_Apply")));
  g_signal_connect (
    G_OBJECT (self->ok_btn), "clicked",
    G_CALLBACK (on_ok_clicked), self);
  gtk_widget_add_css_class (
    GTK_WIDGET (self->ok_btn), "suggested-action");
  adw_header_bar_pack_end (
    self->header, GTK_WIDGET (self->ok_btn));
  self->cancel_btn =
    GTK_BUTTON (gtk_button_new_with_mnemonic (_ ("_Cancel")));
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->cancel_btn), "window.close");
  adw_header_bar_pack_start (
    self->header, GTK_WIDGET (self->cancel_btn));
  gtk_box_append (self->vbox, GTK_WIDGET (self->header));

  self->page =
    ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  gtk_box_append (self->vbox, GTK_WIDGET (self->page));

  /* close on escape */
  GtkShortcutTrigger * trigger =
    gtk_shortcut_trigger_parse_string ("Escape|<Control>w");
  GtkShortcutAction * action =
    gtk_named_action_new ("window.close");
  GtkShortcut * esc_shortcut =
    gtk_shortcut_new (trigger, action);
  GtkShortcutController * sc =
    GTK_SHORTCUT_CONTROLLER (gtk_shortcut_controller_new ());
  gtk_shortcut_controller_add_shortcut (sc, esc_shortcut);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (sc));
  /* note: can also use gtk_widget_class_add_binding_action (widget_class, GDK_KEY_Escape, 0, "window.close", NULL); */
}
