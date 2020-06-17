/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/engine.h"
#include "audio/track.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/editable_label.h"
#include "project.h"
#include "utils/ui.h"
#include "zrythm.h"

G_DEFINE_TYPE (EditableLabelWidget,
               editable_label_widget,
               GTK_TYPE_EVENT_BOX)

static void
on_entry_activated (
  GtkEntry *entry,
  EditableLabelWidget * self)
{
  (*self->setter) (
    self->object,
    gtk_entry_get_text (entry));
  gtk_widget_set_visible (
    GTK_WIDGET (self->popover), 0);
}

static void
on_popover_closed (
  GtkPopover * popover,
  EditableLabelWidget * self)
{
  gtk_widget_destroy (GTK_WIDGET (popover));

  if (self->is_temp)
    {
      g_object_unref (self);
    }
}

/**
 * Shows the popover.
 */
void
editable_label_widget_show_popover (
  EditableLabelWidget * self)
{
  self->popover =
    GTK_POPOVER (
      gtk_popover_new (GTK_WIDGET (self)));
  GtkEntry * entry =
    GTK_ENTRY (gtk_entry_new ());
  self->entry = entry;
  gtk_widget_set_visible (
    GTK_WIDGET (entry), 1);
  gtk_entry_set_text (
    self->entry,
    (*self->getter) (self->object));

  gtk_container_add (
    GTK_CONTAINER (self->popover),
    GTK_WIDGET (entry));

  g_signal_connect (
    G_OBJECT (self->entry), "activate",
    G_CALLBACK (on_entry_activated), self);
  g_signal_connect (
    G_OBJECT (self->popover), "closed",
    G_CALLBACK (on_popover_closed), self);

  gtk_popover_popup (self->popover);
  gtk_entry_set_text (
    self->entry,
    (*self->getter) (self->object));
}

/**
 * Shows a popover without the need of an editable
 * label.
 */
void
editable_label_widget_show_popover_for_widget (
  GtkWidget * parent,
  void *      object,
  EditableLabelWidgetTextGetter getter,
  EditableLabelWidgetTextSetter setter)
{
  EditableLabelWidget * self =
    g_object_new (
      EDITABLE_LABEL_WIDGET_TYPE, NULL);

  self->object = object;
  self->getter = getter;
  self->setter = setter;
  self->is_temp = 1;

  g_object_ref (self);

  self->popover =
    GTK_POPOVER (
      gtk_popover_new (GTK_WIDGET (parent)));
  GtkEntry * entry =
    GTK_ENTRY (gtk_entry_new ());
  self->entry = entry;
  gtk_widget_set_visible (
    GTK_WIDGET (entry), 1);
  gtk_entry_set_text (
    self->entry, (*getter) (object));

  gtk_container_add (
    GTK_CONTAINER (self->popover),
    GTK_WIDGET (entry));

  g_signal_connect (
    G_OBJECT (self->entry), "activate",
    G_CALLBACK (on_entry_activated), self);
  g_signal_connect (
    G_OBJECT (self->popover), "closed",
    G_CALLBACK (on_popover_closed), self);

  gtk_popover_popup (self->popover);
  gtk_entry_set_text (
    self->entry, (*getter) (object));
}

/**
 * Multipress handler.
 */
static void
editable_label_widget_on_mp_press (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  EditableLabelWidget * self)
{
  if (n_press == 2)
    {
      editable_label_widget_show_popover (self);
    }
}

/**
 * Sets up an existing EditableLabelWidget.
 *
 * @param get_val Getter function.
 * @param set_val Setter function.
 * @param object Object to call get/set with.
 */
void
editable_label_widget_setup (
  EditableLabelWidget * self,
  void *                object,
  const char * (*get_val)(void *),
  void (*set_val)(void *, const char *))
{
  self->object = object;
  self->getter = get_val;
  self->setter = set_val;

  if (object)
    {
      gtk_label_set_text (
        self->label,
        (*self->getter) (self->object));
    }
}

/**
 * Returns a new instance of EditableLabelWidget.
 *
 * @param get_val Getter function.
 * @param set_val Setter function.
 * @param object Object to call get/set with.
 * @param width Label width in chars.
 */
EditableLabelWidget *
editable_label_widget_new (
  void *                object,
  const char * (*get_val)(void *),
  void (*set_val)(void *, const char *),
  int                    width)
{
  EditableLabelWidget * self =
    g_object_new (EDITABLE_LABEL_WIDGET_TYPE,
                  NULL);

  editable_label_widget_setup (
    self, object, get_val, set_val);

  gtk_label_set_width_chars (
    self->label, 11);
  gtk_label_set_max_width_chars (
    self->label, 11);

  gtk_widget_set_visible (
    GTK_WIDGET (self), 1);

  return self;
}

static void
editable_label_widget_class_init (
  EditableLabelWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "editable-label");
}

static void
editable_label_widget_init (
  EditableLabelWidget * self)
{
  GtkWidget * label =
    gtk_label_new ("");
  gtk_widget_set_visible (label, 1);
  gtk_container_add (
    GTK_CONTAINER (self), label);
  self->label = GTK_LABEL (label);
  gtk_label_set_ellipsize (
    self->label, PANGO_ELLIPSIZE_END);

  self->mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));

  g_signal_connect (
    G_OBJECT (self->mp), "pressed",
    G_CALLBACK (
      editable_label_widget_on_mp_press), self);
}
