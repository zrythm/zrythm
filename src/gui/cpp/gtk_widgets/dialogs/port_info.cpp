// SPDX-FileCopyrightText: © 2020-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/gtk_widgets/dialogs/port_info.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#include <glib/gi18n.h>

#include "common/dsp/control_port.h"
#include "common/dsp/port.h"
#include "common/utils/ui.h"

G_DEFINE_TYPE (PortInfoDialogWidget, port_info_dialog_widget, GTK_TYPE_WINDOW)

static void
set_values (
  PortInfoDialogWidget * self,
  AdwPreferencesPage *   pref_page,
  Port *                 port)
{
  self->port = port;

  PortIdentifier * id = &port->id_;

  AdwPreferencesGroup * pref_group =
    ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (pref_group, _ ("Port Info"));
  adw_preferences_page_add (pref_page, pref_group);

  AdwActionRow * row;
  GtkLabel *     lbl;
  char           tmp[600];

#define PREPARE_ROW(title) \
  row = ADW_ACTION_ROW (adw_action_row_new ()); \
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), title); \
  lbl = GTK_LABEL (gtk_label_new (nullptr))

#define ADD_ROW \
  adw_action_row_add_suffix (row, GTK_WIDGET (lbl)); \
  adw_preferences_group_add (pref_group, GTK_WIDGET (row))

  PREPARE_ROW (_ ("Name"));
  gtk_label_set_text (lbl, id->label_.c_str ());
  ADD_ROW;

  PREPARE_ROW (_ ("Group"));
  gtk_label_set_text (lbl, id->port_group_.c_str ());
  ADD_ROW;

  PREPARE_ROW (_ ("Designation"));
  gtk_label_set_text (lbl, port->get_full_designation ().c_str ());
  ADD_ROW;

  PREPARE_ROW (_ ("Type"));
  gtk_label_set_text (lbl, ENUM_NAME (id->type_));
  ADD_ROW;

  PREPARE_ROW (_ ("Range"));
  sprintf (tmp, _ ("%.1f to %.1f"), (double) port->minf_, (double) port->maxf_);
  gtk_label_set_text (lbl, tmp);
  ADD_ROW;

  PREPARE_ROW (_ ("Default Value"));
  if (id->is_control ())
    {
      auto ctrl = static_cast<ControlPort *> (port);
      sprintf (tmp, "%.1f", (double) ctrl->deff_);
      gtk_label_set_text (lbl, tmp);
    }
  else
    {
      gtk_label_set_text (lbl, _ ("N/A"));
    }
  ADD_ROW;

  PREPARE_ROW (_ ("Current Value"));
  if (id->type_ == PortType::Control)
    {
      auto ctrl = static_cast<ControlPort *> (port);
      sprintf (tmp, "%f", (double) ctrl->control_);
      gtk_label_set_text (lbl, tmp);
    }
  else
    {
      gtk_label_set_text (lbl, _ ("N/A"));
    }
  ADD_ROW;

  PREPARE_ROW (_ ("Flags"));
  GtkFlowBox * flags_box = GTK_FLOW_BOX (gtk_flow_box_new ());
  for (size_t i = 0; i <= 30; i++)
    {
      if (
        !(ENUM_BITSET_TEST (
          PortIdentifier::Flags, id->flags_,
          ENUM_INT_TO_VALUE (PortIdentifier::Flags, 1 << i))))
        continue;

      GtkWidget * lbl_ = gtk_label_new (
        ENUM_NAME (ENUM_INT_TO_VALUE (PortIdentifier::Flags, i)));
      gtk_widget_set_hexpand (lbl_, 1);
      gtk_flow_box_append (GTK_FLOW_BOX (flags_box), lbl_);
    }
  for (size_t i = 0; i <= 30; i++)
    {
      if (
        !(ENUM_BITSET_TEST (
          PortIdentifier::Flags2, id->flags2_,
          ENUM_INT_TO_VALUE (PortIdentifier::Flags2, 1 << i))))
        continue;

      GtkWidget * lbl_ = gtk_label_new (
        ENUM_NAME (ENUM_INT_TO_VALUE (PortIdentifier::Flags2, i)));
      gtk_widget_set_hexpand (lbl_, 1);
      gtk_flow_box_append (GTK_FLOW_BOX (flags_box), lbl_);
    }
  adw_action_row_add_suffix (row, GTK_WIDGET (flags_box));
  adw_preferences_group_add (pref_group, GTK_WIDGET (row));

  /* TODO scale points */
}

/**
 * Creates a new port_info dialog.
 */
PortInfoDialogWidget *
port_info_dialog_widget_new (Port * port)
{
  PortInfoDialogWidget * self = Z_PORT_INFO_DIALOG_WIDGET (
    g_object_new (PORT_INFO_DIALOG_WIDGET_TYPE, nullptr));

  AdwPreferencesPage * pref_page =
    ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  gtk_window_set_child (GTK_WINDOW (self), GTK_WIDGET (pref_page));
  set_values (self, pref_page, port);

  return self;
}

static void
port_info_dialog_widget_class_init (PortInfoDialogWidgetClass * _klass)
{
}

static void
port_info_dialog_widget_init (PortInfoDialogWidget * self)
{
  gtk_window_set_title (GTK_WINDOW (self), _ ("Port Info"));
  gtk_window_set_icon_name (GTK_WINDOW (self), "zrythm");
  gtk_window_set_default_size (GTK_WINDOW (self), 480, -1);
}
