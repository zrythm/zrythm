/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright 2007-2016 David Robillard <http://drobilla.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "audio/engine.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin_manager.h"
#include "plugins/lv2/lv2_control.h"
#include "plugins/lv2/lv2_ui.h"
#include "project.h"
#include "zrythm_app.h"

#include <lilv/lilv.h>

#include <gtk/gtk.h>

/* TODO all this should be merged with Port, or
 * make a native Zrythm "Control" as an optional
 * member under Port. */

int
scale_point_cmp (
  const void * _a,
  const void * _b)
{
  const Lv2ScalePoint * a =
    (Lv2ScalePoint const *) _a;
  const Lv2ScalePoint * b =
    (Lv2ScalePoint const *) _b;
  return a->value - b->value;
}

Lv2Control*
lv2_new_port_control (
  Lv2Plugin* plugin,
  uint32_t index)
{
  Lv2Port * port  = &plugin->ports[index];
  const LilvPort * lport = port->lilv_port;
  const LilvPlugin * plug  = plugin->lilv_plugin;
  const Lv2Nodes * nodes = &PM_LILV_NODES;

  Lv2Control * id =
    calloc(1, sizeof (Lv2Control));
  id->plugin = plugin;
  id->type = PORT;
  id->node =
    lilv_node_duplicate (
      lilv_port_get_node (plug, lport));
  id->symbol =
    lilv_node_duplicate (
      lilv_port_get_symbol (plug, lport));
  id->label = lilv_port_get_name (plug, lport);
  id->index = index;
  id->port = port;
  id->group =
    lilv_port_get (
      plug, lport, nodes->pg_group);
  id->value_type = plugin->forge.Float;
  id->is_writable =
    lilv_port_is_a (
      plug, lport, nodes->core_InputPort);
  id->is_readable =
    lilv_port_is_a (
      plug, lport, nodes->core_OutputPort);
  id->is_toggle =
    lilv_port_has_property (
      plug, lport, nodes->core_toggled);
  id->is_integer =
    lilv_port_has_property (
      plug, lport, nodes->core_integer);
  id->is_enumeration =
    lilv_port_has_property (
      plug, lport, nodes->core_enumeration);
  id->is_logarithmic =
    lilv_port_has_property (
      plug, lport, nodes->pprops_logarithmic);

  lilv_port_get_range (
    plug, lport, &id->def, &id->min, &id->max);
  id->maxf = lilv_node_as_float(id->max);
  id->minf = lilv_node_as_float(id->min);
  id->deff = lilv_node_as_float(id->def);
  if (lilv_port_has_property (
        plug, lport, nodes->core_sampleRate))
    {
      /* Adjust range for lv2:sampleRate
       * controls */
      if (lilv_node_is_float (id->min) ||
          lilv_node_is_int(id->min))
        {
            const float min =
              lilv_node_as_float(id->min) *
              AUDIO_ENGINE->sample_rate;
            lilv_node_free (id->min);
            id->min =
              lilv_new_float (LILV_WORLD, min);
        }
      if (lilv_node_is_float (id->max) ||
          lilv_node_is_int(id->max))
        {
            const float max =
              lilv_node_as_float(id->max) *
              AUDIO_ENGINE->sample_rate;
            lilv_node_free (id->max);
            id->max =
              lilv_new_float (LILV_WORLD, max);
        }
    }

  /* Get scale points */
  LilvScalePoints * sp =
    lilv_port_get_scale_points(plug, lport);
  if (sp)
    {
      id->points =
        (Lv2ScalePoint*)malloc (
          lilv_scale_points_size(sp) *
          sizeof(Lv2ScalePoint));
      size_t np = 0;
      LILV_FOREACH (scale_points, s, sp)
        {
            const LilvScalePoint* p =
              lilv_scale_points_get(sp, s);
            if (lilv_node_is_float (
                  lilv_scale_point_get_value(p))
                ||
                lilv_node_is_int (
                  lilv_scale_point_get_value(p)))
              {
                  id->points[np].value =
                    lilv_node_as_float(
                      lilv_scale_point_get_value (
                        p));
                  id->points[np].label =
                    strdup (
                      lilv_node_as_string (
                        lilv_scale_point_get_label(
                          p)));
                  ++np;
            }
            /* TODO: Non-float scale points? */
        }

      qsort (id->points, np, sizeof(Lv2ScalePoint),
             scale_point_cmp);
      id->n_points = np;

      lilv_scale_points_free(sp);
    }

  return id;
}

static bool
has_range(Lv2Plugin* plugin, const LilvNode* subject, const char* range_uri)
{
  LilvNode * range =
    lilv_new_uri(LILV_WORLD, range_uri);
  const bool result =
    lilv_world_ask (
      LILV_WORLD, subject,
      PM_LILV_NODES.rdfs_range, range);
  lilv_node_free(range);
  return result;
}

