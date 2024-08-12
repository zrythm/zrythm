/*
 * SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 */

#include "gui/widgets/main_window.h"
#include "gui/widgets/range_action_buttons.h"
#include "project.h"
#include "utils/resources.h"

G_DEFINE_TYPE (RangeActionButtonsWidget, range_action_buttons_widget, GTK_TYPE_BOX)

static void
range_action_buttons_widget_class_init (RangeActionButtonsWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "range_action_buttons.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, RangeActionButtonsWidget, x)

  BIND_CHILD (insert_silence);
  BIND_CHILD (remove);

#undef BIND_CHILD
}

static void
range_action_buttons_widget_init (RangeActionButtonsWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
