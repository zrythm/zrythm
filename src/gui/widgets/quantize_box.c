// SPDX-FileCopyrightText: © 2019, 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 */

#include "gui/widgets/main_window.h"
#include "gui/widgets/quantize_box.h"
#include "gui/widgets/snap_grid.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (QuantizeBoxWidget, quantize_box_widget, GTK_TYPE_BOX)

/**
 * Sets up the QuantizeBoxWidget.
 */
void
quantize_box_widget_setup (QuantizeBoxWidget * self, QuantizeOptions * q_opts)
{
  self->q_opts = q_opts;
  if (QUANTIZE_OPTIONS_IS_EDITOR (q_opts))
    {
      gtk_actionable_set_action_target (
        GTK_ACTIONABLE (self->quick_quantize_btn), "s", "editor");
      gtk_actionable_set_action_target (
        GTK_ACTIONABLE (self->quantize_opts_btn), "s", "editor");
    }
  else if (QUANTIZE_OPTIONS_IS_TIMELINE (q_opts))
    {
      gtk_actionable_set_action_target (
        GTK_ACTIONABLE (self->quick_quantize_btn), "s", "timeline");
      gtk_actionable_set_action_target (
        GTK_ACTIONABLE (self->quantize_opts_btn), "s", "timeline");
    }
}

static void
quantize_box_widget_class_init (QuantizeBoxWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "quantize_box.ui");
  gtk_widget_class_set_css_name (klass, "quantize-box");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, QuantizeBoxWidget, x)

  BIND_CHILD (quick_quantize_btn);
  BIND_CHILD (quantize_opts_btn);

#undef BIND_CHILD
}

static void
quantize_box_widget_init (QuantizeBoxWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#define SET_TOOLTIP(x) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), \
    gtk_widget_get_tooltip_text (GTK_WIDGET (self->x)))

  SET_TOOLTIP (quick_quantize_btn);
  SET_TOOLTIP (quantize_opts_btn);

#undef SET_TOOLTIP
}
