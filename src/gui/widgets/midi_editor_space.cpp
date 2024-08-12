// SPDX-FileCopyrightText: © 2019, 2021-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/piano_roll.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_wrapper.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/piano_roll_keys.h"
#include "gui/widgets/velocity_settings.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (MidiEditorSpaceWidget, midi_editor_space_widget, GTK_TYPE_WIDGET)

static void
on_midi_modifier_changed (GtkComboBox * widget, MidiEditorSpaceWidget * self)
{
  PIANO_ROLL->set_midi_modifier (
    ENUM_INT_TO_VALUE (MidiModifier, gtk_combo_box_get_active (widget)));
}

static int
on_scroll (
  GtkEventControllerScroll * scroll_controller,
  gdouble                    dx,
  gdouble                    dy,
  gpointer                   user_data)
{
  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (scroll_controller));

  if (state & GDK_CONTROL_MASK && state & GDK_SHIFT_MASK)
    {
      midi_arranger_handle_vertical_zoom_scroll (
        MW_MIDI_ARRANGER, scroll_controller, dy);
    }

  return TRUE;
}

/**
 * To be used as a source func when first showing a MIDI
 * region.
 */
gboolean
midi_editor_space_widget_scroll_to_middle (MidiEditorSpaceWidget * self)
{
  /* scroll to the middle on first show */
  if (!self->scrolled_on_first_refresh)
    {
      GtkAdjustment * adj =
        gtk_scrolled_window_get_vadjustment (self->piano_roll_keys_scroll);
      int adj_upper = (int) gtk_adjustment_get_upper (adj);
      if (adj_upper > 1)
        {
          PIANO_ROLL->set_scroll_start_y (adj_upper / 2, F_VALIDATE);
          self->scrolled_on_first_refresh = true;
          return G_SOURCE_REMOVE;
        }
      return G_SOURCE_CONTINUE;
    }
  return G_SOURCE_REMOVE;
}

void
midi_editor_space_widget_refresh (MidiEditorSpaceWidget * self)
{
  piano_roll_keys_widget_refresh (self->piano_roll_keys);

  /* setup combo box */
  gtk_combo_box_set_active (
    GTK_COMBO_BOX (self->midi_modifier_chooser),
    ENUM_VALUE_TO_INT (PIANO_ROLL->midi_modifier_));
}

void
midi_editor_space_widget_update_size_group (
  MidiEditorSpaceWidget * self,
  int                     visible)
{
  clip_editor_inner_widget_add_to_left_of_ruler_sizegroup (
    MW_CLIP_EDITOR_INNER, GTK_WIDGET (self->midi_vel_chooser_box), visible);
  clip_editor_inner_widget_add_to_left_of_ruler_sizegroup (
    MW_CLIP_EDITOR_INNER, GTK_WIDGET (self->midi_notes_box), visible);
}

/**
 * Updates the scroll adjustment.
 */
void
midi_editor_space_widget_set_piano_keys_scroll_start_y (
  MidiEditorSpaceWidget * self,
  int                     y)
{
  GtkAdjustment * hadj =
    gtk_scrolled_window_get_vadjustment (self->piano_roll_keys_scroll);
  if (!math_doubles_equal ((double) y, gtk_adjustment_get_value (hadj)))
    {
      gtk_adjustment_set_value (hadj, (double) y);
    }
}

static void
on_piano_keys_scroll_hadj_changed (
  GtkAdjustment *         adj,
  MidiEditorSpaceWidget * self)
{
  PIANO_ROLL->set_scroll_start_y (
    (int) gtk_adjustment_get_value (adj), F_VALIDATE);
}

void
midi_editor_space_widget_setup (MidiEditorSpaceWidget * self)
{
  if (self->arranger_wrapper)
    {
      arranger_wrapper_widget_setup (
        self->arranger_wrapper, ArrangerWidgetType::ARRANGER_WIDGET_TYPE_MIDI,
        SNAP_GRID_EDITOR);
    }
  if (self->modifier_arranger)
    {
      arranger_widget_setup (
        Z_ARRANGER_WIDGET (self->modifier_arranger),
        ArrangerWidgetType::ARRANGER_WIDGET_TYPE_MIDI_MODIFIER,
        SNAP_GRID_EDITOR);
    }

  piano_roll_keys_widget_setup (self->piano_roll_keys);

  /* add a signal handler to update the editor settings on
   * scroll */
  GtkAdjustment * vadj =
    gtk_scrolled_window_get_vadjustment (self->piano_roll_keys_scroll);
  g_signal_connect (
    vadj, "value-changed", G_CALLBACK (on_piano_keys_scroll_hadj_changed), self);

  midi_editor_space_widget_refresh (self);
}

