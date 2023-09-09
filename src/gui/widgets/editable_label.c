// SPDX-FileCopyrightText: © 2019-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/editable_label.h"

G_DEFINE_TYPE (EditableLabelWidget, editable_label_widget, GTK_TYPE_WIDGET)

static void
on_entry_activated (GtkEntry * entry, EditableLabelWidget * self)
{
  (*self->setter) (self->object, gtk_editable_get_text (GTK_EDITABLE (entry)));
  if (self->foreign_popover)
    gtk_widget_set_visible (GTK_WIDGET (self->foreign_popover), false);

  gtk_label_set_text (self->label, (*self->getter) (self->object));
}

static void
on_popover_closed (GtkPopover * popover, EditableLabelWidget * self)
{
  if (self->is_temp)
    {
      /*g_object_unref (self);*/
    }
}

static gboolean
select_region (gpointer user_data)
{
  EditableLabelWidget * self = Z_EDITABLE_LABEL_WIDGET (user_data);
  gtk_editable_select_region (GTK_EDITABLE (self->entry), 0, -1);

  return G_SOURCE_REMOVE;
}

/**
 * Shows the popover.
 */
void
editable_label_widget_show_popover (EditableLabelWidget * self)
{
  gtk_popover_popup (self->popover);
  gtk_editable_set_text (
    GTK_EDITABLE (self->entry), (*self->getter) (self->object));

  /* workaround because selecting a region doesn't
   * work 100% of the time if done here */
  if (self->select_region_source_id != 0)
    {
      self->select_region_source_id = g_idle_add (select_region, self);
    }
}

/**
 * Shows a popover without the need of an editable
 * label.
 *
 * @param popover A pre-created popover that is a
 *   child of @ref parent.
 */
void
editable_label_widget_show_popover_for_widget (
  GtkWidget *         parent,
  GtkPopover *        popover,
  void *              object,
  GenericStringGetter getter,
  GenericStringSetter setter)
{
  g_return_if_fail (GTK_IS_WIDGET (parent));
  g_return_if_fail (GTK_IS_POPOVER (popover));

  EditableLabelWidget * self = g_object_new (EDITABLE_LABEL_WIDGET_TYPE, NULL);

  self->object = object;
  self->getter = getter;
  self->setter = setter;
  self->is_temp = 1;
  self->foreign_popover = popover;

  g_object_ref (self);

  GtkEntry * entry = GTK_ENTRY (gtk_entry_new ());
  self->entry = entry;

  gtk_popover_set_child (popover, GTK_WIDGET (entry));

  g_signal_connect (
    G_OBJECT (self->entry), "activate", G_CALLBACK (on_entry_activated), self);
  g_signal_connect (
    G_OBJECT (popover), "closed", G_CALLBACK (on_popover_closed), self);

  gtk_popover_popup (popover);
  gtk_editable_set_text (GTK_EDITABLE (self->entry), (*getter) (object));

  /* workaround because selecting a region doesn't
   * work 100% of the time if done here */
  if (self->select_region_source_id != 0)
    {
      self->select_region_source_id = g_idle_add (select_region, self);
    }
}

/**
 * Multipress handler.
 */
static void
editable_label_widget_on_mp_press (
  GtkGestureClick *     gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  EditableLabelWidget * self)
{
  g_debug ("n press %d", n_press);

  if (n_press == 2 && self->setter)
    {
      editable_label_widget_show_popover (self);
    }
}

/**
 * Sets up an existing EditableLabelWidget.
 *
 * @param getter Getter function.
 * @param setter Setter function.
 * @param object Object to call get/set with.
 */
void
editable_label_widget_setup (
  EditableLabelWidget * self,
  void *                object,
  GenericStringGetter   getter,
  GenericStringSetter   setter)
{
  self->object = object;
  self->getter = getter;
  self->setter = setter;

  if (self->getter)
    {
      gtk_label_set_text (self->label, (*self->getter) (self->object));
    }
}

/**
 * Returns a new instance of EditableLabelWidget.
 *
 * @param getter Getter function.
 * @param setter Setter function.
 * @param object Object to call get/set with.
 * @param width Label width in chars.
 */
EditableLabelWidget *
editable_label_widget_new (
  void *              object,
  GenericStringGetter getter,
  GenericStringSetter setter,
  int                 width)
{
  EditableLabelWidget * self = g_object_new (EDITABLE_LABEL_WIDGET_TYPE, NULL);

  editable_label_widget_setup (self, object, getter, setter);

  gtk_label_set_width_chars (self->label, 11);
  gtk_label_set_max_width_chars (self->label, 11);

  return self;
}

static void
dispose (EditableLabelWidget * self)
{
  if (self->select_region_source_id != 0)
    {
      g_source_remove (self->select_region_source_id);
      self->select_region_source_id = 0;
    }

  gtk_widget_unparent (GTK_WIDGET (self->popover));
  gtk_widget_unparent (GTK_WIDGET (self->label));

  G_OBJECT_CLASS (editable_label_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
editable_label_widget_class_init (EditableLabelWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "editable-label");

  gtk_widget_class_set_layout_manager_type (klass, GTK_TYPE_BOX_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}

static void
editable_label_widget_init (EditableLabelWidget * self)
{
  GtkWidget * label = gtk_label_new ("");
  gtk_widget_set_parent (GTK_WIDGET (label), GTK_WIDGET (self));
  self->label = GTK_LABEL (label);
  gtk_label_set_ellipsize (self->label, PANGO_ELLIPSIZE_END);

  self->popover = GTK_POPOVER (gtk_popover_new ());
  gtk_widget_set_parent (GTK_WIDGET (self->popover), GTK_WIDGET (self));
  GtkEntry * entry = GTK_ENTRY (gtk_entry_new ());
  self->entry = entry;
  gtk_popover_set_child (self->popover, GTK_WIDGET (entry));

  g_signal_connect (
    G_OBJECT (self->entry), "activate", G_CALLBACK (on_entry_activated), self);
  g_signal_connect (
    G_OBJECT (self->popover), "closed", G_CALLBACK (on_popover_closed), self);

  self->mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  g_signal_connect (
    G_OBJECT (self->mp), "pressed",
    G_CALLBACK (editable_label_widget_on_mp_press), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->mp));
}
