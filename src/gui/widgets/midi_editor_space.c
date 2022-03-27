/*
 * Copyright (C) 2019, 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/chord_track.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/piano_roll_keys.h"
#include "gui/widgets/ruler.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  MidiEditorSpaceWidget, midi_editor_space_widget,
  GTK_TYPE_WIDGET)

static void
on_midi_modifier_changed (
  GtkComboBox *widget,
  MidiEditorSpaceWidget * self)
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
  MidiEditorSpaceWidget * self)
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

  /* link ruler h scroll to arranger h scroll */
  if (MW_CLIP_EDITOR_INNER->ruler_scroll)
    {
      gtk_scrolled_window_set_hadjustment (
        self->arranger_scroll,
        gtk_scrolled_window_get_hadjustment (
          MW_CLIP_EDITOR_INNER->ruler_scroll));
    }

  /* link modifier arranger h scroll to arranger h
   * scroll */
  if (self->modifier_arranger_scroll)
    {
      gtk_scrolled_window_set_hadjustment (
        self->modifier_arranger_scroll,
        gtk_scrolled_window_get_hadjustment (
          MW_CLIP_EDITOR_INNER->ruler_scroll));
    }

  /* set scrollbar adjustments */
  gtk_scrollbar_set_adjustment (
    self->arranger_hscrollbar,
    gtk_scrolled_window_get_hadjustment (
      MW_CLIP_EDITOR_INNER->ruler_scroll));
  gtk_scrollbar_set_adjustment (
    self->arranger_vscrollbar,
    gtk_scrolled_window_get_vadjustment (
      GTK_SCROLLED_WINDOW (self->arranger_scroll)));
}

static int
on_scroll (
  GtkEventControllerScroll * scroll_controller,
  gdouble                    dx,
  gdouble                    dy,
  gpointer                   user_data)
{
  GdkModifierType state =
    gtk_event_controller_get_current_event_state (
      GTK_EVENT_CONTROLLER (scroll_controller));

  if (state & GDK_CONTROL_MASK &&
      state & GDK_SHIFT_MASK)
    {
      midi_arranger_handle_vertical_zoom_scroll (
        MW_MIDI_ARRANGER, scroll_controller, dy);
    }

  return TRUE;
}

/**
 * Source function that keeps trying to scroll to the
 * mid note until successful.
 */
static gboolean
midi_editor_space_tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  gpointer        user_data)
{
  MidiEditorSpaceWidget * self =
    Z_MIDI_EDITOR_SPACE_WIDGET (user_data);
  GtkAdjustment * adj =
    gtk_scrolled_window_get_vadjustment (
      self->arranger_scroll);
  double lower =
    gtk_adjustment_get_lower (adj);
  double upper =
    gtk_adjustment_get_upper (adj);

  /* keep trying until the scrolled window has
   * a proper size */
  if (upper > 0)
    {
      gtk_adjustment_set_value (
        adj,
        lower + (upper - lower) / 2.0);

      return G_SOURCE_REMOVE;
    }

  return G_SOURCE_CONTINUE;
}

void
midi_editor_space_widget_refresh (
  MidiEditorSpaceWidget * self)
{
  piano_roll_keys_widget_refresh (
    self->piano_roll_keys);

  /* relink scrolls (why?) */
  link_scrolls (self);

  /* setup combo box */
  gtk_combo_box_set_active (
    GTK_COMBO_BOX (
      self->midi_modifier_chooser),
    PIANO_ROLL->midi_modifier);
}

void
midi_editor_space_widget_update_size_group (
  MidiEditorSpaceWidget * self,
  int                     visible)
{
  clip_editor_inner_widget_add_to_left_of_ruler_sizegroup (
    MW_CLIP_EDITOR_INNER,
    GTK_WIDGET (self->midi_vel_chooser_box),
    visible);
  clip_editor_inner_widget_add_to_left_of_ruler_sizegroup (
    MW_CLIP_EDITOR_INNER,
    GTK_WIDGET (self->midi_notes_box),
    visible);
}

