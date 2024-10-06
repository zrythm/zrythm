// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/gtk.h"
#include "common/utils/resources.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/gtk_widgets/cc_bindings.h"
#include "gui/backend/gtk_widgets/cc_bindings_tree.h"
#include "gui/backend/gtk_widgets/center_dock.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/left_dock_edge.h"
#include "gui/backend/gtk_widgets/main_window.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (CcBindingsWidget, cc_bindings_widget, GTK_TYPE_BOX)

/**
 * Refreshes the cc_bindings widget.
 */
void
cc_bindings_widget_refresh (CcBindingsWidget * self)
{
  cc_bindings_tree_widget_refresh (self->bindings_tree);
}

CcBindingsWidget *
cc_bindings_widget_new (void)
{
  CcBindingsWidget * self = static_cast<CcBindingsWidget *> (
    g_object_new (CC_BINDINGS_WIDGET_TYPE, nullptr));

  self->bindings_tree = cc_bindings_tree_widget_new ();
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->bindings_tree));
  gtk_widget_set_vexpand (GTK_WIDGET (self->bindings_tree), 1);

  return self;
}

static void
cc_bindings_widget_class_init (CcBindingsWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "cc-bindings");
}

static void
cc_bindings_widget_init (CcBindingsWidget * self)
{
}
