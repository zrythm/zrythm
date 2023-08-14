// SPDX-FileCopyrightText: © 2020-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port.h"
#include "gui/widgets/dialogs/port_info.h"
#include "project.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "utils/ui.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  PortInfoDialogWidget,
  port_info_dialog_widget,
  GTK_TYPE_WINDOW)

static void
set_values (
  PortInfoDialogWidget * self,
  AdwPreferencesPage *   pref_page,
  Port *                 port)
{
  self->port = port;

  PortIdentifier * id = &port->id;

  AdwPreferencesGroup * pref_group =
    ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (
    pref_group, _ ("Port Info"));
  adw_preferences_page_add (pref_page, pref_group);

  AdwActionRow * row;
  GtkLabel *     lbl;
  char           tmp[600];

#define PREPARE_ROW(title) \
  row = ADW_ACTION_ROW (adw_action_row_new ()); \
  adw_preferences_row_set_title ( \
    ADW_PREFERENCES_ROW (row), title); \
  lbl = GTK_LABEL (gtk_label_new (NULL))

#define ADD_ROW \
  adw_action_row_add_suffix (row, GTK_WIDGET (lbl)); \
  adw_preferences_group_add (pref_group, GTK_WIDGET (row))

  PREPARE_ROW (_ ("Name"));
  gtk_label_set_text (lbl, id->label);
  ADD_ROW;

  PREPARE_ROW (_ ("Group"));
  gtk_label_set_text (lbl, id->port_group);
  ADD_ROW;

  PREPARE_ROW (_ ("Designation"));
  port_get_full_designation (port, tmp);
  gtk_label_set_text (lbl, tmp);
  ADD_ROW;

  PREPARE_ROW (_ ("Type"));
  gtk_label_set_text (lbl, port_type_strings[id->type].str);
  ADD_ROW;

  PREPARE_ROW (_ ("Range"));
  sprintf (
    tmp, _ ("%.1f to %.1f"), (double) port->minf,
    (double) port->maxf);
  gtk_label_set_text (lbl, tmp);
  ADD_ROW;

  PREPARE_ROW (_ ("Default Value"));
  sprintf (tmp, "%.1f", (double) port->deff);
  gtk_label_set_text (lbl, tmp);
  ADD_ROW;

  PREPARE_ROW (_ ("Current Value"));
  if (id->type == TYPE_CONTROL)
    {
      sprintf (tmp, "%f", (double) port->control);
      gtk_label_set_text (lbl, tmp);
    }
  else
    {
      gtk_label_set_text (lbl, _ ("N/A"));
    }
  ADD_ROW;

  PREPARE_ROW (_ ("Flags"));
  GtkFlowBox * flags_box = GTK_FLOW_BOX (gtk_flow_box_new ());
  for (int i = 0;
       i < (int) CYAML_ARRAY_LEN (port_flags_bitvals); i++)
    {
      if (!(id->flags & (1 << i)))
        continue;

      GtkWidget * lbl_ =
        gtk_label_new (port_flags_bitvals[i].name);
      gtk_widget_set_hexpand (lbl_, 1);
      gtk_flow_box_append (GTK_FLOW_BOX (flags_box), lbl_);
    }
  for (int i = 0;
       i < (int) CYAML_ARRAY_LEN (port_flags2_bitvals); i++)
    {
      if (!(id->flags2 & (unsigned int) (1 << i)))
        continue;

      GtkWidget * lbl_ =
        gtk_label_new (port_flags2_bitvals[i].name);
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
  PortInfoDialogWidget * self =
    g_object_new (PORT_INFO_DIALOG_WIDGET_TYPE, NULL);

  AdwPreferencesPage * pref_page =
    ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  gtk_window_set_child (
    GTK_WINDOW (self), GTK_WIDGET (pref_page));
  set_values (self, pref_page, port);

  return self;
}

static void
port_info_dialog_widget_class_init (
  PortInfoDialogWidgetClass * _klass)
{
}

static void
port_info_dialog_widget_init (PortInfoDialogWidget * self)
{
  gtk_window_set_title (GTK_WINDOW (self), _ ("Port Info"));
  gtk_window_set_icon_name (GTK_WINDOW (self), "zrythm");
  gtk_window_set_default_size (GTK_WINDOW (self), 480, -1);
}
