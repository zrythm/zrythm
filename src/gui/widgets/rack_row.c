/*
 * gui/widgets/rack_row.c - Rack row
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/widgets/automator.h"
#include "gui/widgets/rack_plugin.h"
#include "gui/widgets/rack_row.h"
#include "utils/gtk.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (RackRowWidget, rack_row_widget, GTK_TYPE_BOX)

static void
rack_row_widget_class_init (RackRowWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (
    GTK_WIDGET_CLASS (klass),
    "/online/alextee/zrythm/ui/rack_row.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        RackRowWidget,
                                        expander);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        RackRowWidget,
                                        label);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        RackRowWidget,
                                        box);
}

static void
rack_row_widget_init (RackRowWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

RackRowWidget *
rack_row_widget_new (RRWType              type,
                     Channel *            channel) ///< channel, if type channel
{
  RackRowWidget * self = g_object_new (RACK_ROW_WIDGET_TYPE, NULL);

  self->type = type;
  if (type == RRW_TYPE_AUTOMATORS)
    {
      /* set expander label */
      gtk_label_set_text (self->label, "Automators");

      /* set automator widget */
      self->automators[0] = automator_widget_new ();
      self->num_automators = 1;
      gtk_box_pack_start (GTK_BOX (self->box),
                          GTK_WIDGET (self->automators[0]),
                          Z_GTK_NO_EXPAND,
                          Z_GTK_NO_FILL,
                          0);
      gtk_widget_set_halign (GTK_WIDGET (self->automators[0]),
                             GTK_ALIGN_START);
    }
  else if (channel)
    {
      self->channel = channel;

      /* set explander label */
      gtk_label_set_text (self->label, channel->name);
      for (int i = 0; i < STRIP_SIZE; i++)
        {
          Plugin * plugin = channel->strip[i];
          if (plugin)
            {
              self->rack_plugins[i] = rack_plugin_widget_new (plugin);
              gtk_box_pack_start (GTK_BOX (self->box),
                                  GTK_WIDGET (self->rack_plugins[i]),
                                  Z_GTK_NO_EXPAND,
                                  Z_GTK_NO_FILL,
                                  0);
              gtk_widget_set_halign (GTK_WIDGET (self->rack_plugins[i]),
                                     GTK_ALIGN_START);
            }
        }
    }

  return self;
}


