/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/snap_grid.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/snap_grid_popover.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  SnapGridPopoverWidget,
  snap_grid_popover_widget,
  GTK_TYPE_POPOVER)

static void
on_closed (
  SnapGridPopoverWidget * self,
  gpointer                user_data)
{
  snap_grid_widget_refresh (self->owner);
}

static void
block_or_unblock_all_handlers (
  SnapGridPopoverWidget * self,
  bool                    block)
{
  if (block)
    {
      g_signal_handler_block (
        self->snap_triplet_toggle,
        self->snap_triplet_toggle_handler);
      g_signal_handler_block (
        self->snap_dotted_toggle,
        self->snap_dotted_toggle_handler);
      g_signal_handler_block (
        self->snap_adaptive,
        self->snap_adaptive_handler);
      g_signal_handler_block (
        self->default_triplet_toggle,
        self->default_triplet_toggle_handler);
      g_signal_handler_block (
        self->default_dotted_toggle,
        self->default_dotted_toggle_handler);
      g_signal_handler_block (
        self->default_adaptive,
        self->default_adaptive_handler);
      g_signal_handler_block (
        self->link_toggle, self->link_handler);
      g_signal_handler_block (
        self->last_object_toggle,
        self->last_object_handler);
    }
  else
    {
      g_signal_handler_unblock (
        self->snap_triplet_toggle,
        self->snap_triplet_toggle_handler);
      g_signal_handler_unblock (
        self->snap_dotted_toggle,
        self->snap_dotted_toggle_handler);
      g_signal_handler_unblock (
        self->snap_adaptive,
        self->snap_adaptive_handler);
      g_signal_handler_unblock (
        self->default_triplet_toggle,
        self->default_triplet_toggle_handler);
      g_signal_handler_unblock (
        self->default_dotted_toggle,
        self->default_dotted_toggle_handler);
      g_signal_handler_unblock (
        self->default_adaptive,
        self->default_adaptive_handler);
      g_signal_handler_unblock (
        self->link_toggle, self->link_handler);
      g_signal_handler_unblock (
        self->last_object_toggle,
        self->last_object_handler);
    }
}

static void
refresh_link_and_last_object (
  SnapGridPopoverWidget * self)
{
  block_or_unblock_all_handlers (self, true);

  /* whether default controls should be visible */
  bool visible =
    self->owner->snap_grid->length_type
    == NOTE_LENGTH_CUSTOM;
  gtk_toggle_button_set_active (
    self->link_toggle,
    self->owner->snap_grid->length_type
      == NOTE_LENGTH_LINK);
  gtk_toggle_button_set_active (
    self->last_object_toggle,
    self->owner->snap_grid->length_type
      == NOTE_LENGTH_LAST_OBJECT);
  gtk_widget_set_visible (
    GTK_WIDGET (self->default_length_box), visible);
  gtk_widget_set_visible (
    GTK_WIDGET (self->default_triplet_toggle),
    visible);
  gtk_widget_set_visible (
    GTK_WIDGET (self->default_dotted_toggle),
    visible);
  gtk_widget_set_visible (
    GTK_WIDGET (self->default_adaptive), visible);

  block_or_unblock_all_handlers (self, false);
}

static void
on_linked_toggled (
  GtkToggleButton *       btn,
  SnapGridPopoverWidget * self)
{
  bool link = gtk_toggle_button_get_active (btn);
  if (link)
    {
      self->owner->snap_grid->length_type =
        NOTE_LENGTH_LINK;
    }
  else
    {
      self->owner->snap_grid->length_type =
        NOTE_LENGTH_CUSTOM;
    }

  refresh_link_and_last_object (self);
}

static void
on_last_object_toggled (
  GtkToggleButton *       btn,
  SnapGridPopoverWidget * self)
{
  bool last_object =
    gtk_toggle_button_get_active (btn);
  if (last_object)
    {
      self->owner->snap_grid->length_type =
        NOTE_LENGTH_LAST_OBJECT;
    }
  else
    {
      self->owner->snap_grid->length_type =
        NOTE_LENGTH_CUSTOM;
    }

  refresh_link_and_last_object (self);
}

static void
on_type_toggled (
  GtkWidget *             btn,
  SnapGridPopoverWidget * self)
{
  block_or_unblock_all_handlers (self, true);

  SnapGrid * sg = self->owner->snap_grid;
  bool       active = false;
  if (GTK_IS_TOGGLE_BUTTON (btn))
    active = gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (btn));
  else if (GTK_IS_CHECK_BUTTON (btn))
    active = gtk_check_button_get_active (
      GTK_CHECK_BUTTON (btn));

  if (btn == GTK_WIDGET (self->snap_triplet_toggle))
    {
      if (active)
        {
          sg->snap_note_type = NOTE_TYPE_TRIPLET;
          gtk_toggle_button_set_active (
            self->snap_dotted_toggle, false);
        }
      else
        {
          sg->snap_note_type = NOTE_TYPE_NORMAL;
        }
    }
  else if (btn == GTK_WIDGET (self->snap_dotted_toggle))
    {
      if (active)
        {
          sg->snap_note_type = NOTE_TYPE_DOTTED;
          gtk_toggle_button_set_active (
            self->snap_triplet_toggle, false);
        }
      else
        {
          sg->snap_note_type = NOTE_TYPE_NORMAL;
        }
    }
  else if (
    btn == GTK_WIDGET (self->default_triplet_toggle))
    {
      if (active)
        {
          sg->default_note_type = NOTE_TYPE_TRIPLET;
          gtk_toggle_button_set_active (
            self->default_dotted_toggle, false);
        }
      else
        {
          sg->default_note_type = NOTE_TYPE_NORMAL;
        }
    }
  else if (
    btn == GTK_WIDGET (self->default_dotted_toggle))
    {
      if (active)
        {
          sg->default_note_type = NOTE_TYPE_DOTTED;
          gtk_toggle_button_set_active (
            self->default_triplet_toggle, false);
        }
      else
        {
          sg->default_note_type = NOTE_TYPE_NORMAL;
        }
    }
  else if (btn == GTK_WIDGET (self->snap_adaptive))
    {
      sg->snap_adaptive = active;
      gtk_widget_set_sensitive (
        GTK_WIDGET (self->snap_length_dm), !active);
    }
  else if (btn == GTK_WIDGET (self->default_adaptive))
    {
      sg->default_adaptive = active;
      gtk_widget_set_sensitive (
        GTK_WIDGET (self->default_length_dm),
        !active);
    }

  block_or_unblock_all_handlers (self, false);
}

