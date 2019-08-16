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

#include "audio/engine.h"
#include "audio/mixer.h"
#include "audio/port.h"
#include "audio/track.h"
#include "gui/widgets/editable_label.h"
#include "gui/widgets/ports_expander.h"
#include "gui/widgets/port_connections_button.h"
#include "plugins/plugin.h"
#include "project.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (PortsExpanderWidget,
               ports_expander_widget,
               TWO_COL_EXPANDER_BOX_WIDGET_TYPE)

/**
 * Refreshes each field.
 */
void
ports_expander_widget_refresh (
  PortsExpanderWidget * self)
{
  /* set visibility */
  if (self->owner_type == PORT_OWNER_TYPE_PLUGIN)
    {
      if (self->type == TYPE_CONTROL)
        {
          if (self->flow == FLOW_INPUT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self),
                self->plugin->descr->
                  num_ctrl_ins > 0);
            }
          else if (self->flow == FLOW_OUTPUT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self),
                self->plugin->descr->
                  num_ctrl_outs > 0);
            }
        }
      else if (self->type == TYPE_AUDIO)
        {
          if (self->flow == FLOW_INPUT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self),
                self->plugin->descr->
                  num_audio_ins > 0);
            }
          else if (self->flow == FLOW_OUTPUT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self),
                self->plugin->descr->
                  num_audio_outs > 0);
            }
        }
      else if (self->type == TYPE_EVENT)
        {
          if (self->flow == FLOW_INPUT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self),
                self->plugin->descr->
                  num_midi_ins > 0);
            }
          else if (self->flow == FLOW_OUTPUT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self),
                self->plugin->descr->
                  num_midi_outs > 0);
            }
        }
      else if (self->type == TYPE_CV)
        {
          if (self->flow == FLOW_INPUT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self),
                self->plugin->descr->
                  num_cv_ins > 0);
            }
          else if (self->flow == FLOW_OUTPUT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self),
                self->plugin->descr->
                  num_cv_outs > 0);
            }
        }
    }
}

/**
 * Sets up the PortsExpanderWidget for a Plugin.
 */
void
ports_expander_widget_setup_plugin (
  PortsExpanderWidget * self,
  PortFlow      flow,
  PortType      type,
  Plugin *      pl)
{
  self->plugin = pl;
  self->flow = flow;
  self->type = type;
  self->owner_type = PORT_OWNER_TYPE_PLUGIN;

  /* set name and icon */
  char * full_str = NULL;

  if (type == TYPE_CONTROL &&
      flow == FLOW_INPUT)
    full_str = g_strdup (_("Control Ins"));
  else if (type == TYPE_CONTROL &&
      flow == FLOW_OUTPUT)
    full_str = g_strdup (_("Control Outs"));
  else if (type == TYPE_AUDIO &&
      flow == FLOW_INPUT)
    full_str = g_strdup (_("Audio Ins"));
  else if (type == TYPE_AUDIO &&
      flow == FLOW_OUTPUT)
    full_str = g_strdup (_("Audio Outs"));
  else if (type == TYPE_EVENT &&
      flow == FLOW_INPUT)
    full_str = g_strdup (_("MIDI Ins"));
  else if (type == TYPE_EVENT &&
      flow == FLOW_OUTPUT)
    full_str = g_strdup (_("MIDI Outs"));
  else if (type == TYPE_CV &&
      flow == FLOW_INPUT)
    full_str = g_strdup (_("CV Ins"));
  else if (type == TYPE_CV &&
      flow == FLOW_OUTPUT)
    full_str = g_strdup (_("CV Outs"));

  expander_box_widget_set_label (
    Z_EXPANDER_BOX_WIDGET (self),
    full_str);
  g_free (full_str);

  if (type == TYPE_AUDIO)
    expander_box_widget_set_icon_resource (
      Z_EXPANDER_BOX_WIDGET (self),
      ICON_TYPE_ZRYTHM,
      "audio.svg");
  else if (type == TYPE_CV)
    expander_box_widget_set_icon_resource (
      Z_EXPANDER_BOX_WIDGET (self),
      ICON_TYPE_ZRYTHM,
      "cv.svg");
  else if (type == TYPE_EVENT)
    expander_box_widget_set_icon_name (
      Z_EXPANDER_BOX_WIDGET (self),
      "z-audio-midi");
  else if (type == TYPE_CONTROL)
    expander_box_widget_set_icon_name (
      Z_EXPANDER_BOX_WIDGET (self),
      "z-step_object_Controller");

  /* readd ports */
  two_col_expander_box_widget_remove_children (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self));

  PortConnectionsButtonWidget * pcb;
  Port * port;
  if (type == TYPE_CONTROL &&
      flow == FLOW_INPUT)
    {
      for (int i = 0; i < pl->num_in_ports; i++)
        {
          port = pl->in_ports[i];
          if (port->identifier.type
                != TYPE_CONTROL)
            continue;

          pcb =
            port_connections_button_widget_new (
              port);

          two_col_expander_box_widget_add_single (
            Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
            GTK_WIDGET (pcb));
        }
    }
  else if (type == TYPE_CONTROL &&
      flow == FLOW_OUTPUT)
    {
      for (int i = 0; i < pl->num_out_ports; i++)
        {
          port = pl->out_ports[i];
          if (port->identifier.type
                != TYPE_CONTROL)
            continue;

          pcb =
            port_connections_button_widget_new (
              port);

          two_col_expander_box_widget_add_single (
            Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
            GTK_WIDGET (pcb));
        }
    }
  else if (type == TYPE_CV &&
      flow == FLOW_INPUT)
    {
      for (int i = 0; i < pl->num_in_ports; i++)
        {
          port = pl->in_ports[i];
          if (port->identifier.type
                != TYPE_CV)
            continue;

          pcb =
            port_connections_button_widget_new (
              port);

          two_col_expander_box_widget_add_single (
            Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
            GTK_WIDGET (pcb));
        }
    }
  else if (type == TYPE_CV &&
      flow == FLOW_OUTPUT)
    {
      for (int i = 0; i < pl->num_out_ports; i++)
        {
          port = pl->out_ports[i];
          if (port->identifier.type
                != TYPE_CV)
            continue;

          pcb =
            port_connections_button_widget_new (
              port);

          two_col_expander_box_widget_add_single (
            Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
            GTK_WIDGET (pcb));
        }
    }
  else if (type == TYPE_AUDIO &&
      flow == FLOW_INPUT)
    {
      for (int i = 0; i < pl->num_in_ports; i++)
        {
          port = pl->in_ports[i];
          if (port->identifier.type
                != TYPE_AUDIO)
            continue;

          pcb =
            port_connections_button_widget_new (
              port);

          two_col_expander_box_widget_add_single (
            Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
            GTK_WIDGET (pcb));
        }
    }
  else if (type == TYPE_AUDIO &&
      flow == FLOW_OUTPUT)
    {
      for (int i = 0; i < pl->num_out_ports; i++)
        {
          port = pl->out_ports[i];
          if (port->identifier.type
                != TYPE_AUDIO)
            continue;

          pcb =
            port_connections_button_widget_new (
              port);

          two_col_expander_box_widget_add_single (
            Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
            GTK_WIDGET (pcb));
        }
    }
  else if (type == TYPE_EVENT &&
      flow == FLOW_INPUT)
    {
      for (int i = 0; i < pl->num_in_ports; i++)
        {
          port = pl->in_ports[i];
          if (port->identifier.type
                != TYPE_EVENT)
            continue;

          pcb =
            port_connections_button_widget_new (
              port);

          two_col_expander_box_widget_add_single (
            Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
            GTK_WIDGET (pcb));
        }
    }
  else if (type == TYPE_EVENT &&
      flow == FLOW_OUTPUT)
    {
      for (int i = 0; i < pl->num_out_ports; i++)
        {
          port = pl->out_ports[i];
          if (port->identifier.type
                != TYPE_EVENT)
            continue;

          pcb =
            port_connections_button_widget_new (
              port);

          two_col_expander_box_widget_add_single (
            Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
            GTK_WIDGET (pcb));
        }
    }

  ports_expander_widget_refresh (
    self);
}

