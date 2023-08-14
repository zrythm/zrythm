/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-config.h"

#include "dsp/engine.h"
#include "dsp/engine_jack.h"
#include "dsp/transport.h"
#include "gui/widgets/cpu.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/live_waveform.h"
#include "gui/widgets/midi_activity_bar.h"
#include "gui/widgets/top_bar.h"
#include "gui/widgets/transport_controls.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (TopBarWidget, top_bar_widget, GTK_TYPE_BOX)

void
top_bar_widget_refresh (TopBarWidget * self)
{
}

static void
top_bar_widget_class_init (TopBarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "top_bar.ui");

  gtk_widget_class_set_css_name (klass, "top-bar");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child ( \
    klass, TopBarWidget, child)

#undef BIND_CHILD
}

static void
top_bar_widget_init (TopBarWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
