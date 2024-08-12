// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_note.h"
#include "gui/backend/midi_selections.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/editor_selection_info.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/selection_info.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  EditorSelectionInfoWidget,
  editor_selection_info_widget,
  GTK_TYPE_STACK)

void
editor_selection_info_widget_refresh (
  EditorSelectionInfoWidget * self,
  MidiSelections *            mas)
{
  ArrangerObject * obj =
    arranger_selections_get_first_object ((ArrangerSelections *) mas, 0);

  selection_info_widget_clear (self->selection_info);
  gtk_stack_set_visible_child (
    GTK_STACK (self), GTK_WIDGET (self->no_selection_label));

  if (obj->type == ArrangerObject::Type::MidiNote)
    {
      DigitalMeterWidget * dm = digital_meter_widget_new_for_position (
        obj, nullptr, arranger_object_get_pos, arranger_object_pos_setter,
        nullptr, _ ("start position"));
      digital_meter_set_draw_line (dm, 1);

      selection_info_widget_add_info (
        self->selection_info, nullptr, GTK_WIDGET (dm));
      gtk_stack_set_visible_child (
        GTK_STACK (self), GTK_WIDGET (self->selection_info));
    }
}

static void
editor_selection_info_widget_class_init (EditorSelectionInfoWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "piano-roll-selection-info");
}

static void
editor_selection_info_widget_init (EditorSelectionInfoWidget * self)
{
  self->no_selection_label =
    GTK_LABEL (gtk_label_new (_ ("No object selected")));
  gtk_widget_set_visible (GTK_WIDGET (self->no_selection_label), 1);
  self->selection_info = g_object_new (SELECTION_INFO_WIDGET_TYPE, nullptr);
  gtk_widget_set_visible (GTK_WIDGET (self->selection_info), 1);

  gtk_stack_add_named (
    GTK_STACK (self), GTK_WIDGET (self->no_selection_label), "no-selection");
  gtk_stack_add_named (
    GTK_STACK (self), GTK_WIDGET (self->selection_info), "selection-info");

  gtk_widget_set_size_request (GTK_WIDGET (self), -1, 38);
}