/**
 * Sets up the PortsExpanderWidget for Track sends.
 *
 * @param prefader 1 for pre-fader, 0 for
 *   post-fader.
 */
void
ports_expander_widget_setup_sends (
  PortsExpanderWidget * self,
  Track *       tr,
  int           prefader)
{
  self->track = tr;
  self->owner_type = PORT_OWNER_TYPE_FADER;

  expander_box_widget_set_label (
    Z_EXPANDER_BOX_WIDGET (self),
    prefader ?
    _("Pre-Fader Sends") :
    _("Post-Fader Sends"));

  expander_box_widget_set_icon_resource (
    Z_EXPANDER_BOX_WIDGET (self),
    ICON_TYPE_ZRYTHM,
    "audio.svg");

  /* readd ports */
  two_col_expander_box_widget_remove_children (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self));

  PortConnectionsButtonWidget * pcb;
  Port * port;
  if (prefader)
    {
      pcb =
        port_connections_button_widget_new (
          tr->channel->prefader.stereo_out->l);
      two_col_expander_box_widget_add_single (
        Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
        GTK_WIDGET (pcb));

      pcb =
        port_connections_button_widget_new (
          tr->channel->prefader.stereo_out->r);
      two_col_expander_box_widget_add_single (
        Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
        GTK_WIDGET (pcb));
    }
  else
    {
      pcb =
        port_connections_button_widget_new (
          tr->channel->fader.stereo_out->l);
      two_col_expander_box_widget_add_single (
        Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
        GTK_WIDGET (pcb));

      pcb =
        port_connections_button_widget_new (
          tr->channel->fader.stereo_out->r);
      two_col_expander_box_widget_add_single (
        Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
        GTK_WIDGET (pcb));
    }
}

static void
ports_expander_widget_class_init (
  PortsExpanderWidgetClass * klass)
{
}

static void
ports_expander_widget_init (
  PortsExpanderWidget * self)
{
  /*two_col_expander_box_widget_add_single (*/
    /*Z_TWO_COL_EXPANDER_BOX_WIDGET (self),*/
    /*GTK_WIDGET (self->name));*/
}