static gboolean
midi_editor_space_tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  gpointer        user_data)
{
  MidiEditorSpaceWidget * self = Z_MIDI_EDITOR_SPACE_WIDGET (user_data);

  GtkAdjustment * vadj =
    gtk_scrolled_window_get_vadjustment (self->piano_roll_keys_scroll);
  gtk_adjustment_set_value (vadj, PIANO_ROLL->scroll_start_y_);

  return G_SOURCE_CONTINUE;
}

static void
dispose (MidiEditorSpaceWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->midi_arranger_velocity_paned));

  G_OBJECT_CLASS (midi_editor_space_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
midi_editor_space_widget_init (MidiEditorSpaceWidget * self)
{
  g_type_ensure (PIANO_ROLL_KEYS_WIDGET_TYPE);
  g_type_ensure (VELOCITY_SETTINGS_WIDGET_TYPE);
  g_type_ensure (ARRANGER_WRAPPER_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_paned_set_resize_start_child (self->midi_arranger_velocity_paned, true);
  gtk_paned_set_resize_end_child (self->midi_arranger_velocity_paned, true);
  gtk_paned_set_shrink_start_child (self->midi_arranger_velocity_paned, false);
  gtk_paned_set_shrink_end_child (self->midi_arranger_velocity_paned, false);

  self->arranger_wrapper->child->type =
    ArrangerWidgetType::ARRANGER_WIDGET_TYPE_MIDI;
  self->modifier_arranger->type =
    ArrangerWidgetType::ARRANGER_WIDGET_TYPE_MIDI_MODIFIER;

  /* doesn't work */
  self->arranger_and_keys_vsize_group =
    gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (
    self->arranger_and_keys_vsize_group,
    GTK_WIDGET (self->arranger_wrapper->child));
  gtk_size_group_add_widget (
    self->arranger_and_keys_vsize_group,
    GTK_WIDGET (self->piano_roll_keys_scroll));

  /* above doesn't work so set vexpand instead */
  gtk_widget_set_vexpand (GTK_WIDGET (self->arranger_wrapper->child), true);

  /* also hexpand the modifier arranger TODO use a size
   * group */
  gtk_widget_set_hexpand (GTK_WIDGET (self->modifier_arranger), true);

  /* setup signals */
  g_signal_connect (
    G_OBJECT (self->midi_modifier_chooser), "changed",
    G_CALLBACK (on_midi_modifier_changed), self);
  /*g_signal_connect (*/
  /*G_OBJECT (self->piano_roll_keys_box),*/
  /*"size-allocate",*/
  /*G_CALLBACK (on_keys_box_size_allocate), self);*/

  GtkEventControllerScroll * scroll_controller = GTK_EVENT_CONTROLLER_SCROLL (
    gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES));
  g_signal_connect (
    G_OBJECT (scroll_controller), "scroll", G_CALLBACK (on_scroll), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (scroll_controller));

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), midi_editor_space_tick_cb, self, nullptr);
}

static void
midi_editor_space_widget_class_init (MidiEditorSpaceWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "midi_editor_space.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, MidiEditorSpaceWidget, x)

  BIND_CHILD (midi_modifier_chooser);
  BIND_CHILD (piano_roll_keys_scroll);
  BIND_CHILD (piano_roll_keys);
  BIND_CHILD (midi_arranger_velocity_paned);
  BIND_CHILD (arranger_wrapper);
  BIND_CHILD (velocity_settings);
  BIND_CHILD (modifier_arranger);
  BIND_CHILD (midi_notes_box);
  BIND_CHILD (midi_vel_chooser_box);

#undef BIND_CHILD

  gtk_widget_class_set_layout_manager_type (klass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