/**
 * Creates a digital meter with the given type (bpm or position).
 */
SnapGridPopoverWidget *
snap_grid_popover_widget_new (SnapGridWidget * owner)
{
  SnapGridPopoverWidget * self = g_object_new (
    SNAP_GRID_POPOVER_WIDGET_TYPE, NULL);

  self->owner = owner;

  /* -- snap -- */
  self->snap_length_dm = digital_meter_widget_new (
    DIGITAL_METER_TYPE_NOTE_LENGTH,
    &owner->snap_grid->snap_note_length,
    &owner->snap_grid->snap_note_type,
    "note length");
  gtk_box_append (
    GTK_BOX (self->snap_length_box),
    GTK_WIDGET (self->snap_length_dm));
#if 0
  self->snap_type_dm =
    digital_meter_widget_new (
      DIGITAL_METER_TYPE_NOTE_TYPE,
      &owner->snap_grid->snap_note_length,
      &owner->snap_grid->snap_note_type,
      "note type");
  gtk_container_add (
    GTK_CONTAINER (self->snap_type_box),
    GTK_WIDGET (self->snap_type_dm));
#endif

  /* -- default -- */
  self->default_length_dm = digital_meter_widget_new (
    DIGITAL_METER_TYPE_NOTE_LENGTH,
    &owner->snap_grid->default_note_length,
    &owner->snap_grid->default_note_type,
    "note length");
  gtk_box_append (
    GTK_BOX (self->default_length_box),
    GTK_WIDGET (self->default_length_dm));
#if 0
  self->default_type_dm =
    digital_meter_widget_new (
      DIGITAL_METER_TYPE_NOTE_TYPE,
      &owner->snap_grid->default_note_length,
      &owner->snap_grid->default_note_type,
      "note type");
  gtk_container_add (
    GTK_CONTAINER (self->default_type_box),
    GTK_WIDGET (self->default_type_dm));
#endif

  self
    ->snap_triplet_toggle_handler = g_signal_connect (
    GTK_WIDGET (self->snap_triplet_toggle),
    "toggled", G_CALLBACK (on_type_toggled), self);
  self->snap_dotted_toggle_handler = g_signal_connect (
    GTK_WIDGET (self->snap_dotted_toggle),
    "toggled", G_CALLBACK (on_type_toggled), self);
  self->snap_adaptive_handler = g_signal_connect (
    GTK_WIDGET (self->snap_adaptive), "toggled",
    G_CALLBACK (on_type_toggled), self);
  self->default_triplet_toggle_handler =
    g_signal_connect (
      GTK_WIDGET (self->default_triplet_toggle),
      "toggled", G_CALLBACK (on_type_toggled), self);
  self->default_dotted_toggle_handler =
    g_signal_connect (
      GTK_WIDGET (self->default_dotted_toggle),
      "toggled", G_CALLBACK (on_type_toggled), self);
  self->default_adaptive_handler = g_signal_connect (
    GTK_WIDGET (self->default_adaptive), "toggled",
    G_CALLBACK (on_type_toggled), self);
  self->link_handler = g_signal_connect (
    GTK_WIDGET (self->link_toggle), "toggled",
    G_CALLBACK (on_linked_toggled), self);
  self->last_object_handler = g_signal_connect (
    GTK_WIDGET (self->last_object_toggle), "toggled",
    G_CALLBACK (on_last_object_toggled), self);

  refresh_link_and_last_object (self);

  return self;
}

static void
snap_grid_popover_widget_class_init (
  SnapGridPopoverWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "snap_grid_popover.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, SnapGridPopoverWidget, x)

  BIND_CHILD (snap_length_box);
  BIND_CHILD (snap_type_box);
  BIND_CHILD (snap_triplet_toggle);
  BIND_CHILD (snap_dotted_toggle);
  BIND_CHILD (snap_adaptive);
  BIND_CHILD (default_length_box);
  BIND_CHILD (default_type_box);
  BIND_CHILD (default_triplet_toggle);
  BIND_CHILD (default_dotted_toggle);
  BIND_CHILD (default_adaptive);
  BIND_CHILD (link_toggle);
  BIND_CHILD (last_object_toggle);

#undef BIND_CHILD

  gtk_widget_class_bind_template_callback (
    klass, on_closed);
}

static void
snap_grid_popover_widget_init (
  SnapGridPopoverWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
