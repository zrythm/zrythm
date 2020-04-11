/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/channel.h"
#include "audio/track.h"
#include "gui/widgets/channel_slot.h"
#include "gui/widgets/plugin_strip_expander.h"
#include "project.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#define PLUGIN_STRIP_EXPANDER_WIDGET_TYPE \
  (plugin_strip_expander_widget_get_type ())
G_DEFINE_TYPE (
  PluginStripExpanderWidget,
  plugin_strip_expander_widget,
  EXPANDER_BOX_WIDGET_TYPE)

/**
 * Queues a redraw of the given slot.
 */
void
plugin_strip_expander_widget_redraw_slot (
  PluginStripExpanderWidget * self,
  int                         slot)
{
  switch (self->type)
    {
    case PSE_TYPE_INSERTS:
      gtk_widget_queue_draw (
        GTK_WIDGET (self->inserts[slot]));
      break;
    default:
      break;
    }
}

/**
 * Unsets state flags and redraws the widget at the
 * given slot.
 *
 * @param slot Slot, or -1 for all slots.
 * @param set True to set, false to unset.
 */
void
plugin_strip_expander_widget_set_state_flags (
  PluginStripExpanderWidget * self,
  int                         slot,
  GtkStateFlags               flags,
  bool                        set)
{
  switch (self->type)
    {
    case PSE_TYPE_INSERTS:
      if (slot == -1)
        {
          for (int i = 0; i < STRIP_SIZE; i++)
            {
              plugin_strip_expander_widget_set_state_flags (
                self, i, flags, set);
            }
        }
      else
        {
          if (set)
            {
              gtk_widget_set_state_flags (
                GTK_WIDGET (self->inserts[slot]),
                flags, 0);
            }
          else
            {
              gtk_widget_unset_state_flags (
                GTK_WIDGET (self->inserts[slot]),
                flags);
            }
          gtk_widget_queue_draw (
            GTK_WIDGET (self->inserts[slot]));
        }
      break;
    default:
      break;
    }
}

/**
 * Refreshes each field.
 */
void
plugin_strip_expander_widget_refresh (
  PluginStripExpanderWidget * self)
{
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      switch (self->type)
        {
        case PSE_TYPE_INSERTS:
          gtk_widget_queue_draw (
            GTK_WIDGET (self->inserts[i]));
          break;
        default:
          break;
        }
    }
}

static void
set_icon_from_type (
  PluginStripExpanderWidget * self,
  PluginStripExpanderType     type)
{
  /* TODO */
  expander_box_widget_set_icon_name (
    Z_EXPANDER_BOX_WIDGET (self),
    "z-configure");
}

/**
 * Sets up the PluginStripExpanderWidget.
 */
void
plugin_strip_expander_widget_setup (
  PluginStripExpanderWidget * self,
  PluginStripExpanderType     type,
  Track *                     track)
{
  self->track = track;
  self->type = type;

  /* set name and icon */
  char fullstr[200];
  switch (type)
    {
    case PSE_TYPE_INSERTS:
      strcpy (fullstr, _("Inserts"));
      break;
    case PSE_TYPE_MIDI_EFFECTS:
      strcpy (fullstr, "MIDI FX");
      break;
    case PSE_TYPE_MODULATORS:
      strcpy (fullstr, _("Modulators"));
      break;
    default:
      g_return_if_reached ();
      break;
    }
  expander_box_widget_set_label (
    Z_EXPANDER_BOX_WIDGET (self),
    fullstr);

  set_icon_from_type (self, type);

  /* remove children */
  /*two_col_expander_box_widget_remove_children (*/
    /*Z_TWO_COL_EXPANDER_BOX_WIDGET (self));*/

  Channel * ch =
    track_get_channel (track);
  g_return_if_fail (ch);
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      GtkBox * strip_box =
        GTK_BOX (
          gtk_box_new (
            GTK_ORIENTATION_HORIZONTAL, 0));
      self->strip_boxes[i] = strip_box;

      switch (type)
        {
        case PSE_TYPE_INSERTS:
          {
            ChannelSlotWidget * csw =
              channel_slot_widget_new (i, ch);
            self->inserts[i] = csw;
            gtk_box_pack_start (
              strip_box, GTK_WIDGET (csw), 1, 1, 0);
          }
          break;
        default:
          g_warn_if_reached ();
          break;
        }

      gtk_box_pack_start (
        self->box,
        GTK_WIDGET (strip_box), 0, 1, 0);
      gtk_widget_show_all (
        GTK_WIDGET (strip_box));
    }

  plugin_strip_expander_widget_refresh (self);
}

static void
plugin_strip_expander_widget_class_init (
  PluginStripExpanderWidgetClass * klass)
{
}

static void
plugin_strip_expander_widget_init (
  PluginStripExpanderWidget * self)
{
  self->scroll =
    GTK_SCROLLED_WINDOW (
      gtk_scrolled_window_new (
        NULL, NULL));
  gtk_widget_set_vexpand (
    GTK_WIDGET (self->scroll), 1);
  gtk_widget_set_visible (
    GTK_WIDGET (self->scroll), 1);
  gtk_scrolled_window_set_shadow_type (
    self->scroll, GTK_SHADOW_ETCHED_IN);
  gtk_widget_set_size_request (
    GTK_WIDGET (self->scroll), -1, 124);

  self->viewport =
    GTK_VIEWPORT (
      gtk_viewport_new (NULL, NULL));
  gtk_widget_set_visible (
    GTK_WIDGET (self->viewport), 1);
  gtk_container_add (
    GTK_CONTAINER (self->scroll),
    GTK_WIDGET (self->viewport));

  self->box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_widget_set_visible (
    GTK_WIDGET (self->box), 1);
  gtk_container_add (
    GTK_CONTAINER (self->viewport),
    GTK_WIDGET (self->box));

  expander_box_widget_add_content (
    Z_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->scroll));
}
