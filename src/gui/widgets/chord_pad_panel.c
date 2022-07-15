// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/chord_descriptor.h"
#include "audio/scale.h"
#include "gui/widgets/chord_pad.h"
#include "gui/widgets/chord_pad_panel.h"
#include "settings/chord_preset_pack_manager.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ChordPadPanelWidget,
  chord_pad_panel_widget,
  GTK_TYPE_GRID)

void
chord_pad_panel_widget_setup (ChordPadPanelWidget * self)
{
  for (int i = 0; i < 12; i++)
    {
      chord_pad_widget_refresh (self->chords[i], i);
    }
}

void
chord_pad_panel_widget_refresh_load_preset_menu (
  ChordPadPanelWidget * self)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  /* --- predefined --- */
  GMenu * predefined_menu = g_menu_new ();
  GMenu * scales_submenu = g_menu_new ();
  for (int i = 0; i < NUM_SCALES; i++)
    {
      MusicalScaleType type = (MusicalScaleType) i;

      /* ignore scales with unimplemented triads */
      const ChordType * triad_types =
        musical_scale_get_triad_types (type, true);
      if (triad_types[0] == CHORD_TYPE_NONE)
        continue;

      GMenu * scale_submenu = g_menu_new ();

      for (int j = 0; j < 12; j++)
        {
          char action[800];
          sprintf (
            action, "app.load-chord-preset-from-scale::%d,%d",
            i, j);
          menuitem = z_gtk_create_menu_item (
            chord_descriptor_note_to_string ((MusicalNote) j),
            "minuet-chords", action);
          g_menu_append_item (scale_submenu, menuitem);
        }
      g_menu_append_submenu (
        scales_submenu, musical_scale_type_to_string (type),
        G_MENU_MODEL (scale_submenu));
    }
  g_menu_append_submenu (
    predefined_menu, _ ("From scale"),
    G_MENU_MODEL (scales_submenu));
  g_menu_append_section (
    menu, _ ("Predefined"), G_MENU_MODEL (predefined_menu));

  /* --- packs --- */
  GMenu * packs_menu = g_menu_new ();
  int num_packs = chord_preset_pack_manager_get_num_packs (
    CHORD_PRESET_PACK_MANAGER);
  for (int i = 0; i < num_packs; i++)
    {
      ChordPresetPack * pack =
        chord_preset_pack_manager_get_pack_at (
          CHORD_PRESET_PACK_MANAGER, i);

      GMenu * pack_submenu = g_menu_new ();
      for (int j = 0; j < pack->num_presets; j++)
        {
          ChordPreset * pset = pack->presets[j];

          char action[800];
          sprintf (
            action, "app.load-chord-preset::%d,%d", i, j);
          menuitem = z_gtk_create_menu_item (
            pset->name, "minuet-chords", action);
          g_menu_append_item (pack_submenu, menuitem);
        }
      g_menu_append_submenu (
        packs_menu, pack->name, G_MENU_MODEL (pack_submenu));
    }
  g_menu_append_section (
    menu, _ ("Preset Packs"), G_MENU_MODEL (packs_menu));

  gtk_menu_button_set_menu_model (
    self->load_preset_btn, G_MENU_MODEL (menu));
}

void
chord_pad_panel_widget_refresh (ChordPadPanelWidget * self)
{
  g_debug ("refreshing chord pad...");

  chord_pad_panel_widget_setup (self);

  chord_pad_panel_widget_refresh_load_preset_menu (self);
}

ChordPadPanelWidget *
chord_pad_panel_widget_new (void)
{
  ChordPadPanelWidget * self =
    g_object_new (CHORD_PAD_PANEL_WIDGET_TYPE, NULL);

  return self;
}

static void
chord_pad_panel_widget_init (ChordPadPanelWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  /* save preset button */
  gtk_button_set_icon_name (
    self->save_preset_btn, "document-save-as");
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->save_preset_btn),
    "app.save-chord-preset");

  /* load preset menu button */
  gtk_menu_button_set_icon_name (
    self->load_preset_btn, "document-open");
  chord_pad_panel_widget_refresh_load_preset_menu (self);

  /* transpose actions */
  gtk_actionable_set_detailed_action_name (
    GTK_ACTIONABLE (self->transpose_up_btn),
    "app.transpose-chord-pad::up");
  gtk_actionable_set_detailed_action_name (
    GTK_ACTIONABLE (self->transpose_down_btn),
    "app.transpose-chord-pad::down");

  /* chord pads */
  for (int i = 0; i < 12; i++)
    {
      ChordPadWidget * chord = chord_pad_widget_new ();
      self->chords[i] = chord;
      gtk_grid_attach (
        self->chords_grid, GTK_WIDGET (chord), i % 6, i / 6,
        1, 1);
    }
}

static void
chord_pad_panel_widget_class_init (
  ChordPadPanelWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "chord_pad_panel.ui");

  gtk_widget_class_set_css_name (klass, "chord-pad");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ChordPadPanelWidget, x)

  BIND_CHILD (chords_grid);
  BIND_CHILD (save_preset_btn);
  BIND_CHILD (load_preset_btn);
  BIND_CHILD (transpose_up_btn);
  BIND_CHILD (transpose_down_btn);

#undef BIND_CHILD
}
