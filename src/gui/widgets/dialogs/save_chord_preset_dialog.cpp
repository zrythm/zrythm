// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/dialogs/save_chord_preset_dialog.h"
#include "project.h"
#include "settings/chord_preset_pack_manager.h"
#include "utils/error.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  SaveChordPresetDialogWidget,
  save_chord_preset_dialog_widget,
  GTK_TYPE_DIALOG)

static void
on_response (GtkDialog * dialog, gint response_id, gpointer user_data)
{
  SaveChordPresetDialogWidget * self =
    Z_SAVE_CHORD_PRESET_DIALOG_WIDGET (user_data);

  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      const char * entered_name =
        gtk_editable_get_text (GTK_EDITABLE (self->preset_name_entry));
      WrappedObjectWithChangeSignal * wrapped_pack =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
          gtk_drop_down_get_selected_item (self->pack_dropdown));
      g_return_if_fail (wrapped_pack);
      g_return_if_fail (
        wrapped_pack->type
        == WrappedObjectType::WRAPPED_OBJECT_TYPE_CHORD_PSET_PACK);
      ChordPresetPack * pack = (ChordPresetPack *) wrapped_pack->obj;

      if (!entered_name || strlen (entered_name) < 1)
        {
          ui_show_error_message (
            _ ("Invalid Name"), _ ("Please enter a valid name."));
          return;
        }
      else if (chord_preset_pack_contains_name (pack, entered_name))
        {
          ui_show_message_printf (
            _ ("Name Unavailable"),
            _ ("Name '%s' is taken. Please enter "
               "a different name"),
            entered_name);
          return;
        }
      else
        {
          /* save */
          g_debug ("accept: %s, %s", pack->name, entered_name);
          ChordPreset * pset = chord_preset_new_from_name (entered_name);
          for (int i = 0; i < 12; i++)
            {
              ChordDescriptor * descr = CHORD_EDITOR->chords[i];
              pset->descr[i] = chord_descriptor_clone (descr);
            }
          chord_preset_pack_manager_add_preset (
            CHORD_PRESET_PACK_MANAGER, pack, pset, true);
          chord_preset_free (pset);

          EVENTS_PUSH (EventType::ET_CHORD_PRESET_ADDED, NULL);
        }
    }

  gtk_window_destroy (GTK_WINDOW (dialog));
}

static GtkDropDown *
generate_packs_dropdown (void)
{
  GListStore * store = g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);

  int num_packs =
    chord_preset_pack_manager_get_num_packs (CHORD_PRESET_PACK_MANAGER);
  for (int i = 0; i < num_packs; i++)
    {
      ChordPresetPack * pack =
        chord_preset_pack_manager_get_pack_at (CHORD_PRESET_PACK_MANAGER, i);
      if (pack->is_standard)
        continue;

      WrappedObjectWithChangeSignal * wrapped_pack =
        wrapped_object_with_change_signal_new (
          pack, WrappedObjectType::WRAPPED_OBJECT_TYPE_CHORD_PSET_PACK);

      g_list_store_append (store, wrapped_pack);
    }

  GtkExpression * expr = gtk_cclosure_expression_new (
    G_TYPE_STRING, NULL, 0, NULL,
    G_CALLBACK (wrapped_object_with_change_signal_get_display_name), NULL, NULL);
  GtkDropDown * dropdown =
    GTK_DROP_DOWN (gtk_drop_down_new (G_LIST_MODEL (store), expr));

  return dropdown;
}

/**
 * Creates a new save_chord_preset dialog.
 */
SaveChordPresetDialogWidget *
save_chord_preset_dialog_widget_new (GtkWindow * parent_window)
{
  SaveChordPresetDialogWidget * self = static_cast<
    SaveChordPresetDialogWidget *> (g_object_new (
    SAVE_CHORD_PRESET_DIALOG_WIDGET_TYPE, "title", _ ("Save Chord Preset"),
    "modal", true, "transient-for", parent_window, NULL));

  return self;
}

static void
save_chord_preset_dialog_widget_class_init (
  SaveChordPresetDialogWidgetClass * _klass)
{
}

static void
save_chord_preset_dialog_widget_init (SaveChordPresetDialogWidget * self)
{
  gtk_dialog_add_button (GTK_DIALOG (self), _ ("_Save"), GTK_RESPONSE_ACCEPT);
  gtk_dialog_add_button (GTK_DIALOG (self), _ ("_Cancel"), GTK_RESPONSE_REJECT);

  GtkBox * content_area =
    GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self)));

  GtkGrid *     grid = GTK_GRID (gtk_grid_new ());
  GtkLabel *    pack_lbl = GTK_LABEL (gtk_label_new (_ ("Pack")));
  GtkLabel *    preset_name_lbl = GTK_LABEL (gtk_label_new (_ ("Preset Name")));
  GtkDropDown * pack_dropdown = generate_packs_dropdown ();
  self->pack_dropdown = pack_dropdown;
  GtkEntry * preset_name_entry = GTK_ENTRY (gtk_entry_new ());
  self->preset_name_entry = preset_name_entry;
  gtk_grid_attach (grid, GTK_WIDGET (pack_lbl), 0, 0, 1, 1);
  gtk_grid_attach (grid, GTK_WIDGET (pack_dropdown), 1, 0, 1, 1);
  gtk_grid_attach (grid, GTK_WIDGET (preset_name_lbl), 0, 1, 1, 1);
  gtk_grid_attach (grid, GTK_WIDGET (preset_name_entry), 1, 1, 1, 1);
  gtk_grid_set_row_spacing (grid, 2);
  gtk_grid_set_column_spacing (grid, 2);
  z_gtk_widget_set_margin (GTK_WIDGET (grid), 4);
  gtk_box_append (content_area, GTK_WIDGET (grid));

  g_signal_connect (G_OBJECT (self), "response", G_CALLBACK (on_response), self);
}
