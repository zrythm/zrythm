// clang-format off
// SPDX-FileCopyrightText: © 2019-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

#include "gui/widgets/help_toolbar.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (HelpToolbarWidget, help_toolbar_widget, GTK_TYPE_BOX)

static void
help_toolbar_widget_init (HelpToolbarWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable (GTK_ACTIONABLE (self->x), tooltip)
  SET_TOOLTIP (about, _ ("About Zrythm"));
  SET_TOOLTIP (chat, _ ("Chat (Matrix)"));
  SET_TOOLTIP (manual, _ ("Manual"));
  SET_TOOLTIP (shortcuts, _ ("Keyboard Shortcuts"));
  SET_TOOLTIP (donate_btn, _ ("Donate"));
  SET_TOOLTIP (report_a_bug_btn, _ ("Report a Bug"));
#undef SET_TOOLTIP

  /* TODO port */
#if 0
  /* these icons get extremely large on windows */
  z_gtk_tool_button_set_icon_size (
    self->manual, GTK_ICON_SIZE_SMALL_TOOLBAR);
  z_gtk_tool_button_set_icon_size (
    self->shortcuts, GTK_ICON_SIZE_SMALL_TOOLBAR);
#endif
}

static void
help_toolbar_widget_class_init (HelpToolbarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "help_toolbar.ui");

  gtk_widget_class_set_css_name (klass, "help-toolbar");
  gtk_widget_class_set_accessible_role (klass, GTK_ACCESSIBLE_ROLE_TOOLBAR);

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, HelpToolbarWidget, x)

  BIND_CHILD (about);
  BIND_CHILD (chat);
  BIND_CHILD (manual);
  BIND_CHILD (shortcuts);
  BIND_CHILD (donate_btn);
  BIND_CHILD (report_a_bug_btn);

#undef BIND_CHILD
}
