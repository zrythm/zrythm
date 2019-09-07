/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/widgets/help_toolbar.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (HelpToolbarWidget,
               help_toolbar_widget,
               GTK_TYPE_TOOLBAR)

static void
help_toolbar_widget_init (HelpToolbarWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), \
    tooltip)
  SET_TOOLTIP (chat, _("Chat (Matrix)"));
  SET_TOOLTIP (manual, _("Manual"));
  SET_TOOLTIP (shortcuts, _("Keyboard Shortcuts"));
  SET_TOOLTIP (donate_btn, _("Donate"));
  SET_TOOLTIP (report_a_bug_btn, _("Report a Bug"));
#undef SET_TOOLTIP
}

static void
help_toolbar_widget_class_init (HelpToolbarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "help_toolbar.ui");

  gtk_widget_class_set_css_name (
    klass, "help-toolbar");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    HelpToolbarWidget, \
    x)

  BIND_CHILD (chat);
  BIND_CHILD (manual);
  BIND_CHILD (shortcuts);
  BIND_CHILD (donate_btn);
  BIND_CHILD (report_a_bug_btn);

#undef BIND_CHILD
}
