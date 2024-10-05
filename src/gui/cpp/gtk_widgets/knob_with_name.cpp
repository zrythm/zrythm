// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/gtk_widgets/editable_label.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/knob.h"
#include "gui/cpp/gtk_widgets/knob_with_name.h"

G_DEFINE_TYPE (KnobWithNameWidget, knob_with_name_widget, GTK_TYPE_BOX)

/**
 * Returns a new instance.
 *
 * @param label_before Whether to show the label
 *   before the knob.
 */
KnobWithNameWidget *
knob_with_name_widget_new (
  void *              obj,
  GenericStringGetter name_getter,
  GenericStringSetter name_setter,
  KnobWidget *        knob,
  GtkOrientation      orientation,
  bool                label_before,
  int                 spacing)
{
  KnobWithNameWidget * self = static_cast<KnobWithNameWidget *> (g_object_new (
    KNOB_WITH_NAME_WIDGET_TYPE, "orientation", orientation, "spacing", 2,
    nullptr));

  EditableLabelWidget * label =
    editable_label_widget_new (obj, name_getter, name_setter, -1);

  if (label_before)
    {
      gtk_box_append (GTK_BOX (self), GTK_WIDGET (label));
      gtk_box_append (GTK_BOX (self), GTK_WIDGET (knob));
    }
  else
    {
      gtk_box_append (GTK_BOX (self), GTK_WIDGET (knob));
      gtk_box_append (GTK_BOX (self), GTK_WIDGET (label));
    }

  return self;
}

static void
dispose (KnobWithNameWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));

  G_OBJECT_CLASS (knob_with_name_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
knob_with_name_widget_class_init (KnobWithNameWidgetClass * _klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}

static void
knob_with_name_widget_init (KnobWithNameWidget * self)
{
  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (nullptr));
  gtk_widget_set_parent (GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));
}