void
midi_editor_space_widget_setup (
  MidiEditorSpaceWidget * self)
{
  if (self->arranger)
    {
      arranger_widget_setup (
        Z_ARRANGER_WIDGET (self->arranger),
        ARRANGER_WIDGET_TYPE_MIDI,
        SNAP_GRID_EDITOR);
    }
  if (self->modifier_arranger)
    {
      arranger_widget_setup (
        Z_ARRANGER_WIDGET (self->modifier_arranger),
        ARRANGER_WIDGET_TYPE_MIDI_MODIFIER,
        SNAP_GRID_EDITOR);
    }

  piano_roll_keys_widget_setup (
    self->piano_roll_keys);

  midi_editor_space_widget_refresh (self);
}

static void
dispose (
  MidiEditorSpaceWidget * self)
{
  gtk_widget_unparent (
    GTK_WIDGET (self->midi_arranger_velocity_paned));

  G_OBJECT_CLASS (
    midi_editor_space_widget_parent_class)->
      dispose (G_OBJECT (self));
}

static void
midi_editor_space_widget_init (
  MidiEditorSpaceWidget * self)
{
  g_type_ensure (PIANO_ROLL_KEYS_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  /* make hscrollbar invisible until GTK bug
   * 4478 is fixed */
  gtk_widget_set_visible (
    GTK_WIDGET (self->arranger_hscrollbar), false);

  gtk_paned_set_resize_start_child (
    self->midi_arranger_velocity_paned, true);
  gtk_paned_set_resize_end_child (
    self->midi_arranger_velocity_paned, true);
  gtk_paned_set_shrink_start_child (
    self->midi_arranger_velocity_paned, false);
  gtk_paned_set_shrink_end_child (
    self->midi_arranger_velocity_paned, false);

  self->arranger->type =
    ARRANGER_WIDGET_TYPE_MIDI;
  self->modifier_arranger->type =
    ARRANGER_WIDGET_TYPE_MIDI_MODIFIER;

  self->arranger_and_keys_vsize_group =
    gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (
    self->arranger_and_keys_vsize_group,
    GTK_WIDGET (self->arranger));
  gtk_size_group_add_widget (
    self->arranger_and_keys_vsize_group,
    GTK_WIDGET (self->piano_roll_keys));

  /* setup signals */
  g_signal_connect (
    G_OBJECT(self->midi_modifier_chooser),
    "changed",
    G_CALLBACK (on_midi_modifier_changed),  self);
  /*g_signal_connect (*/
    /*G_OBJECT (self->piano_roll_keys_box),*/
    /*"size-allocate",*/
    /*G_CALLBACK (on_keys_box_size_allocate), self);*/

  GtkEventControllerScroll * scroll_controller =
    GTK_EVENT_CONTROLLER_SCROLL (
      gtk_event_controller_scroll_new (
        GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES));
  g_signal_connect (
    G_OBJECT (scroll_controller), "scroll",
    G_CALLBACK (on_scroll), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (scroll_controller));

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), midi_editor_space_tick_cb,
    self, NULL);
}

static void
midi_editor_space_widget_class_init (
  MidiEditorSpaceWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass,
    "midi_editor_space.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, MidiEditorSpaceWidget, x)

  BIND_CHILD (midi_modifier_chooser);
  BIND_CHILD (piano_roll_keys_scroll);
  BIND_CHILD (piano_roll_keys);
  BIND_CHILD (midi_arranger_velocity_paned);
  BIND_CHILD (arranger_scroll);
  BIND_CHILD (arranger);
  BIND_CHILD (modifier_arranger_scroll);
  BIND_CHILD (modifier_arranger);
  BIND_CHILD (arranger_hscrollbar);
  BIND_CHILD (arranger_vscrollbar);
  BIND_CHILD (midi_notes_box);
  BIND_CHILD (midi_vel_chooser_box);

#undef BIND_CHILD

  gtk_widget_class_set_layout_manager_type (
    klass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass =
    G_OBJECT_CLASS (klass);
  oklass->dispose =
    (GObjectFinalizeFunc) dispose;
}
