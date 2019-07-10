/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/channel.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/piano_roll_key.h"
#include "gui/widgets/piano_roll_key_label.h"
#include "gui/widgets/ruler.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (PianoRollWidget,
               piano_roll_widget,
               GTK_TYPE_BOX)

#define DRUM_MODE (self->piano_roll->drum_mode)
#define DEFAULT_PX_PER_KEY 8

static void
on_midi_modifier_changed (
  GtkComboBox *widget,
  PianoRollWidget * self)
{
  piano_roll_set_midi_modifier (
    PIANO_ROLL,
    gtk_combo_box_get_active (widget));
}

/**
 * Links scroll windows after all widgets have been
 * initialized.
 */
static void
link_scrolls (
  PianoRollWidget * self)
{
  /* link note keys v scroll to arranger v scroll */
  if (self->piano_roll_keys_scroll)
    {
      gtk_scrolled_window_set_vadjustment (
        self->piano_roll_keys_scroll,
        gtk_scrolled_window_get_vadjustment (
          GTK_SCROLLED_WINDOW (
            self->arranger_scroll)));
    }

  /* link arranger h scroll to timeline h scroll */
  /*if (self->arranger_scroll)*/
    /*{*/
      /*g_warn_if_fail (MW_CENTER_DOCK);*/
      /*g_warn_if_fail (MW_CENTER_DOCK->timeline_scroll);*/
      /*gtk_scrolled_window_set_hadjustment (*/
        /*self->arranger_scroll,*/
        /*gtk_scrolled_window_get_hadjustment (*/
          /*GTK_SCROLLED_WINDOW (*/
            /*MW_CENTER_DOCK->timeline_scroll)));*/
    /*}*/

  /* link ruler h scroll to arranger h scroll */
  if (self->midi_ruler_scroll)
    {
      gtk_scrolled_window_set_hadjustment (
        self->midi_ruler_scroll,
        gtk_scrolled_window_get_hadjustment (
          GTK_SCROLLED_WINDOW (
            self->arranger_scroll)));
    }

  /* link modifier arranger h scroll to arranger h scroll */
  if (self->modifier_arranger_scroll)
    {
      gtk_scrolled_window_set_hadjustment (
        self->modifier_arranger_scroll,
        gtk_scrolled_window_get_hadjustment (
          GTK_SCROLLED_WINDOW (
            self->arranger_scroll)));
    }

}

/**
 * Refresh the labels only (for highlighting).
 *
 * @param hard_refresh Removes and radds the labels,
 *   otherwise just calls refresh on them.
 */
void
piano_roll_widget_refresh_labels (
  PianoRollWidget * self,
  int               hard_refresh)
{
  for (int i = 0; i < 128; i++)
    {
      if (GTK_IS_WIDGET (
            self->piano_roll_key_labels[i]))
        piano_roll_key_label_widget_refresh (
          self->piano_roll_key_labels[i]);
    }
}