Lv2Control*
lv2_new_property_control (
  Lv2Plugin *      plugin,
  const LilvNode * property)
{
  Lv2Control* id = calloc (1, sizeof (Lv2Control));
  id->plugin = plugin;
  id->type = PROPERTY;
  id->node = lilv_node_duplicate (property);
  id->symbol =
    lilv_world_get_symbol (LILV_WORLD, property);
  id->label =
    lilv_world_get (LILV_WORLD,
                    property,
                    PM_LILV_NODES.rdfs_label, NULL);
  id->property =
    plugin->map.map (
      plugin, lilv_node_as_uri(property));

  id->min =
    lilv_world_get (
      LILV_WORLD, property,
      PM_LILV_NODES.core_minimum, NULL);
  id->max =
    lilv_world_get (
      LILV_WORLD, property,
      PM_LILV_NODES.core_maximum, NULL);
  id->def =
    lilv_world_get (
      LILV_WORLD, property,
      PM_LILV_NODES.core_default, NULL);

  const char* const types[] = {
    LV2_ATOM__Int,
    LV2_ATOM__Long,
    LV2_ATOM__Float,
    LV2_ATOM__Double,
    LV2_ATOM__Bool,
    LV2_ATOM__String,
    LV2_ATOM__Path,
    LV2_ATOM__URI,
    NULL
  };

  for (const char*const* t = types; *t; ++t) {
    if (has_range (plugin, property, *t))
      {
        id->value_type =
          plugin->map.map (plugin, *t);
        break;
      }
  }

  id->is_toggle  =
    id->value_type == plugin->forge.Bool;
  id->is_integer =
    (id->value_type == plugin->forge.Int ||
    id->value_type == plugin->forge.Long);
  id->is_uri =
    id->value_type == plugin->forge.URI;

  if (!id->value_type)
    {
      g_warning (
        "Unknown value type for property <%s>",
        lilv_node_as_string (property));
    }

  return id;
}

void
lv2_add_control(Lv2Controls* controls, Lv2Control* control)
{
  controls->controls =
    (Lv2Control**) realloc (
      controls->controls,
      (controls->n_controls + 1) *
        sizeof(Lv2Control*));
  controls->controls[controls->n_controls++] =
    control;
}

Lv2Control*
lv2_get_property_control (
  const Lv2Controls* controls,
  LV2_URID property)
{
  for (int i = 0; i < controls->n_controls; ++i) {
    if (controls->controls[i]->property == property) {
      return controls->controls[i];
    }
  }

  return NULL;
}

/**
 * Called when a generic UI control changes.
 */
void
lv2_control_set_control (
  const Lv2Control* control,
  uint32_t          size,
  LV2_URID          type,
  const void*       body)
{
  /*g_message ("lv2_control_set_control");*/
  Lv2Plugin* plugin = control->plugin;
  if (control->type == PORT &&
      type == plugin->forge.Float)
    {
      Lv2Port* port =
        &control->plugin->ports[control->index];
      g_return_if_fail (port->port);
      port->port->control = *(float*)body;
      /*g_message ("set to %f",*/
                 /*port->control);*/
    }
  else if (control->type == PROPERTY)
    {
      // Copy forge since it is used by process thread
      LV2_Atom_Forge       forge = plugin->forge;
      LV2_Atom_Forge_Frame frame;
      uint8_t              buf[1024];
      lv2_atom_forge_set_buffer(&forge, buf, sizeof(buf));

      lv2_atom_forge_object(&forge, &frame, 0, PM_URIDS.patch_Set);
      lv2_atom_forge_key(&forge, PM_URIDS.patch_property);
      lv2_atom_forge_urid(&forge, control->property);
      lv2_atom_forge_key(&forge, PM_URIDS.patch_value);
      lv2_atom_forge_atom(&forge, size, type);
      lv2_atom_forge_write(&forge, body, size);

      const LV2_Atom* atom =
        lv2_atom_forge_deref (
          &forge, frame.ref);
      lv2_ui_send_event_from_ui_to_plugin (
        plugin, plugin->control_in,
        lv2_atom_total_size(atom),
        PM_URIDS.atom_eventTransfer, atom);
    }
}

/**
 * Returns the human readable control label.
 */
const char *
lv2_control_get_label (
  const Lv2Control * control)
{
  return lilv_node_as_string (
    control->label);
}

/**
 * Returns the Lv2Control from the port index.
 */
Lv2Control *
lv2_control_get_from_port (
  Lv2Port * port)
{
  /* get the plugin. if loading a project, plugin
   * will be null so use the indices */
  Plugin * pl = port_get_plugin (port->port, 1);
  if (!pl)
    {
      PluginIdentifier * pl_id =
        &port->port->id.plugin_id;
      switch (pl_id->slot_type)
        {
        case PLUGIN_SLOT_MIDI_FX:
          pl =
            TRACKLIST->tracks[
              port->port->id.track_pos]->
                channel->midi_fx[pl_id->slot];
          break;
        case PLUGIN_SLOT_INSTRUMENT:
          pl =
            TRACKLIST->tracks[
              port->port->id.track_pos]->
                channel->instrument;
          break;
        case PLUGIN_SLOT_INSERT:
          pl =
            TRACKLIST->tracks[
              port->port->id.track_pos]->
                channel->inserts[pl_id->slot];
          break;
        }
    }

  Lv2Plugin * plgn =
    pl->lv2;
  Lv2Controls * ctrls =
    &plgn->controls;
  for (int i = 0; i < ctrls->n_controls; i++)
    if (ctrls->controls[i]->port ==
          port)
      return ctrls->controls[i];

  g_warn_if_reached ();
  return NULL;
}
