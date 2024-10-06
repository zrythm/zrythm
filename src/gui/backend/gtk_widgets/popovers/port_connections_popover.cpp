// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/automation_track.h"
#include "common/dsp/port_connections_manager.h"
#include "common/utils/gtk.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/gtk_widgets/dialogs/port_selector_dialog.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/inspector_port.h"
#include "gui/backend/gtk_widgets/popovers/port_connections_popover.h"
#include "gui/backend/gtk_widgets/port_connection_row.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  PortConnectionsPopoverWidget,
  port_connections_popover_widget,
  GTK_TYPE_POPOVER)

static void
on_add_clicked (GtkButton * btn, PortConnectionsPopoverWidget * self)
{
  PortSelectorDialogWidget * dialog = port_selector_dialog_widget_new (self);
  port_selector_dialog_widget_refresh (dialog, self->port);

  gtk_widget_set_visible (GTK_WIDGET (self), false);

  gtk_window_present (GTK_WINDOW (dialog));
}

/**
 * Refreshes the popover.
 *
 * Removes all children of ports_box and readds them
 * based on the current connections.
 */
void
port_connections_popover_widget_refresh (
  PortConnectionsPopoverWidget * self,
  Port *                         port)
{
  z_return_if_fail (IS_PORT_AND_NONNULL (port));

  self->port = port;

  z_gtk_widget_destroy_all_children (GTK_WIDGET (self->ports_box));

  /* set title and add ports */
  if (self->port->id_.flow_ == PortFlow::Input)
    {
      if (GTK_IS_LABEL (self->title))
        {
          gtk_label_set_text (self->title, _ ("INPUTS"));
        }

      std::vector<PortConnection> srcs;
      PORT_CONNECTIONS_MGR->get_sources_or_dests (&srcs, self->port->id_, true);
      for (const auto &conn : srcs)
        {
          if (!conn.locked_)
            {
              PortConnectionRowWidget * pcr =
                port_connection_row_widget_new (self, conn, false);
              gtk_box_append (GTK_BOX (self->ports_box), GTK_WIDGET (pcr));
            }
        }
    }
  else if (self->port->id_.flow_ == PortFlow::Output)
    {
      if (GTK_IS_LABEL (self->title))
        {
          gtk_label_set_text (self->title, _ ("OUTPUTS"));
        }

      std::vector<PortConnection> dests;
      PORT_CONNECTIONS_MGR->get_sources_or_dests (
        &dests, self->port->id_, false);
      for (const auto &conn : dests)
        {
          if (!conn.locked_)
            {
              PortConnectionRowWidget * pcr =
                port_connection_row_widget_new (self, conn, true);
              gtk_box_append (GTK_BOX (self->ports_box), GTK_WIDGET (pcr));
            }
        }
    }
}

/**
 * Creates the popover.
 *
 * @param owner Owner widget to pop up at.
 * @param port Owner port.
 */
PortConnectionsPopoverWidget *
port_connections_popover_widget_new (GtkWidget * owner)
{
  z_return_val_if_fail (GTK_IS_WIDGET (owner), nullptr);

  PortConnectionsPopoverWidget * self = Z_PORT_CONNECTIONS_POPOVER_WIDGET (
    g_object_new (PORT_CONNECTIONS_POPOVER_WIDGET_TYPE, nullptr));

  return self;
}

static void
finalize (PortConnectionsPopoverWidget * self)
{
  G_OBJECT_CLASS (port_connections_popover_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
port_connections_popover_widget_class_init (
  PortConnectionsPopoverWidgetClass * _klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
port_connections_popover_widget_init (PortConnectionsPopoverWidget * self)
{
  /* create all */
  self->main_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 2));
  self->title = GTK_LABEL (gtk_label_new (""));
  self->ports_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 1));

  self->add = GTK_BUTTON (gtk_button_new ());
  GtkWidget * btn_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  GtkWidget * img = gtk_image_new_from_icon_name ("add");
  gtk_box_append (GTK_BOX (btn_box), img);
  GtkWidget * lbl = gtk_label_new (_ ("Add"));
  gtk_box_append (GTK_BOX (btn_box), lbl);
  gtk_button_set_child (GTK_BUTTON (self->add), btn_box);
  g_signal_connect (
    G_OBJECT (self->add), "clicked", G_CALLBACK (on_add_clicked), self);

  GtkWidget * separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);

  /* add to each other */
  gtk_box_append (self->main_box, GTK_WIDGET (self->title));
  gtk_box_append (self->main_box, separator);
  gtk_box_append (self->main_box, GTK_WIDGET (self->ports_box));
  gtk_box_append (self->main_box, GTK_WIDGET (self->add));

  /* add to popover */
  gtk_popover_set_child (GTK_POPOVER (self), GTK_WIDGET (self->main_box));
}