void
piano_roll_widget_refresh (
  PianoRollWidget * self)
{
  self->px_per_key =
    DEFAULT_PX_PER_KEY *
    self->piano_roll->notes_zoom;
  self->total_key_px =
    self->px_per_key * 128;

  /* readd the notes */
  z_gtk_container_destroy_all_children (
    GTK_CONTAINER (self->piano_roll_keys_box));

  MidiNoteDescriptor * descr;
  for (int i = 0; i < 128; i++)
    {
      if (DRUM_MODE)
        descr = &self->piano_roll->
          drum_descriptors[i];
      else
        descr = &self->piano_roll->
          piano_descriptors[i];

      /* skip invisible notes */
      if (!descr->visible)
        continue;

      GtkBox * box =
        GTK_BOX (
          gtk_box_new (GTK_ORIENTATION_HORIZONTAL,
                       0));
      gtk_widget_set_visible (
        GTK_WIDGET (box), 1);
      /* add thin line on the bottom of each note */
      if (i != 127)
        z_gtk_widget_add_style_class (
          GTK_WIDGET (box), "piano_roll_key_box");

      /* add label */
      PianoRollKeyLabelWidget * lbl =
        piano_roll_key_label_widget_new (descr);
      piano_roll_key_label_widget_refresh (
        lbl);
      gtk_box_pack_start (
        box, GTK_WIDGET (lbl),
        1, 1, 0);
      gtk_widget_set_size_request (
        GTK_WIDGET (lbl),
        -1, self->px_per_key);
      self->piano_roll_key_labels[i] = lbl;

      if (!DRUM_MODE)
        {
          /* add label */
          PianoRollKeyWidget * key =
            piano_roll_key_widget_new (descr);
          gtk_box_pack_end (
            box, GTK_WIDGET (key),
            0, 1, 0);
          self->piano_roll_keys[i] = key;
        }

      gtk_box_pack_start (
        self->piano_roll_keys_box,
        GTK_WIDGET (box),
        0, 0, 0);
    }

  midi_arranger_widget_refresh_size (
    MIDI_ARRANGER);

  /* relink scrolls */
  link_scrolls (self);

  /* setup combo box */
  gtk_combo_box_set_active (
    GTK_COMBO_BOX (
      self->midi_modifier_chooser),
    PIANO_ROLL->midi_modifier);
  g_message ("SET TO %d", PIANO_ROLL->midi_modifier);
}

void
piano_roll_widget_setup (
  PianoRollWidget * self,
  PianoRoll *       pr)
{
  g_warn_if_fail (pr);
  self->piano_roll = pr;

  self->px_per_key =
    DEFAULT_PX_PER_KEY * pr->notes_zoom;
  self->total_key_px =
    self->px_per_key * 128;

  if (self->arranger)
    {
      arranger_widget_setup (
        Z_ARRANGER_WIDGET (self->arranger),
        &PROJECT->snap_grid_midi);
      gtk_widget_show_all (
        GTK_WIDGET (self->arranger));
    }
  if (self->modifier_arranger)
    {
      arranger_widget_setup (
        Z_ARRANGER_WIDGET (self->modifier_arranger),
        &PROJECT->snap_grid_midi);
      gtk_widget_show_all (
        GTK_WIDGET (self->modifier_arranger));
    }

  piano_roll_widget_refresh (self);
}

static void
piano_roll_widget_init (PianoRollWidget * self)
{
  g_type_ensure (COLOR_AREA_WIDGET_TYPE);
  g_type_ensure (MIDI_RULER_WIDGET_TYPE);
  g_type_ensure (MIDI_ARRANGER_WIDGET_TYPE);
  g_type_ensure (MIDI_MODIFIER_ARRANGER_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_label_set_text (self->midi_name_label,
                      _("Select a region..."));

  GdkRGBA * color = calloc (1, sizeof (GdkRGBA));
  gdk_rgba_parse (color, "gray");
  color_area_widget_set_color (
    self->color_bar, color);

  /* setup signals */
  g_signal_connect (
    G_OBJECT(self->midi_modifier_chooser),
    "changed",
    G_CALLBACK (on_midi_modifier_changed),  self);
}

static void
piano_roll_widget_class_init (
  PianoRollWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass,
    "piano_roll.ui");

  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    color_bar);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    midi_bot_toolbar);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    midi_name_label);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    midi_modifier_chooser);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    midi_controls_above_notes_box);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    midi_ruler_scroll);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    midi_ruler_viewport);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    ruler);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    piano_roll_keys_scroll);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    piano_roll_keys_viewport);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    piano_roll_keys_box);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    midi_arranger_velocity_paned);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    arranger_scroll);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    arranger_viewport);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    arranger);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    toggle_notation);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    modifier_arranger_scroll);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    modifier_arranger_viewport);
  gtk_widget_class_bind_template_child (
    klass,
    PianoRollWidget,
    modifier_arranger);
}
