// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notices:
 *
 * ---
 *
  Copyright 2007-2016 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

  ---

 * Copyright (C) 2008-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2008-2017 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2008-2019 David Robillard <d@drobilla.net>
 * Copyright (C) 2012-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2013-2018 John Emmas <john@creativepost.co.uk>
 * Copyright (C) 2013 Michael R. Fisher <mfisher@bketech.com>
 * Copyright (C) 2014-2016 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2016-2017 Damien Zammit <damien@zamaudio.com>
 * Copyright (C) 2016 Nick Mainsbridge <mainsbridge@gmail.com>
 * Copyright (C) 2017 Johannes Mueller <github@johannes-mueller.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---
*/

#include "zrythm-config.h"

/* for dlinfo */
#define _GNU_SOURCE
#if !defined(_WOE32) && !defined(__APPLE__)
#  include <dlfcn.h>
#  include <link.h>
#endif

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "dsp/engine.h"
#include "dsp/midi_event.h"
#include "dsp/tempo_track.h"
#include "dsp/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/lv2/lv2_evbuf.h"
#include "plugins/lv2/lv2_gtk.h"
#include "plugins/lv2/lv2_log.h"
#include "plugins/lv2/lv2_state.h"
#include "plugins/lv2/lv2_ui.h"
#include "plugins/lv2/lv2_worker.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <lilv/lilv.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/event/event.h>
#include <lv2/instance-access/instance-access.h>
#include <lv2/midi/midi.h>
#include <lv2/patch/patch.h>
#include <lv2/port-groups/port-groups.h>
#include <lv2/port-props/port-props.h>
#include <lv2/presets/presets.h>
#include <lv2/resize-port/resize-port.h>
#include <lv2/time/time.h>
#include <lv2/units/units.h>
#include <sratom/sratom.h>
#include <suil/suil.h>
#include <unistd.h>

typedef enum
{
  Z_PLUGINS_LV2_PLUGIN_ERROR_FAILED,
  Z_PLUGINS_LV2_PLUGIN_ERROR_CREATION_FAILED,
  Z_PLUGINS_LV2_PLUGIN_ERROR_INSTANTIATION_FAILED,
  Z_PLUGINS_LV2_PLUGIN_ERROR_NO_UI,
  Z_PLUGINS_LV2_PLUGIN_ERROR_CANNOT_FIND_URI,
} ZPluginsLv2PluginError;

#define Z_PLUGINS_LV2_PLUGIN_ERROR z_plugins_lv2_plugin_error_quark ()
GQuark
z_plugins_lv2_plugin_error_quark (void);
G_DEFINE_QUARK (
  z - plugins - lv2 - plugin - error - quark,
  z_plugins_lv2_plugin_error)

/**
 * Size factor for UI ring buffers.  The ring size
 * is a few times the size of an event output to
 * give the UI a chance to keep up.  Experiments
 * with Ingen, which can highly saturate its event
 * output, led me to this value (4).  It really
 * ought to be enough for anybody(TM).
 *
 * The number of MIDI buffers that will fit in a
 * UI/worker comm buffer.
 * This needs to be roughly the number of cycles
 * the UI will get around to actually processing
 * the traffic.  Lower values are flakier but save
 * memory.
 */
#define N_BUFFER_CYCLES 4

/* pulled by X11 */
#undef Bool

/**
 * Returns whether Zrythm supports the given
 * option.
 */
static bool
option_is_supported (Lv2Plugin * self, const char * uri)
{
  size_t num_opts = sizeof (self->options) / sizeof (self->options[0]);
  for (size_t i = 0; i < num_opts; i++)
    {
      LV2_Options_Option * opt = &self->options[i];
      if (lv2_urid_map_uri (self, uri) == opt->key)
        {
          return true;
        }
    }
  return false;
}

/**
 * Returns whether Zrythm supports the given
 * feature.
 */
static bool
feature_is_supported (Lv2Plugin * self, const char * uri)
{
  if (string_is_equal (uri, LV2_CORE__isLive))
    return true;

  /* handled by lilv */
  if (string_is_equal (uri, LV2_STATE__mapPath))
    return true;

  for (const LV2_Feature * const * f = self->features; *f; ++f)
    {
      if (string_is_equal (uri, (*f)->URI))
        {
          return true;
        }
    }
  return false;
}

static char *
get_port_group_label (const LilvNode * group)
{
  LilvNode * group_label = lilv_world_get (
    LILV_WORLD, group, PM_GET_NODE (LILV_NS_RDFS "label"), NULL);
  char * ret = NULL;
  if (group_label)
    {
      const char * group_name_str = lilv_node_as_string (group_label);
      ret = g_strdup (group_name_str);
      lilv_node_free (group_label);
    }

  return ret;
}

static bool
param_has_range (
  Lv2Plugin *      plugin,
  const LilvNode * subject,
  const char *     range_uri)
{
  LilvNode * range = lilv_new_uri (LILV_WORLD, range_uri);
  const bool result = lilv_world_ask (
    LILV_WORLD, subject, PM_GET_NODE (LILV_NS_RDFS "range"), range);
  lilv_node_free (range);

  return result;
}

/**
 * Create a port structure from data description.
 *
 * This is called before plugin and Jack
 * instantiation. The remaining instance-specific
 * setup (e.g. buffers) is done later in
 * connect_port().
 *
 * @param param If non-null, this port is for a
 *   parameter (set via atom messages).
 * @param port_exists If zrythm port exists (when
 *   loading)
 *
 * @return Non-zero if fail.
 */
static Port *
create_port (
  Lv2Plugin *    self,
  const uint32_t lv2_port_index,
  Lv2Parameter * param,
  const float    default_value,
  const int      port_exists)
{
  Plugin *           pl = self->plugin;
  bool               is_project = lv2_plugin_is_in_active_project (self);
  const LilvPlugin * lilv_plugin = self->lilv_plugin;
  LV2_Atom_Forge *   forge = &self->main_forge;
  Port *             port = NULL;
  if (port_exists && is_project)
    {
      if (param)
        {
          port = plugin_get_port_by_param_uri (
            pl, lv2_urid_unmap_uri (self, param->urid));
        }
      else
        {
          const LilvPort * lilv_port =
            lilv_plugin_get_port_by_index (lilv_plugin, lv2_port_index);
          const LilvNode * sym_node =
            lilv_port_get_symbol (lilv_plugin, lilv_port);
          const char * sym = lilv_node_as_string (sym_node);
          port = plugin_get_port_by_symbol (pl, sym);
        }
      g_return_val_if_fail (port, NULL);
    }

  if (param)
    {
      if (!port_exists || !is_project)
        {
          port = port_new_with_type (TYPE_CONTROL, FLOW_INPUT, param->label);
        }
      g_return_val_if_fail (port, NULL);
      PortIdentifier * pi = &port->id;

      pi->uri = g_strdup (lv2_urid_unmap_uri (NULL, param->urid));
      pi->flags |= PORT_FLAG_IS_PROPERTY;
      pi->sym = g_strdup (param->symbol);
      g_return_val_if_fail (IS_PORT_AND_NONNULL (port), NULL);
      port->value_type = param->value_type_urid;
      pi->comment = g_strdup (param->comment);

      if (param->has_range)
        {
          port->minf = param->minf;
          port->maxf = param->maxf;
          port->deff = param->deff;
        }

      if (param->value_type_urid == forge->Bool)
        {
          pi->flags |= PORT_FLAG_TOGGLE;
        }
      if (
        param->value_type_urid == forge->Int
        || param->value_type_urid == forge->Long)
        {
          pi->flags |= PORT_FLAG_INTEGER;
        }
      if (param->value_type_urid == forge->URI)
        {
          pi->flags2 |= PORT_FLAG2_URI_PARAM;
        }

      if (
        param->value_type_urid == forge->Int
        || param->value_type_urid == forge->Long
        || param->value_type_urid == forge->Float
        || param->value_type_urid == forge->Double
        || param->value_type_urid == forge->Bool)
        {
          pi->flags |= PORT_FLAG_AUTOMATABLE;
        }

      port->lilv_port_index = -1;
    }
  else
    {
      const LilvPort * lilv_port =
        lilv_plugin_get_port_by_index (lilv_plugin, lv2_port_index);
      const LilvNode * lilv_port_node =
        lilv_port_get_node (lilv_plugin, lilv_port);

      LilvNode *def, *min, *max;
      lilv_port_get_range (lilv_plugin, lilv_port, &def, &min, &max);

#define PORT_IS_A(x) lilv_port_is_a (lilv_plugin, lilv_port, PM_GET_NODE (x))

      bool is_atom = PORT_IS_A (LV2_ATOM__AtomPort);
      bool is_control = PORT_IS_A (LV2_CORE__ControlPort);
      bool is_input = PORT_IS_A (LV2_CORE__InputPort);
      bool is_output = PORT_IS_A (LV2_CORE__OutputPort);
      bool is_audio = PORT_IS_A (LV2_CORE__AudioPort);
      bool is_cv = PORT_IS_A (LV2_CORE__CVPort);
      bool is_event = PORT_IS_A (LV2_EVENT__EventPort);

#undef PORT_IS_A

      LilvNode *   name_node = lilv_port_get_name (lilv_plugin, lilv_port);
      const char * name_str = lilv_node_as_string (name_node);
      if (port_exists && is_project)
        {
          g_return_val_if_fail (IS_PORT_AND_NONNULL (port), NULL);
          port->buf = g_realloc (
            port->buf, (size_t) AUDIO_ENGINE->block_length * sizeof (float));
        }
      else
        {
          PortType type = 0;
          PortFlow flow = 0;

          /* Set the lv2_port flow (input or output) */
          if (is_input)
            {
              flow = FLOW_INPUT;
            }
          else if (is_output)
            {
              flow = FLOW_OUTPUT;
            }

          /* Set type */
          if (is_control)
            {
              type = TYPE_CONTROL;
            }
          else if (is_audio)
            {
              type = TYPE_AUDIO;
            }
          else if (is_cv)
            {
              type = TYPE_CV;
            }
          else if (is_event || is_atom)
            {
              type = TYPE_EVENT;
            }

          port = port_new_with_type_and_owner (
            type, flow, name_str, PORT_OWNER_TYPE_PLUGIN, self->plugin);
        }
      lilv_node_free (name_node);

      g_return_val_if_fail (IS_PORT_AND_NONNULL (port), NULL);
      port->evbuf = NULL;
      port->lilv_port_index = (int) lv2_port_index;
      port->control = 0.0f;
      port->unsnapped_control = 0.0f;
      port->value_type = forge->Float;

#define HAS_PROPERTY(x) \
  lilv_port_has_property (lilv_plugin, lilv_port, PM_GET_NODE (x))

      const bool optional = HAS_PROPERTY (LV2_CORE__connectionOptional);

      PortIdentifier * pi = &port->id;

      /* set the symbol */
      const LilvNode * sym_node = lilv_port_get_symbol (lilv_plugin, lilv_port);
      pi->sym = g_strdup (lilv_node_as_string (sym_node));

      /* set the comment */
      LilvNode * comment = lilv_world_get (
        LILV_WORLD, lilv_port_node, PM_GET_NODE (LILV_NS_RDFS "comment"), NULL);
      pi->comment = g_strdup (lilv_node_as_string (comment));
      lilv_node_free (comment);

      /* Set the lv2_port flow (input or output) */
      if (is_input)
        {
          pi->flow = FLOW_INPUT;
        }
      else if (is_output)
        {
          pi->flow = FLOW_OUTPUT;
        }
      else if (!optional)
        {
          g_warning (
            "Mandatory lv2_port at %d has "
            "unknown type"
            " (neither input nor output)",
            lv2_port_index);
          return NULL;
        }

      /* Set control values */
      if (is_control)
        {
          pi->type = TYPE_CONTROL;
          port->control = isnan (default_value) ? 0.0f : default_value;
          port->unsnapped_control = port->control;

          /* set port flags */
          if (HAS_PROPERTY (LV2_PORT_PROPS__trigger))
            {
              pi->flags |= PORT_FLAG_TRIGGER;
            }
          if (HAS_PROPERTY (LV2_CORE__freeWheeling))
            {
              pi->flags |= PORT_FLAG_FREEWHEEL;
            }
          if (self->enabled_in == (int) lv2_port_index)
            {
              pi->flags |= PORT_FLAG_PLUGIN_ENABLED;
              pl->own_enabled_port = port;
            }
          if (HAS_PROPERTY (LV2_CORE__reportsLatency))
            {
              pi->flags |= PORT_FLAG_REPORTS_LATENCY;
            }
          if (HAS_PROPERTY (LV2_PORT_PROPS__logarithmic))
            {
              pi->flags |= PORT_FLAG_LOGARITHMIC;
            }
          if (HAS_PROPERTY (LV2_CORE__enumeration))
            {
              pi->flags2 |= PORT_FLAG2_ENUMERATION;
            }
          if (HAS_PROPERTY (LV2_CORE__toggled))
            {
              pi->flags |= PORT_FLAG_TOGGLE;
            }
          else if (HAS_PROPERTY (LV2_CORE__integer))
            {
              pi->flags |= PORT_FLAG_INTEGER;
            }

          /* TODO enumeration / scale points */

          if (pi->flow == FLOW_INPUT && !HAS_PROPERTY (LV2_CORE__freeWheeling))
            {
              pi->flags |= PORT_FLAG_AUTOMATABLE;
            }

          port->maxf = 1.f;
          port->minf = 0.f;
          if (max)
            {
              port->maxf = lilv_node_as_float (max);
            }
          if (min)
            {
              port->minf = lilv_node_as_float (min);
            }
          if (def)
            {
              port->deff = lilv_node_as_float (def);
            }
          else
            {
              port->deff = port->minf;
            }
          port->zerof = port->minf;

#define FIND_OBJECTS(x) \
  lilv_world_find_nodes (LILV_WORLD, lilv_port_node, PM_GET_NODE (x), NULL)

          /* set unit */
          LilvNodes * units = FIND_OBJECTS (LV2_UNITS__unit);

#define SET_UNIT(caps, sc) \
  if (lilv_nodes_contains (units, PM_GET_NODE (LV2_UNITS__##sc))) \
    { \
      port->id.unit = PORT_UNIT_##caps; \
    }

          SET_UNIT (DB, db);
          SET_UNIT (DEGREES, degree);
          SET_UNIT (HZ, hz);
          SET_UNIT (MHZ, mhz);
          SET_UNIT (MS, ms);
          SET_UNIT (SECONDS, s);

#undef SET_UNIT

          lilv_nodes_free (units);
        }
      else if (is_audio)
        {
          pi->type = TYPE_AUDIO;
        }
      else if (is_cv)
        {
          port->maxf = 1.f;
          port->minf = -1.f;
          if (max)
            {
              port->maxf = lilv_node_as_float (max);
            }
          if (min)
            {
              port->minf = lilv_node_as_float (min);
            }
          if (def)
            {
              port->deff = lilv_node_as_float (def);
            }
          else
            {
              port->deff = port->minf;
            }
          port->zerof = 0.f;
          pi->type = TYPE_CV;
        }
      else if (is_event)
        {
          pi->type = TYPE_EVENT;
          pi->flags2 |= PORT_FLAG2_SUPPORTS_MIDI;
          if (!port->midi_events)
            {
              port->midi_events = midi_events_new ();
            }
          port->old_api = true;
        }
      else if (is_atom)
        {
          pi->type = TYPE_EVENT;
          if (!port->midi_events)
            {
              port->midi_events = midi_events_new ();
            }
          port->old_api = false;

          LilvNodes * buffer_types = lilv_port_get_value (
            lilv_plugin, lilv_port, PM_GET_NODE (LV2_ATOM__bufferType));
          LilvNodes * atom_supports = lilv_port_get_value (
            lilv_plugin, lilv_port, PM_GET_NODE (LV2_ATOM__supports));

          if (
            lilv_nodes_contains (buffer_types, PM_GET_NODE (LV2_ATOM__Sequence)))
            {
              pi->flags2 |= PORT_FLAG2_SEQUENCE;

              if (lilv_nodes_contains (
                    atom_supports, PM_GET_NODE (LV2_TIME__Position)))
                {
                  pi->flags |= PORT_FLAG_WANT_POSITION;
                  self->want_position = true;
                }
              if (lilv_nodes_contains (
                    atom_supports, PM_GET_NODE (LV2_MIDI__MidiEvent)))
                {
                  pi->flags2 |= PORT_FLAG2_SUPPORTS_MIDI;
                }
              if (lilv_nodes_contains (
                    atom_supports, PM_GET_NODE (LV2_PATCH__Message)))
                {
                  pi->flags2 |= PORT_FLAG2_SUPPORTS_PATCH_MESSAGE;
                }
            }
          lilv_nodes_free (buffer_types);
          lilv_nodes_free (atom_supports);
        }
      else if (!optional)
        {
          g_warning (
            "Mandatory lv2_port at %d has unknown "
            "data type",
            lv2_port_index);
          return NULL;
        }
      pi->flags |= PORT_FLAG_PLUGIN_CONTROL;

      if (HAS_PROPERTY (LV2_PORT_PROPS__notOnGUI))
        {
          pi->flags |= PORT_FLAG_NOT_ON_GUI;
        }

      if (HAS_PROPERTY (LV2_CORE__sampleRate))
        {
          /* Adjust range for lv2:sampleRate
           * controls */
          if (lilv_node_is_float (min) || lilv_node_is_int (min))
            {
              port->minf = lilv_node_as_float (min) * AUDIO_ENGINE->sample_rate;
            }
          if (lilv_node_is_float (max) || lilv_node_is_int (max))
            {
              port->maxf = lilv_node_as_float (max) * AUDIO_ENGINE->sample_rate;
            }
        }

      /* Get scale points */
      LilvScalePoints * sp = lilv_port_get_scale_points (lilv_plugin, lilv_port);
      if (sp)
        {
          /* clear previous scale points */
          for (int i = 0; i < port->num_scale_points; i++)
            {
              object_free_w_func_and_null (
                port_scale_point_free, port->scale_points[i]);
            }
          port->num_scale_points = 0;

          size_t num_scale_points = lilv_scale_points_size (sp);
          port->scale_points = object_new_n (num_scale_points, PortScalePoint *);

          LILV_FOREACH (scale_points, s, sp)
            {
              const LilvScalePoint * p = lilv_scale_points_get (sp, s);
              if (
                lilv_node_is_float (lilv_scale_point_get_value (p))
                || lilv_node_is_int (lilv_scale_point_get_value (p)))
                {
                  port->scale_points[port->num_scale_points++] =
                    port_scale_point_new (
                      lilv_node_as_float (lilv_scale_point_get_value (p)),
                      lilv_node_as_string (lilv_scale_point_get_label (p)));
                }

              /* TODO: Non-float scale points */
            }

          qsort (
            port->scale_points, (size_t) port->num_scale_points,
            sizeof (PortScalePoint *), port_scale_point_cmp);
          lilv_scale_points_free (sp);
        }

      if (HAS_PROPERTY (LV2_CORE__isSideChain))
        {
          g_debug ("%s is sidechain", pi->label);
          pi->flags |= PORT_FLAG_SIDECHAIN;
        }
      LilvNodes * groups = lilv_port_get_value (
        lilv_plugin, lilv_port, PM_GET_NODE (LV2_PORT_GROUPS__group));
      if (lilv_nodes_size (groups) > 0)
        {
          const LilvNode * group = lilv_nodes_get_first (groups);

          /* get the name of the port group */
          pi->port_group = get_port_group_label (group);
          g_debug ("'%s' has port group '%s'", pi->label, pi->port_group);

          /* get sideChainOf */
          LilvNode * side_chain_of = lilv_world_get (
            LILV_WORLD, group, PM_GET_NODE (LV2_PORT_GROUPS__sideChainOf), NULL);
          if (side_chain_of)
            {
              g_debug ("'%s' is sidechain", pi->label);
              lilv_node_free (side_chain_of);
              pi->flags |= PORT_FLAG_SIDECHAIN;
            }

          /* get all designations we are interested
           * in (e.g., "right") */
          LilvNodes * designations = lilv_port_get_value (
            lilv_plugin, lilv_port, PM_GET_NODE (LV2_CORE__designation));
          /* get all pg:elements of the pg:group */
          LilvNodes * group_childs = lilv_world_find_nodes (
            LILV_WORLD, group, PM_GET_NODE (LV2_PORT_GROUPS__element), NULL);
          if (lilv_nodes_size (designations) > 0)
            {
              /* iterate over all port
               * designations */
              LILV_FOREACH (nodes, i, designations)
                {
                  const LilvNode * designation =
                    lilv_nodes_get (designations, i);
                  const char * designation_str = lilv_node_as_uri (designation);
                  g_debug ("'%s' designation: <%s>", pi->label, designation_str);
                  if (lilv_node_equals (
                        designation, PM_GET_NODE (LV2_PORT_GROUPS__left)))
                    {
                      pi->flags |= PORT_FLAG_STEREO_L;
                      g_debug ("left port");
                    }
                  if (lilv_node_equals (
                        designation, PM_GET_NODE (LV2_PORT_GROUPS__right)))
                    {
                      pi->flags |= PORT_FLAG_STEREO_R;
                      g_debug ("right port");
                    }

                  if (lilv_nodes_size (group_childs) > 0)
                    {
                      /* match the lv2:designation's
                       * element against the
                       * port-group's element */
                      LILV_FOREACH (nodes, j, group_childs)
                        {
                          const LilvNode * group_el =
                            lilv_nodes_get (group_childs, j);
                          LilvNodes * elem = lilv_world_find_nodes (
                            LILV_WORLD, group_el,
                            PM_GET_NODE (LV2_CORE__designation), designation);

                          /*if found, look up the
                           * index (channel number of
                           * the pg:Element */
                          if (lilv_nodes_size (elem) > 0)
                            {
                              LilvNodes * idx = lilv_world_find_nodes (
                                LILV_WORLD, lilv_nodes_get_first (elem),
                                PM_GET_NODE (LV2_CORE__index), NULL);
                              if (lilv_node_is_int (lilv_nodes_get_first (idx)))
                                {
                                  int group_channel = lilv_node_as_int (
                                    lilv_nodes_get_first (idx));
                                  g_debug ("group chan %d", group_channel);
                                }
                            }
                        }
                    }
                }
            }
          lilv_nodes_free (designations);
          lilv_nodes_free (groups);
        }

      LilvNode * min_size = lilv_port_get (
        lilv_plugin, lilv_port, PM_GET_NODE (LV2_RESIZE_PORT__minimumSize));
      if (min_size)
        {
          if (lilv_node_is_int (min_size))
            {
              port->min_buf_size = (size_t) lilv_node_as_int (min_size);
            }
          lilv_node_free (min_size);
        }

      lilv_node_free (max);
      lilv_node_free (min);
      lilv_node_free (def);
    } /* endif control port */

#undef HAS_PROPERTY

  g_return_val_if_fail (IS_PORT_AND_NONNULL (port), NULL);

  return port;
}

void
lv2_plugin_init_loaded (Lv2Plugin * self)
{
  self->magic = LV2_PLUGIN_MAGIC;
}

/**
 * Comparator.
 */
static int
param_port_cmp (const void * _a, const void * _b)
{
  const Lv2Parameter * a = (const Lv2Parameter *) _a;
  const Lv2Parameter * b = (const Lv2Parameter *) _b;
  return (int) a->urid - (int) b->urid;
}

/**
 * Returns all writable/readable parameters
 * declared by the plugin.
 */
static void
lv2_plugin_get_parameters (Lv2Plugin * self, GArray * params_array)
{
  const LilvNode * patch_writable = PM_GET_NODE (LV2_PATCH__writable);
  const LilvNode * patch_readable = PM_GET_NODE (LV2_PATCH__readable);

  for (int i = 0; i < 2; i++)
    {
      bool writable = i == 0;

      /* get all parameters declared in the ttl
       * (myplugin:myparameter a lv2:Parameter)
       * marked as either patch:readable or
       * patch:writable */
      LilvNodes * properties = lilv_world_find_nodes (
        LILV_WORLD, lilv_plugin_get_uri (self->lilv_plugin),
        writable ? patch_writable : patch_readable, NULL);
      LILV_FOREACH (nodes, p, properties)
        {
          const LilvNode * property = lilv_nodes_get (properties, p);
          const char *     property_uri = lilv_node_as_uri (property);
          LV2_URID property_urid = lv2_urid_map_uri (NULL, property_uri);
          g_message (
            "%s parameter found: <%s>", writable ? "writable" : "readable",
            property_uri);

          bool found = false;

          /* if searching for readable params and
           * existing writable parameter exists,
           * also mark it as readable and skip
           * creation */
          if (
            !writable
            && lilv_world_ask (
              LILV_WORLD, lilv_plugin_get_uri (self->lilv_plugin),
              patch_writable, property))
            {
              for (guint j = 0; j < params_array->len; j++)
                {
                  Lv2Parameter * param =
                    &g_array_index (params_array, Lv2Parameter, j);
                  LV2_URID cur_param_urid = param->urid;
#if 0
                  const char * cur_param_uri =
                    lv2_urid_unmap_uri (
                      NULL, cur_param_urid);
                  g_message (
                    "param URI %s", cur_param_uri);
#endif

                  if (property_urid == cur_param_urid)
                    {
                      found = true;
                      param->readable = true;

                      /* break writable param
                       * search */
                      break;
                    }
                }

              if (found)
                {
#if 0
                  g_messsage (
                    "existing writable param "
                    "found for this property, "
                    "skipping creation and "
                    "continuing to next "
                    "property (if any)");
#endif

                  /* continue LILV_FOREACH */
                  continue;
                }
            }

          /* create new param */
          Lv2Parameter param;
          memset (&param, 0, sizeof (Lv2Parameter));
          param.urid = property_urid;
          param.readable = !writable;
          param.writable = writable;

          LilvNode *   symbol = lilv_world_get_symbol (LILV_WORLD, property);
          const char * symbol_str = lilv_node_as_string (symbol);
          strncpy (param.symbol, symbol_str, LV2_PARAM_MAX_STR_LEN - 1);
          lilv_node_free (symbol);

          LilvNode * label = lilv_world_get (
            LILV_WORLD, property, PM_GET_NODE (LILV_NS_RDFS "label"), NULL);
          if (label)
            {
              const char * label_str = lilv_node_as_string (label);
              strncpy (
                param.label, label_str,

                LV2_PARAM_MAX_STR_LEN - 1);
              lilv_node_free (label);
            }

          LilvNode * comment = lilv_world_get (
            LILV_WORLD, property, PM_GET_NODE (LILV_NS_RDFS "comment"), NULL);
          if (comment)
            {
              const char * comment_str = lilv_node_as_string (comment);
              strncpy (param.comment, comment_str, LV2_PARAM_MAX_STR_LEN - 1);
              lilv_node_free (comment);
            }

          LilvNode * min = lilv_world_get (
            LILV_WORLD, property, PM_GET_NODE (LV2_CORE__minimum), NULL);
          if (lilv_node_is_int (min) || lilv_node_is_float (min))
            {
              param.has_range = true;
              param.minf = lilv_node_as_float (min);
              LilvNode * max = lilv_world_get (
                LILV_WORLD, property, PM_GET_NODE (LV2_CORE__maximum), NULL);
              param.maxf = lilv_node_as_float (max);
              lilv_node_free (max);

              LilvNode * def = lilv_world_get (
                LILV_WORLD, property, PM_GET_NODE (LV2_CORE__default), NULL);
              param.deff = lilv_node_as_float (def);
              lilv_node_free (def);
            }
          else
            {
              param.has_range = false;
            }
          lilv_node_free (min);

          const char * const types[] = {
            LV2_ATOM__Int,    LV2_ATOM__Long, LV2_ATOM__Float,
            LV2_ATOM__Double, LV2_ATOM__Bool, LV2_ATOM__String,
            LV2_ATOM__Path,   LV2_ATOM__URI,  NULL
          };

          for (const char * const * t = types; *t; t++)
            {
              if (param_has_range (self, property, *t))
                {
                  param.value_type_urid = lv2_urid_map_uri (NULL, *t);
                  break;
                }
            }

          if (param.value_type_urid <= 0)
            {
              g_warning (
                "Unknown value type for property "
                "<%s>, skipping",
                lilv_node_as_string (property));
              continue;
            }

          g_array_append_val (params_array, param);
        } /* end foreach property */
      lilv_nodes_free (properties);
    } /* end loop 2 */

  /* sort parameters */
  g_array_sort (params_array, param_port_cmp);
}

/**
 * Create port structures from data (via
 * create_port()) for all ports.
 */
NONNULL static int
lv2_create_or_init_ports_and_parameters (Lv2Plugin * self)
{
  g_return_val_if_fail (
    IS_PLUGIN_AND_NONNULL (self->plugin) && self->lilv_plugin, -1);

  Plugin * pl = self->plugin;

  /* zrythm ports exist when loading a
   * project since they are serialized */
  int ports_exist =
    /* excluding enabled/gain created by zrythm */
    pl->num_in_ports > 2 || pl->num_out_ports > 0;

  GArray * params_array =
    g_array_new (F_NOT_ZERO_TERMINATED, F_CLEAR, sizeof (Lv2Parameter));
  lv2_plugin_get_parameters (self, params_array);

  int num_lilv_ports = (int) lilv_plugin_get_num_ports (self->lilv_plugin);

  if (ports_exist)
    {
#if 0
      /* TODO check number of ports is correct */
      g_warn_if_fail (
        pl->num_ports ==
        (int)
        /* lilv ports + params + zrythm provided
         * ports */
        num_lilv_ports + num_params + 2);
#endif
    }

  float * default_values = object_new_n ((size_t) num_lilv_ports, float);
  lilv_plugin_get_port_ranges_float (
    self->lilv_plugin, NULL, NULL, default_values);

  /* set control input index */
  const LilvPort * control_input = lilv_plugin_get_port_by_designation (
    self->lilv_plugin, PM_GET_NODE (LV2_CORE__InputPort),
    PM_GET_NODE (LV2_CORE__control));
  if (control_input)
    {
      self->control_in =
        (int) lilv_port_get_index (self->lilv_plugin, control_input);
    }

  pl->num_lilv_ports = num_lilv_ports;
  pl->lilv_ports = object_new_n ((size_t) num_lilv_ports, Port *);

  /* set enabled port index */
  const LilvPort * enabled_port = lilv_plugin_get_port_by_designation (
    self->lilv_plugin, PM_GET_NODE (LV2_CORE__InputPort),
    PM_GET_NODE (LV2_CORE__enabled));
  if (enabled_port)
    {
      self->enabled_in =
        (int) lilv_port_get_index (self->lilv_plugin, enabled_port);
    }

  /* create and add zrythm ports for lilv ports
   * and parameters */
  for (int i = 0; i < num_lilv_ports + (int) params_array->len; i++)
    {
      Lv2Parameter * param =
        &g_array_index (params_array, Lv2Parameter, i - num_lilv_ports);
      Port * port = create_port (
        self, (uint32_t) i, (i < num_lilv_ports) ? NULL : param,
        i < num_lilv_ports ? default_values[i] : 0, ports_exist);
      g_return_val_if_fail (IS_PORT_AND_NONNULL (port), -1);

      if (i < num_lilv_ports)
        {
          pl->lilv_ports[i] = port;
        }

      if (!ports_exist)
        {
          port->tmp_plugin = pl;
          if (port->id.flow == FLOW_INPUT)
            {
              plugin_add_in_port (pl, port);
            }
          else if (port->id.flow == FLOW_OUTPUT)
            {
              plugin_add_out_port (pl, port);
            }
          else
            {
              g_return_val_if_reached (-1);
            }
        }
    }

  free (default_values);
  g_array_free (params_array, F_FREE);

  return 0;
}

/**
 * Allocate port buffers (only necessary for MIDI).
 */
void
lv2_plugin_allocate_port_buffers (Lv2Plugin * lv2_plugin)
{
  Plugin * pl = lv2_plugin->plugin;
  g_return_if_fail (pl);
  for (int i = 0; i < pl->num_lilv_ports; ++i)
    {
      Port * port = pl->lilv_ports[i];

      if (port->id.type != TYPE_EVENT)
        continue;

      g_debug ("allocating event buf for port %s", port->id.sym);

      object_free_w_func_and_null (lv2_evbuf_free, port->evbuf);
      const size_t buf_size =
        MAX (port->min_buf_size, AUDIO_ENGINE->midi_buf_size);
      g_return_if_fail (buf_size > 0 && lv2_plugin->map.map);
      g_debug ("buf size: %zu", buf_size);

      port->evbuf = lv2_evbuf_new (
        (uint32_t) buf_size, lv2_urid_map_uri (lv2_plugin, LV2_ATOM__Chunk),
        lv2_urid_map_uri (lv2_plugin, LV2_ATOM__Sequence));
      g_return_if_fail (port->evbuf);

      /* reset the evbuf - needed if output since
       * it is reset after the run cycle in
       * lv2_plugin_process() */
      lv2_evbuf_reset (port->evbuf, port->id.flow == FLOW_INPUT);

      lilv_instance_connect_port (
        lv2_plugin->instance, (uint32_t) i, lv2_evbuf_get_buffer (port->evbuf));
    }
}

/**
 * Returns the property port matching the given
 * property URID.
 */
Port *
lv2_plugin_get_property_port (Lv2Plugin * self, LV2_URID property)
{
  Plugin * pl = self->plugin;
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * port = pl->in_ports[i];
      if (
        port->id.type != TYPE_CONTROL
        || !(port->id.flags & PORT_FLAG_IS_PROPERTY))
        continue;

      if (lv2_urid_map_uri (self, port->id.uri) == property)
        {
          return port;
        }
    }

  return NULL;
}

/**
 * Function to get a port value.
 *
 * Used when saving the state.
 * This function MUST set size and type
 * appropriately.
 */
const void *
lv2_plugin_get_port_value (
  const char * port_sym,
  void *       user_data,
  uint32_t *   size,
  uint32_t *   type)
{
  Lv2Plugin * lv2_plugin = (Lv2Plugin *) user_data;

  Plugin * pl = lv2_plugin->plugin;
  g_return_val_if_fail (IS_PLUGIN_AND_NONNULL (pl), NULL);

  Port * port = plugin_get_port_by_symbol (pl, port_sym);
  *size = 0;
  *type = 0;

  if (port)
    {
      *size = sizeof (float);
      *type = PM_URIDS.atom_Float;
      return (const void *) &port->control;
    }

  return NULL;
}

/**
 * Runs the plugin for this cycle.
 */
static int
run (Lv2Plugin * plugin, const nframes_t nframes)
{
  /* Read and apply control change events from UI */
  if (plugin->plugin->visible)
    lv2_ui_read_and_apply_events (plugin, nframes);

  /* Run plugin for this cycle */
  lilv_instance_run (plugin->instance, nframes);

  /* Process any worker replies. */
  lv2_worker_emit_responses (&plugin->state_worker, plugin->instance);
  lv2_worker_emit_responses (&plugin->worker, plugin->instance);

  /* Notify the plugin the run() cycle is
   * finished */
  if (plugin->worker.iface && plugin->worker.iface->end_run)
    {
      plugin->worker.iface->end_run (plugin->instance->lv2_handle);
    }

  bool have_custom_ui = !plugin->plugin->setting->force_generic_ui;

  /* Check if it's time to send updates to the UI */
  plugin->event_delta_t += nframes;
  bool     send_ui_updates = false;
  uint32_t update_frames =
    (uint32_t) ((float) AUDIO_ENGINE->sample_rate / plugin->plugin->ui_update_hz);
  if (
    have_custom_ui && plugin->plugin->visible
    && (plugin->event_delta_t > update_frames))
    {
      send_ui_updates = true;
      plugin->event_delta_t = 0;
    }

  return send_ui_updates;
}

/**
 * Connect the port to its buffer.
 */
NONNULL static void
connect_port (Lv2Plugin * lv2_plugin, uint32_t port_index)
{
  Plugin * pl = lv2_plugin->plugin;
  g_return_if_fail (IS_PLUGIN_AND_NONNULL (pl));

  g_return_if_fail ((int) port_index < pl->num_lilv_ports);
  Port * port = pl->lilv_ports[port_index];

  /* Connect unsupported ports to NULL (known to
   * be optional by this point) */
  if (port->id.flow == FLOW_UNKNOWN || port->id.type == TYPE_UNKNOWN)
    {
      lilv_instance_connect_port (lv2_plugin->instance, port_index, NULL);
      return;
    }

  /* Connect the port based on its type */
  switch (port->id.type)
    {
    case TYPE_CONTROL:
      lilv_instance_connect_port (
        lv2_plugin->instance, port_index, &port->control);
      break;
    case TYPE_AUDIO:
    case TYPE_CV:
      /* connect lv2 ports to plugin port
       * buffers */
      lilv_instance_connect_port (lv2_plugin->instance, port_index, port->buf);
      break;
    case TYPE_EVENT:
      /* already connected to port */
      g_return_if_fail (port->evbuf);
      break;
    default:
      break;
    }
}

/**
 * Initializes the plugin features.
 *
 * This is called on instantiation.
 */
static void
set_features (Lv2Plugin * self)
{
#define INIT_FEATURE(x, uri, _data) \
  self->x.URI = uri; \
  self->x.data = _data

  self->ext_data_feature.data_access = NULL;

  INIT_FEATURE (map_feature, LV2_URID__map, &self->map);
  INIT_FEATURE (unmap_feature, LV2_URID__unmap, &self->unmap);
  INIT_FEATURE (
    make_path_temp_feature, LV2_STATE__makePath, &self->make_path_temp);
  INIT_FEATURE (sched_feature, LV2_WORKER__schedule, &self->sched);
  INIT_FEATURE (state_sched_feature, LV2_WORKER__schedule, &self->ssched);
  INIT_FEATURE (safe_restore_feature, LV2_STATE__threadSafeRestore, NULL);
  INIT_FEATURE (log_feature, LV2_LOG__log, &self->llog);
  INIT_FEATURE (options_feature, LV2_OPTIONS__options, self->options);
  INIT_FEATURE (def_state_feature, LV2_STATE__loadDefaultState, NULL);
  INIT_FEATURE (hard_rt_capable_feature, LV2_CORE__hardRTCapable, NULL);
  INIT_FEATURE (data_access_feature, LV2_DATA_ACCESS_URI, NULL);
  INIT_FEATURE (instance_access_feature, LV2_INSTANCE_ACCESS_URI, NULL);
  INIT_FEATURE (
    bounded_block_length_feature, LV2_BUF_SIZE__boundedBlockLength, NULL);

#undef INIT_FEATURE

  self->features[0] = &self->map_feature;
  self->features[1] = &self->unmap_feature;
  self->features[2] = &self->sched_feature;
  self->features[3] = &self->log_feature;
  self->features[4] = &self->options_feature;
  self->features[5] = &self->def_state_feature;
  self->features[6] = &self->make_path_temp_feature;
  self->features[7] = &self->safe_restore_feature;
  self->features[8] = &self->hard_rt_capable_feature;
  self->features[9] = &self->data_access_feature;
  self->features[10] = &self->instance_access_feature;
  self->features[11] = &self->bounded_block_length_feature;
  self->features[12] = NULL;

  self->state_features[0] = &self->map_feature;
  self->state_features[1] = &self->unmap_feature;
  self->state_features[2] = &self->state_sched_feature;
  self->state_features[3] = &self->safe_restore_feature;
  self->state_features[4] = &self->log_feature;
  self->state_features[5] = &self->options_feature;
  self->state_features[6] = NULL;
}

/**
 * Returns whether the plugin contains a port
 * that reports latency.
 */
NONNULL static bool
reports_latency (Lv2Plugin * self)
{
  Plugin * pl = self->plugin;
  g_return_val_if_fail (IS_PLUGIN_AND_NONNULL (pl), false);

  if (pl->instantiation_failed)
    {
      /* ignore */
      return false;
    }

  for (int i = 0; i < pl->num_lilv_ports; i++)
    {
      Port * port = pl->lilv_ports[i];

      if (port->id.flags & PORT_FLAG_REPORTS_LATENCY)
        {
          return true;
        }
    }

  return false;
}

/**
 * Returns the plugin's latency in samples.
 *
 * Searches for a port marked as reportsLatency
 * and gets it value when a 0-size buffer is
 * passed.
 */
nframes_t
lv2_plugin_get_latency (Lv2Plugin * self)
{
  if (reports_latency (self))
    {
      const EngineProcessTimeInfo time_nfo = {
        .g_start_frame = (unsigned_frame_t) PLAYHEAD->frames,
        .g_start_frame_w_offset = (unsigned_frame_t) PLAYHEAD->frames,
        .local_offset = 0,
        .nframes = 0
      };
      lv2_plugin_process (self, &time_nfo);
    }

  return self->plugin->latency;
}

/**
 * Returns a newly allocated plugin descriptor for
 * the given LilvPlugin
 * if it can be hosted, otherwise NULL.
 */
PluginDescriptor *
lv2_plugin_create_descriptor_from_lilv (const LilvPlugin * lp)
{
  const LilvNode * lv2_uri = lilv_plugin_get_uri (lp);
  const char *     uri_str = lilv_node_as_string (lv2_uri);
  if (!string_is_ascii (uri_str))
    {
      g_warning (
        "Invalid plugin URI (not ascii),"
        " skipping");
      return NULL;
    }

  LilvNode * name = lilv_plugin_get_name (lp);

  /* check if we can host the Plugin */
  if (!lv2_uri)
    {
      g_warning (
        "Plugin URI not found, try lv2ls "
        "to get a list of all plugins");
      lilv_node_free (name);
      return NULL;
    }
  if (!name || !lilv_plugin_get_port_by_index (lp, 0))
    {
      g_message (
        "Ignoring invalid LV2 plugin %s",
        lilv_node_as_string (lilv_plugin_get_uri (lp)));
      lilv_node_free (name);
      return NULL;
    }

  /* check if shared lib exists */
  const LilvNode * lib_uri = lilv_plugin_get_library_uri (lp);
  char * path = lilv_file_uri_parse (lilv_node_as_string (lib_uri), NULL);
  if (access (path, F_OK) == -1)
    {
      g_warning ("%s not found, skipping %s", path, lilv_node_as_string (name));
      lilv_node_free (name);
      return NULL;
    }
  lilv_free (path);

  if (lilv_plugin_has_feature (lp, PM_GET_NODE (LV2_CORE__inPlaceBroken)))
    {
      g_warning (
        "Ignoring LV2 plugin \"%s\" "
        "since it cannot do inplace "
        "processing.",
        lilv_node_as_string (name));
      lilv_node_free (name);
      return NULL;
    }

  /* Zrythm splits the processing cycle so
   * powerof2 and fixed block length are not
   * supported */
  LilvNodes * required_features = lilv_plugin_get_required_features (lp);
  if (lilv_nodes_contains (
        required_features, PM_GET_NODE (LV2_BUF_SIZE__powerOf2BlockLength)))
    {
      g_message (
        "Ignoring LV2 Plugin %s because "
        "it requires a power of 2 block length",
        lilv_node_as_string (name));
      lilv_nodes_free (required_features);
      lilv_node_free (name);
      return NULL;
    }
  if (lilv_nodes_contains (
        required_features, PM_GET_NODE (LV2_BUF_SIZE__fixedBlockLength)))
    {
      g_message (
        "Ignoring LV2 Plugin %s because "
        "it requires fixed block length",
        lilv_node_as_string (name));
      lilv_nodes_free (required_features);
      lilv_node_free (name);
      return NULL;
    }
  lilv_nodes_free (required_features);

  /* set descriptor info */
  PluginDescriptor * pd = plugin_descriptor_new ();
  pd->protocol = Z_PLUGIN_PROTOCOL_LV2;
  pd->arch = ARCH_64;
  const char * str = lilv_node_as_string (name);
  pd->name = g_strdup (str);
  lilv_node_free (name);
  LilvNode * author = lilv_plugin_get_author_name (lp);
  str = lilv_node_as_string (author);
  pd->author = g_strdup (str);
  lilv_node_free (author);
  LilvNode * website = lilv_plugin_get_author_homepage (lp);
  str = lilv_node_as_string (website);
  pd->website = g_strdup (str);
  lilv_node_free (website);
  const LilvPluginClass * pclass = lilv_plugin_get_class (lp);
  const LilvNode *        label = lilv_plugin_class_get_label (pclass);
  str = lilv_node_as_string (label);
  if (str)
    {
      pd->category_str = string_get_substr_before_suffix (str, " Plugin");
    }
  else
    {
      pd->category_str = g_strdup ("Plugin");
    }

  pd->category = plugin_descriptor_string_to_category (pd->category_str);

  /* count atom-event-ports that feature
   * atom:supports <http://lv2plug.in/ns/ext/midi#MidiEvent>
   */
  int count_midi_out = 0;
  int count_midi_in = 0;
  for (uint32_t i = 0; i < lilv_plugin_get_num_ports (lp); ++i)
    {
      const LilvPort * port = lilv_plugin_get_port_by_index (lp, i);
      if (lilv_port_is_a (lp, port, PM_GET_NODE (LV2_ATOM__AtomPort)))
        {
          LilvNodes * buffer_types =
            lilv_port_get_value (lp, port, PM_GET_NODE (LV2_ATOM__bufferType));
          LilvNodes * atom_supports =
            lilv_port_get_value (lp, port, PM_GET_NODE (LV2_ATOM__supports));

          if (
            lilv_nodes_contains (buffer_types, PM_GET_NODE (LV2_ATOM__Sequence))
            && lilv_nodes_contains (
              atom_supports, PM_GET_NODE (LV2_MIDI__MidiEvent)))
            {
              if (lilv_port_is_a (lp, port, PM_GET_NODE (LV2_CORE__InputPort)))
                {
                  count_midi_in++;
                }
              if (lilv_port_is_a (lp, port, PM_GET_NODE (LV2_CORE__OutputPort)))
                {
                  count_midi_out++;
                }
            }
          lilv_nodes_free (buffer_types);
          lilv_nodes_free (atom_supports);
        }
    }

  pd->num_audio_ins = (int) lilv_plugin_get_num_ports_of_class (
    lp, PM_GET_NODE (LV2_CORE__InputPort), PM_GET_NODE (LV2_CORE__AudioPort),
    NULL);
  pd->num_midi_ins =
    (int) lilv_plugin_get_num_ports_of_class (
      lp, PM_GET_NODE (LV2_CORE__InputPort), PM_GET_NODE (LV2_EVENT__EventPort),
      NULL)
    + count_midi_in;
  pd->num_audio_outs = (int) lilv_plugin_get_num_ports_of_class (
    lp, PM_GET_NODE (LV2_CORE__OutputPort), PM_GET_NODE (LV2_CORE__AudioPort),
    NULL);
  pd->num_midi_outs =
    (int) lilv_plugin_get_num_ports_of_class (
      lp, PM_GET_NODE (LV2_CORE__OutputPort),
      PM_GET_NODE (LV2_EVENT__EventPort), NULL)
    + count_midi_out;
  pd->num_ctrl_ins = (int) lilv_plugin_get_num_ports_of_class (
    lp, PM_GET_NODE (LV2_CORE__InputPort), PM_GET_NODE (LV2_CORE__ControlPort),
    NULL);
  pd->num_ctrl_outs = (int) lilv_plugin_get_num_ports_of_class (
    lp, PM_GET_NODE (LV2_CORE__OutputPort), PM_GET_NODE (LV2_CORE__ControlPort),
    NULL);
  pd->num_cv_ins = (int) lilv_plugin_get_num_ports_of_class (
    lp, PM_GET_NODE (LV2_CORE__InputPort), PM_GET_NODE (LV2_CORE__CVPort), NULL);
  pd->num_cv_outs = (int) lilv_plugin_get_num_ports_of_class (
    lp, PM_GET_NODE (LV2_CORE__OutputPort), PM_GET_NODE (LV2_CORE__CVPort),
    NULL);

  pd->uri = g_strdup (uri_str);
  pd->min_bridge_mode = plugin_descriptor_get_min_bridge_mode (pd);
  pd->has_custom_ui = plugin_descriptor_has_custom_ui (pd);

  return pd;
}

/**
 * Creates an LV2 plugin from given uri.
 *
 * Note that this does not instantiate the plugin.
 * For instantiating the plugin using a preset or
 * state file, see lv2_plugin_instantiate.
 *
 * @param pl A newly created Plugin with its
 *   descriptor filled in.
 * @param uri The URI.
 */
Lv2Plugin *
lv2_plugin_new_from_uri (Plugin * pl, const char * uri, GError ** error)
{
  g_message ("Creating from uri: %s...", uri);

  LilvNode *         lv2_uri = lilv_new_uri (LILV_WORLD, uri);
  const LilvPlugin * lilv_plugin =
    lilv_plugins_get_by_uri (LILV_PLUGINS, lv2_uri);
  lilv_node_free (lv2_uri);

  if (!lilv_plugin)
    {
      g_set_error (
        error, Z_PLUGINS_LV2_PLUGIN_ERROR,
        Z_PLUGINS_LV2_PLUGIN_ERROR_CREATION_FAILED,
        _ ("Failed to get LV2 plugin from URI "
           "<%s>"),
        uri);
      return NULL;
    }
  Lv2Plugin * lv2_plugin = lv2_plugin_new (pl);

  lv2_plugin->lilv_plugin = lilv_plugin;

  g_message ("done");

  return lv2_plugin;
}

/**
 * Creates a new LV2 plugin using the given Plugin
 * instance.
 *
 * The given plugin instance must be a newly
 * allocated one.
 *
 * @param plugin A newly allocated Plugin instance.
 */
Lv2Plugin *
lv2_plugin_new (Plugin * plugin)
{
  Lv2Plugin * self = object_new (Lv2Plugin);

  self->magic = LV2_PLUGIN_MAGIC;

  /* set pointers to each other */
  self->plugin = plugin;
  plugin->lv2 = self;

  return self;
}

bool
lv2_plugin_ui_type_is_external (const LilvNode * ui_type)
{
  return lilv_node_equals (ui_type, PM_GET_NODE (LV2_KX__externalUi));
}

/**
 * Returns whether the plugin has a custom UI that
 * is deprecated (GtkUI, QtUI, etc.).
 *
 * @return If the plugin has a deprecated UI,
 *   returns the UI URI, otherwise NULL.
 */
char *
lv2_plugin_has_deprecated_ui (const char * uri)
{
  LilvNode *         lv2_uri = lilv_new_uri (LILV_WORLD, uri);
  const LilvPlugin * lilv_plugin =
    lilv_plugins_get_by_uri (LILV_PLUGINS, lv2_uri);
  lilv_node_free (lv2_uri);
  const LilvUI *   ui;
  const LilvNode * ui_type;
  LilvUIs *        uis = lilv_plugin_get_uis (lilv_plugin);
  bool             ui_picked =
    lv2_plugin_pick_ui (uis, LV2_PLUGIN_UI_WRAPPABLE, &ui, &ui_type);
  if (!ui_picked)
    {
      ui_picked =
        lv2_plugin_pick_ui (uis, LV2_PLUGIN_UI_EXTERNAL, &ui, &ui_type);
    }
  if (!ui_picked)
    {
      ui_picked =
        lv2_plugin_pick_ui (uis, LV2_PLUGIN_UI_FOR_BRIDGING, &ui, &ui_type);
    }
  if (!ui_picked)
    {
      return false;
    }

  const char * ui_type_str = lilv_node_as_string (ui_type);
  char *       ret = NULL;
  if (
    lilv_node_equals (ui_type, PM_GET_NODE (LV2_UI__GtkUI))
    || lilv_node_equals (ui_type, PM_GET_NODE (LV2_UI__Gtk3UI))
    || lilv_node_equals (ui_type, PM_GET_NODE (LV2_UI__Qt5UI))
    || lilv_node_equals (ui_type, PM_GET_NODE (LV2_UI__Qt4UI)))
    {
      ret = g_strdup (ui_type_str);
    }
  lilv_uis_free (uis);

  return ret;
}

/**
 * Returns whether the given UI uri is supported
 * for non-bridged plugins.
 */
bool
lv2_plugin_is_ui_supported (const char * pl_uri, const char * ui_uri)
{
  char * ui_class = lv2_plugin_get_ui_class (pl_uri, ui_uri);
  if (!ui_class)
    {
      g_warning ("failed to get UI class for <%s>:<%s>", pl_uri, ui_uri);
      return false;
    }

  if (
    suil_ui_supported (LV2_UI__Gtk4UI, ui_class)
    || string_is_equal (ui_class, LV2_KX__externalUi))
    {
      g_free (ui_class);
      return true;
    }

  return false;
}

/**
 * Returns the UI URIs that this plugin has.
 */
void
lv2_plugin_get_uis (const char * pl_uri, char ** uris, int * num_uris)
{
  *num_uris = 0;

  LilvNode *         lv2_uri = lilv_new_uri (LILV_WORLD, pl_uri);
  const LilvPlugin * lilv_plugin =
    lilv_plugins_get_by_uri (LILV_PLUGINS, lv2_uri);
  lilv_node_free (lv2_uri);
  LilvUIs * uis = lilv_plugin_get_uis (lilv_plugin);
  LILV_FOREACH (uis, u, uis)
    {
      const LilvUI *   cur_ui = lilv_uis_get (uis, u);
      const LilvNode * ui_uri = lilv_ui_get_uri (cur_ui);
      uris[(*num_uris)++] = g_strdup (lilv_node_as_uri (ui_uri));
    }

  lilv_uis_free (uis);
}

char *
lv2_plugin_get_ui_class (const char * pl_uri, const char * ui_uri)
{
  LilvNode *         lv2_uri = lilv_new_uri (LILV_WORLD, pl_uri);
  const LilvPlugin * lilv_plugin =
    lilv_plugins_get_by_uri (LILV_PLUGINS, lv2_uri);
  lilv_node_free (lv2_uri);
  g_return_val_if_fail (lilv_plugin, NULL);
  LilvUIs *      uis = lilv_plugin_get_uis (lilv_plugin);
  LilvNode *     ui_uri_node = lilv_new_uri (LILV_WORLD, ui_uri);
  const LilvUI * ui = lilv_uis_get_by_uri (uis, ui_uri_node);

  const LilvNodes * ui_classes = lilv_ui_get_classes (ui);
  char *            ui_type_uri = NULL;
  LILV_FOREACH (nodes, t, ui_classes)
    {
      const LilvNode * ui_type = lilv_nodes_get (ui_classes, t);
      ui_type_uri = g_strdup (lilv_node_as_uri (ui_type));
      break;
    }

  lilv_node_free (ui_uri_node);
  lilv_uis_free (uis);

  return ui_type_uri;
}

/**
 * Returns the bundle path of the UI as a URI.
 */
char *
lv2_plugin_get_ui_bundle_uri (const char * pl_uri, const char * ui_uri)
{
  LilvNode *         lv2_uri = lilv_new_uri (LILV_WORLD, pl_uri);
  const LilvPlugin * lilv_plugin =
    lilv_plugins_get_by_uri (LILV_PLUGINS, lv2_uri);
  lilv_node_free (lv2_uri);
  LilvUIs *  uis = lilv_plugin_get_uis (lilv_plugin);
  LilvNode * ui_uri_node = lilv_new_uri (LILV_WORLD, ui_uri);
  g_return_val_if_fail (ui_uri_node, NULL);
  const LilvUI * ui = lilv_uis_get_by_uri (uis, ui_uri_node);

  const LilvNode * bundle_uri_node = lilv_ui_get_bundle_uri (ui);
  char *           bundle_uri = g_strdup (lilv_node_as_uri (bundle_uri_node));

  lilv_node_free (ui_uri_node);
  lilv_uis_free (uis);

  return bundle_uri;
}

/**
 * Returns the binary path of the UI as a URI.
 */
char *
lv2_plugin_get_ui_binary_uri (const char * pl_uri, const char * ui_uri)
{
  LilvNode *         lv2_uri = lilv_new_uri (LILV_WORLD, pl_uri);
  const LilvPlugin * lilv_plugin =
    lilv_plugins_get_by_uri (LILV_PLUGINS, lv2_uri);
  lilv_node_free (lv2_uri);
  LilvUIs *  uis = lilv_plugin_get_uis (lilv_plugin);
  LilvNode * ui_uri_node = lilv_new_uri (LILV_WORLD, ui_uri);
  g_return_val_if_fail (ui_uri_node, NULL);
  const LilvUI * ui = lilv_uis_get_by_uri (uis, ui_uri_node);

  const LilvNode * binary_uri_node = lilv_ui_get_binary_uri (ui);
  char *           binary_uri = g_strdup (lilv_node_as_uri (binary_uri_node));

  lilv_node_free (ui_uri_node);
  lilv_uis_free (uis);

  return binary_uri;
}

bool
lv2_plugin_is_ui_external (const char * uri, const char * ui_uri, GError ** error)
{
  LilvNode *         lv2_uri = lilv_new_uri (LILV_WORLD, uri);
  const LilvPlugin * lilv_plugin =
    lilv_plugins_get_by_uri (LILV_PLUGINS, lv2_uri);
  lilv_node_free (lv2_uri);
  LilvUIs *      uis = lilv_plugin_get_uis (lilv_plugin);
  LilvNode *     ui_uri_node = lilv_new_uri (LILV_WORLD, ui_uri);
  const LilvUI * ui = lilv_uis_get_by_uri (uis, ui_uri_node);
  if (!ui)
    {
      g_set_error (
        error, Z_PLUGINS_LV2_PLUGIN_ERROR, Z_PLUGINS_LV2_PLUGIN_ERROR_NO_UI,
        _ ("Could not find UI by URI <%s>"), ui_uri);
      return false;
    }

  const LilvNodes * ui_classes = lilv_ui_get_classes (ui);
  LILV_FOREACH (nodes, t, ui_classes)
    {
      const LilvNode * ui_type = lilv_nodes_get (ui_classes, t);

      if (lilv_node_equals (ui_type, PM_GET_NODE (LV2_KX__externalUi)))
        {
          lilv_node_free (ui_uri_node);
          lilv_uis_free (uis);
          return true;
        }
    }

  lilv_node_free (ui_uri_node);
  lilv_uis_free (uis);

  return false;
}

/**
 * Pick the most preferable UI for non-bridged
 * plugins.
 *
 * Calls lv2_plugin_pick_ui().
 *
 * @param[out] ui (Output) UI of the specific
 *   plugin.
 * @param[out] ui_type UI type (eg, X11).
 *
 * @return Whether a UI was picked.
 */
bool
lv2_plugin_pick_most_preferable_ui (
  const char * plugin_uri,
  char **      out_ui_str,
  char **      out_ui_type_str,
  bool         allow_bridged,
  bool         print_result)
{
  LilvNode *         uri = lilv_new_uri (LILV_WORLD, plugin_uri);
  const LilvPlugin * lilv_pl = lilv_plugins_get_by_uri (LILV_PLUGINS, uri);
  lilv_node_free (uri);
  g_return_val_if_fail (lilv_pl, false);

  LilvUIs *        uis = lilv_plugin_get_uis (lilv_pl);
  const LilvUI *   out_ui;
  const LilvNode * out_ui_type;

  /* get wrappable UI */
  bool ui_picked =
    lv2_plugin_pick_ui (uis, LV2_PLUGIN_UI_WRAPPABLE, &out_ui, &out_ui_type);
  bool have_wrappable = ui_picked;

  bool have_bridgable = false;
#ifdef HAVE_CARLA
  /* try a bridged UI */
  if (!ui_picked && allow_bridged)
    {
      ui_picked = lv2_plugin_pick_ui (
        uis, LV2_PLUGIN_UI_FOR_BRIDGING, &out_ui, &out_ui_type);
      have_bridgable = ui_picked;
    }
#endif

  /* try an external UI */
  bool have_external = false;
  if (!ui_picked)
    {
      ui_picked =
        lv2_plugin_pick_ui (uis, LV2_PLUGIN_UI_EXTERNAL, &out_ui, &out_ui_type);
      have_external = ui_picked;
    }

  if (out_ui_str)
    {
      const LilvNode * out_ui_uri = lilv_ui_get_uri (out_ui);
      *out_ui_str = g_strdup (lilv_node_as_uri (out_ui_uri));
    }
  if (out_ui_type_str)
    {
      *out_ui_type_str = g_strdup (lilv_node_as_uri (out_ui_type));
    }

  if (print_result)
    {
      g_debug (
        "most preferable UI: wrappable %d, bridgeable %d, external %d",
        have_wrappable, have_bridgable, have_external);
    }

  lilv_uis_free (uis);

  return ui_picked;
}

/**
 * Pick the most preferable UI for the given flag.
 *
 * @param[out] ui (Output) UI of the specific
 *   plugin.
 * @param[out] ui_type UI type (eg, X11).
 *
 * @return Whether a UI was picked.
 */
bool
lv2_plugin_pick_ui (
  const LilvUIs *     uis,
  Lv2PluginPickUiFlag flag,
  const LilvUI **     out_ui,
  const LilvNode **   out_ui_type)
{
  LILV_FOREACH (uis, u, uis)
    {
      const LilvUI *    cur_ui = lilv_uis_get (uis, u);
      const LilvNodes * ui_types = lilv_ui_get_classes (cur_ui);
#if 0
      g_debug (
        "Checking UI: %s",
        lilv_node_as_uri (lilv_ui_get_uri (cur_ui)));
#endif
      LILV_FOREACH (nodes, t, ui_types)
        {
          const LilvNode * ui_type = lilv_nodes_get (ui_types, t);
          const char *     ui_type_uri = lilv_node_as_uri (ui_type);
          /*g_debug ("Found UI type: %s", ui_type_uri);*/

          bool acceptable = false;
          switch (flag)
            {
            case LV2_PLUGIN_UI_WRAPPABLE:
              acceptable =
                (bool) suil_ui_supported (LV2_UI__Gtk4UI, ui_type_uri);
              if (acceptable)
                {
                  if (out_ui_type)
                    *out_ui_type = ui_type;
                  /*g_debug ("Wrappable UI accepted");*/
                }
              break;
            case LV2_PLUGIN_UI_EXTERNAL:
              if (string_is_equal (ui_type_uri, LV2_KX__externalUi))
                {
                  acceptable = true;
                  if (out_ui_type)
                    {
                      *out_ui_type = PM_GET_NODE (LV2_KX__externalUi);
                    }
                  /*g_debug ("External KX UI accepted");*/
                }
              break;
            case LV2_PLUGIN_UI_FOR_BRIDGING:
              if (
#ifdef HAVE_CARLA_BRIDGE_LV2_GTK2
                lilv_node_equals (ui_type, PM_GET_NODE (LV2_UI__GtkUI)) ||
#endif
#ifdef HAVE_CARLA_BRIDGE_LV2_GTK3
                lilv_node_equals (ui_type, PM_GET_NODE (LV2_UI__Gtk3UI)) ||
#endif
#ifdef HAVE_CARLA_BRIDGE_LV2_QT5
                lilv_node_equals (ui_type, PM_GET_NODE (LV2_UI__Qt5UI)) ||
#endif
#ifdef HAVE_CARLA_BRIDGE_LV2_QT4
                lilv_node_equals (ui_type, PM_GET_NODE (LV2_UI__Qt4UI)) ||
#endif
                /*lilv_node_equals (*/
                /*ui_type,*/
                /*PM_LILV_NODES.ui_externalkx) ||*/
                false)
                {
                  acceptable = true;
                  if (out_ui_type)
                    {
                      *out_ui_type = ui_type;
                    }
#if 0
                  g_debug (
                    "UI %s accepted for "
                    "bridging",
                    lilv_node_as_string (ui_type));
#endif
                }
              break;
            }

          if (acceptable)
            {
              if (out_ui)
                {
                  *out_ui = cur_ui;
                }
              /*g_debug ("LV2: picked UI <%s>", ui_type_uri);*/
              return true;
            }
        }
    }

  return false;
}

/**
 * Returns the library path.
 *
 * Must be free'd with free().
 */
char *
lv2_plugin_get_library_path (Lv2Plugin * self)
{
  const LilvNode * library_uri = lilv_plugin_get_library_uri (self->lilv_plugin);
  char * library_path = lilv_node_get_path (library_uri, NULL);
  g_warn_if_fail (library_path);

  return library_path;
}

char *
lv2_plugin_get_abs_state_file_path (Lv2Plugin * self, bool is_backup)
{
  char * abs_state_dir =
    plugin_get_abs_state_dir (self->plugin, is_backup, true);
  char * state_file_abs_path =
    g_build_filename (abs_state_dir, "state.ttl", NULL);

  g_free (abs_state_dir);

  return state_file_abs_path;
}

/**
 * Instantiate the plugin.
 *
 * All of the actual initialization is done here.
 * If this is a new plugin, preset_uri should be
 * empty. If the project is being loaded, preset
 * uri should be the state file path.
 *
 * @param self Plugin to instantiate.
 * @param use_state_file Whether to use the plugin's
 *   state file to instantiate the plugin.
 * @param preset_uri URI of preset to load.
 * @param state State to load, if loading from
 *   a state. This is used when cloning plugins
 *   for example. The state of the original plugin
 *   is passed here.
 *
 * @return 0 if OK, non-zero if error.
 */
int
lv2_plugin_instantiate (
  Lv2Plugin * self,
  bool        use_state_file,
  char *      preset_uri,
  LilvState * state,
  GError **   error)
{
  g_message (
    "Instantiating... uri: %s, "
    "preset_uri: %s, use_state_file: %d"
    "state: %p",
    self->plugin->setting->descr->uri, preset_uri, use_state_file, state);

  Plugin * pl = self->plugin;
  g_return_val_if_fail (IS_PLUGIN_AND_NONNULL (pl), -1);

  const PluginDescriptor * descr = pl->setting->descr;

  set_features (self);

  zix_sem_init (&self->work_lock, 1);

  self->map.handle = self;
  self->map.map = lv2_urid_map_uri;

  self->worker.plugin = self;
  self->state_worker.plugin = self;

  self->unmap.handle = self;
  self->unmap.unmap = lv2_urid_unmap_uri;

  lv2_atom_forge_init (&self->main_forge, &self->map);
  lv2_atom_forge_init (&self->dsp_forge, &self->map);

  self->env = serd_env_new (NULL);
  serd_env_set_prefix_from_strings (
    self->env, (const uint8_t *) "patch", (const uint8_t *) LV2_PATCH_PREFIX);
  serd_env_set_prefix_from_strings (
    self->env, (const uint8_t *) "time", (const uint8_t *) LV2_TIME_PREFIX);
  serd_env_set_prefix_from_strings (
    self->env, (const uint8_t *) "xsd", (const uint8_t *) LILV_NS_XSD);

  self->sratom = sratom_new (&self->map);
  self->ui_sratom = sratom_new (&self->map);
  sratom_set_env (self->sratom, self->env);
  sratom_set_env (self->ui_sratom, self->env);

  GError * err = NULL;
  char *   templ = g_dir_make_tmp ("lv2_self_XXXXXX", &err);
  if (!templ)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "%s", _ ("Failed to make temporary dir"));
      return -1;
    }
  self->temp_dir = g_strjoin (NULL, templ, "/", NULL);
  g_free (templ);

  self->make_path_temp.handle = self;
  self->make_path_temp.path = lv2_state_make_path_temp;

  self->sched.handle = &self->worker;
  self->sched.schedule_work = lv2_worker_schedule;

  self->ssched.handle = &self->state_worker;
  self->ssched.schedule_work = lv2_worker_schedule;

  self->llog.handle = self;
  lv2_log_set_printf_funcs (&self->llog);

  /*zix_sem_init (&self->exit_sem, 0);*/

  zix_sem_init (&self->worker.sem, 0);

  /* Load preset, if specified */
  if (!state)
    {
      if (preset_uri)
        {
          LilvNode * preset = lilv_new_uri (LILV_WORLD, preset_uri);

          lv2_state_load_presets (self, NULL, NULL);
          state = lilv_state_new_from_world (LILV_WORLD, &self->map, preset);
          self->preset = state;
          lilv_node_free (preset);
          if (!state)
            {
              g_set_error (
                error, Z_PLUGINS_LV2_PLUGIN_ERROR,
                Z_PLUGINS_LV2_PLUGIN_ERROR_CANNOT_FIND_URI,
                _ ("Failed to find preset with URI "
                   "<%s>"),
                preset_uri);
              return -1;
            }
        } /* end if preset uri */
      else if (use_state_file)
        {
          char * state_file_path = lv2_plugin_get_abs_state_file_path (
            self, PROJECT->loading_from_backup);
          state = lilv_state_new_from_file (
            LILV_WORLD, &self->map, NULL, state_file_path);
          if (!state)
            {
              g_set_error (
                error, Z_PLUGINS_LV2_PLUGIN_ERROR,
                Z_PLUGINS_LV2_PLUGIN_ERROR_FAILED,
                _ ("Failed to load state from %s"), state_file_path);
              return -1;
            }
          g_free (state_file_path);

          LilvNode * lv2_uri =
            lilv_node_duplicate (lilv_state_get_plugin_uri (state));

          if (!lv2_uri)
            {
              g_warning (
                "Missing plugin URI, try lv2ls"
                " to list plugins");
            }

          /* Find plugin */
          const char * lv2_uri_str = lilv_node_as_string (lv2_uri);
          g_message ("Plugin URI: %s", lv2_uri_str);
          g_return_val_if_fail (LILV_PLUGINS, -1);
          self->lilv_plugin = lilv_plugins_get_by_uri (LILV_PLUGINS, lv2_uri);
          if (!self->lilv_plugin)
            {
              g_set_error (
                error, Z_PLUGINS_LV2_PLUGIN_ERROR,
                Z_PLUGINS_LV2_PLUGIN_ERROR_CANNOT_FIND_URI,
                _ ("Failed to find plugin with URI "
                   "<%s>"),
                lv2_uri_str);
              lilv_node_free (lv2_uri);
              return -1;
            }
          lilv_node_free (lv2_uri);
        } /* end if use state file */
    }
  g_warn_if_fail (self->lilv_plugin);

  int ret = 0;
#if !defined(_WOE32) && !defined(__APPLE__)
  /* check that plugin .so doesn't contain illegal
   * dynamic dependencies */
  char * library_path = lv2_plugin_get_library_path (self);
  void * handle = dlopen (library_path, RTLD_LAZY);
  if (!handle)
    {
      g_set_error (
        error, Z_PLUGINS_LV2_PLUGIN_ERROR, Z_PLUGINS_LV2_PLUGIN_ERROR_FAILED,
        _ ("Failed to dlopen %s: %s"), library_path, dlerror ());
      return -1;
    }
  struct link_map * lm;
  ret = dlinfo (handle, RTLD_DI_LINKMAP, &lm);
  if (!lm || ret != 0)
    {
      g_set_error (
        error, Z_PLUGINS_LV2_PLUGIN_ERROR, Z_PLUGINS_LV2_PLUGIN_ERROR_FAILED,
        _ ("Failed to get dlinfo for %s"), library_path);
      return -1;
    }
  if (ZRYTHM_HAVE_UI)
    {
      while (lm)
        {
          g_message (" - %s (0x%016" PRIX64 ")", lm->l_name, lm->l_addr);
          if (
            string_contains_substr (lm->l_name, "Qt5Widgets.so")
            || string_contains_substr (lm->l_name, "libgtk-3.so")
            || string_contains_substr (lm->l_name, "libgtk"))
            {
              char * basename = io_path_get_basename (lm->l_name);
              char   msg[1200];
              sprintf (
                msg,
                _ ("%s <%s> contains a reference to %s, which "
                   "may cause issues.\nIf the plugin does not "
                   "load, please try instantiating the plugin "
                   "in full-bridged mode, and report this to "
                   "the plugin distributor and/or author:\n"
                   "%s <%s>"),
                descr->name, descr->uri, basename, descr->author,
                descr->website);
              ui_show_error_message (NULL, msg);
              g_free (basename);
            }

          lm = lm->l_next;
        }
    }
  dlclose (handle);
#endif

  self->control_in = -1;
  self->enabled_in = -1;

  /* Set default values for all ports */
  ret = lv2_create_or_init_ports_and_parameters (self);
  if (ret != 0)
    {
      g_set_error (
        error, Z_PLUGINS_LV2_PLUGIN_ERROR, Z_PLUGINS_LV2_PLUGIN_ERROR_FAILED,
        "%s",
        _ ("Failed to create or init ports and "
           "parameters"));
      return -1;
    }

  /* --- Check for required features --- */

  /* check that any required features are
   * supported */
  g_message ("checking required features");
  LilvNodes * req_feats = lilv_plugin_get_required_features (self->lilv_plugin);
  LILV_FOREACH (nodes, f, req_feats)
    {
      const char * uri = lilv_node_as_uri (lilv_nodes_get (req_feats, f));
      if (feature_is_supported (self, uri))
        {
          g_message ("Required feature %s is supported", uri);
        }
      else
        {
          g_set_error (
            error, Z_PLUGINS_LV2_PLUGIN_ERROR, Z_PLUGINS_LV2_PLUGIN_ERROR_FAILED,
            _ ("Required feature %s is not supported"), uri);
          lilv_nodes_free (req_feats);
          return -1;
        }
    }
  lilv_nodes_free (req_feats);

  g_message ("checking optional features");
  LilvNodes * optional_features =
    lilv_plugin_get_optional_features (self->lilv_plugin);
  LILV_FOREACH (nodes, f, optional_features)
    {
      const char * uri =
        lilv_node_as_uri (lilv_nodes_get (optional_features, f));
      if (feature_is_supported (self, uri))
        {
          g_message ("Optional feature %s is supported", uri);
        }
      else
        {
          g_message ("Optional feature %s is not supported", uri);
        }
    }
  lilv_nodes_free (optional_features);

  g_message ("checking extension data");
  LilvNodes * ext_data = lilv_plugin_get_extension_data (self->lilv_plugin);
  LILV_FOREACH (nodes, f, ext_data)
    {
      const char * uri = lilv_node_as_uri (lilv_nodes_get (ext_data, f));
      g_message ("Plugin has extension data: %s", uri);
    }
  lilv_nodes_free (ext_data);

  /* Check for thread-safe state restore()
   * method. */
  if (lilv_plugin_has_feature (
        self->lilv_plugin, PM_GET_NODE (LV2_STATE__threadSafeRestore)))
    {
      g_message ("plugin has thread safe restore feature");
      self->safe_restore = true;
    }

  /* Check for default state feature */
  if (lilv_plugin_has_feature (
        self->lilv_plugin, PM_GET_NODE (LV2_STATE__loadDefaultState)))
    {
      g_message ("plugin has default state feature");
      self->has_default_state = true;
    }

  if (!state)
    {
      /* Not restoring state, load the plugin as a
       * preset to get default - see
       * http://lv2plug.in/ns/ext/state/#loadDefaultState */
      state = lilv_state_new_from_world (
        LILV_WORLD, &self->map, lilv_plugin_get_uri (self->lilv_plugin));
    }

  const char * pl_uri = self->plugin->setting->descr->uri;
  const char * ui_uri = self->plugin->setting->ui_uri;
  if (ui_uri)
    {
      /* set whether the UI is external */
      bool is_ui_external = lv2_plugin_is_ui_external (pl_uri, ui_uri, &err);
      if (err)
        {
          PROPAGATE_PREFIXED_ERROR (
            error, err, "%s",
            _ ("Failed checking whether lv2 plugin "
               "UI is external"));
          return -1;
        }
      else if (is_ui_external)
        {
          self->has_external_ui = true;
        }

      char * class = lv2_plugin_get_ui_class (pl_uri, ui_uri);
      g_message ("Selected UI: %s (type: %s)", ui_uri, class);
      g_free (class);
    }
  else
    {
      g_message ("Selected UI: Generic");
    }

  /* The UI ring is fed by self->output
   * ports (usually one), and the UI
   * updates roughly once per cycle.
   * The ring size is a few times the
   * size of the MIDI output to give the UI
   * a chance to keep up.  The UI
   * should be able to keep up with 4 cycles,
   * and tests show this works
   * for me, but this value might need
   * increasing to avoid overflows.
   */
  self->comm_buffer_size =
    (uint32_t) (AUDIO_ENGINE->midi_buf_size * N_BUFFER_CYCLES);

  /* buffer data communication from plugin UI to plugin instance.
   * this buffer needs to potentially hold
   *   (port's minimumSize) * (audio-periods) / (UI-periods)
   * bytes.
   *
   *  e.g 48kSPS / 128fpp -> audio-periods = 375 Hz
   *  ui-periods = 25 Hz (SuperRapidScreenUpdate)
   *  default minimumSize = 32K (see LV2Plugin::allocate_atom_event_buffers()
   *
   * it is NOT safe to overflow (msg.size will be misinterpreted)
   */
  uint32_t bufsiz = 32768;
  int      fact = (int) ceilf (AUDIO_ENGINE->sample_rate / 3000.f);
  self->comm_buffer_size =
    MAX ((uint32_t) bufsiz * MAX (8, (uint32_t) fact), self->comm_buffer_size);

  g_message ("==========================");
  g_message ("Comm buffers: %d bytes", self->comm_buffer_size);
  g_message ("Update rate:  %.01f Hz", (double) self->plugin->ui_update_hz);
  g_message ("Scale factor: %.01f", (double) self->plugin->ui_scale_factor);
  g_message ("==========================");

  static float        samplerate = 0.f;
  static int          nominal_blocklength = 0;
  static int          min_blocklength = 0;
  static int          max_blocklength = 4096;
  static int          midi_buf_size = 0;
  static const char * prog_name = PROGRAM_NAME;

  samplerate = (float) AUDIO_ENGINE->sample_rate;
  nominal_blocklength = (int) AUDIO_ENGINE->block_length;
  midi_buf_size = (int) AUDIO_ENGINE->midi_buf_size;

  /* Build options array to pass to plugin */
  const LV2_Options_Option options[] = {
    {LV2_OPTIONS_INSTANCE,  0, PM_URIDS.param_sampleRate,         sizeof (float),
     PM_URIDS.atom_Float,                                                                                      &samplerate                   },
    { LV2_OPTIONS_INSTANCE, 0, PM_URIDS.bufsz_minBlockLength,     sizeof (int32_t),
     PM_URIDS.atom_Int,                                                                                        &min_blocklength              },
    { LV2_OPTIONS_INSTANCE, 0, PM_URIDS.bufsz_maxBlockLength,     sizeof (int32_t),
     PM_URIDS.atom_Int,                                                                                        &max_blocklength              },
    { LV2_OPTIONS_INSTANCE, 0, PM_URIDS.bufsz_nominalBlockLength,
     sizeof (int32_t),                                                                   PM_URIDS.atom_Int,    &nominal_blocklength          },
    { LV2_OPTIONS_INSTANCE, 0, PM_URIDS.bufsz_sequenceSize,       sizeof (int32_t),
     PM_URIDS.atom_Int,                                                                                        &midi_buf_size                },
    { LV2_OPTIONS_INSTANCE, 0, PM_URIDS.ui_updateRate,            sizeof (float),
     PM_URIDS.atom_Float,                                                                                      &self->plugin->ui_update_hz   },
#ifdef HAVE_LV2_1_18
    { LV2_OPTIONS_INSTANCE, 0, PM_URIDS.ui_scaleFactor,           sizeof (float),
     PM_URIDS.atom_Float,                                                                                      &self->plugin->ui_scale_factor},
#endif
    { LV2_OPTIONS_INSTANCE, 0, PM_URIDS.z_hostInfo_name,          sizeof (const char *),
     PM_URIDS.atom_String,                                                                                     &prog_name                    },
    { LV2_OPTIONS_INSTANCE, 0, PM_URIDS.z_hostInfo_version,
     sizeof (const char *),                                                              PM_URIDS.atom_String, &ZRYTHM->version              },
    { LV2_OPTIONS_INSTANCE, 0, 0,                                 0,                     0,                    NULL                          },
  };
  g_return_val_if_fail (
    (sizeof (options) / sizeof (options[0]))
      <= (sizeof (self->options) / sizeof (self->options[0])),
    -1);
  memcpy (self->options, options, sizeof (options));

  /* --- Check for required options --- */

  /* check for required options */
  g_message ("checking required options");
  LilvNodes * required_options = lilv_world_find_nodes (
    LILV_WORLD, lilv_plugin_get_uri (self->lilv_plugin),
    PM_GET_NODE (LV2_OPTIONS__requiredOption), NULL);
  if (required_options)
    {
      LILV_FOREACH (nodes, i, required_options)
        {
          const char * uri =
            lilv_node_as_uri (lilv_nodes_get (required_options, i));

          if (option_is_supported (self, uri))
            {
              g_message ("Required option %s is supported", uri);
            }
          else
            {
              g_set_error (
                error, Z_PLUGINS_LV2_PLUGIN_ERROR,
                Z_PLUGINS_LV2_PLUGIN_ERROR_FAILED,
                _ ("Required option %s is not supported"), uri);
              lilv_nodes_free (required_options);
              return -1;
            }
        }
    }
  lilv_nodes_free (required_options);

  /* check for optional options */
  g_message ("checking supported options");
  LilvNodes * supported_options = lilv_world_find_nodes (
    LILV_WORLD, lilv_plugin_get_uri (self->lilv_plugin),
    PM_GET_NODE (LV2_OPTIONS__supportedOption), NULL);
  if (supported_options)
    {
      LILV_FOREACH (nodes, i, supported_options)
        {
          const char * uri =
            lilv_node_as_uri (lilv_nodes_get (supported_options, i));

          if (option_is_supported (self, uri))
            {
              g_message ("Optional option %s is supported", uri);
            }
          else
            {
              g_message ("Optional option %s is not supported", uri);
            }
        }
    }
  lilv_nodes_free (supported_options);

  /* Create Plugin <=> UI communication
   * buffers */
  self->ui_to_plugin_events =
    zix_ring_new (zix_default_allocator (), self->comm_buffer_size);
  self->plugin_to_ui_events =
    zix_ring_new (zix_default_allocator (), self->comm_buffer_size);
  zix_ring_mlock (self->ui_to_plugin_events);
  zix_ring_mlock (self->plugin_to_ui_events);

  /* Instantiate the plugin */
  self->instance = lilv_plugin_instantiate (
    self->lilv_plugin, AUDIO_ENGINE->sample_rate, self->features);
  if (!self->instance)
    {
      g_set_error_literal (
        error, Z_PLUGINS_LV2_PLUGIN_ERROR,
        Z_PLUGINS_LV2_PLUGIN_ERROR_INSTANTIATION_FAILED,
        _ ("lilv_plugin_instantiate() failed"));
      return -1;
    }
  g_message ("Lilv plugin instantiated");

  /* --- Handle extension data --- */

  /* prepare the LV2_Extension_Data_Feature to pass
   * to the plugin UI */
  const LV2_Descriptor * lv2_descriptor =
    lilv_instance_get_descriptor (self->instance);
  self->ext_data_feature.data_access = lv2_descriptor->extension_data;

  lv2_plugin_allocate_port_buffers (self);

  /* Create workers if necessary */
  if (lilv_plugin_has_extension_data (
        self->lilv_plugin, PM_GET_NODE (LV2_WORKER__interface)))
    {
      g_message ("Instantiating workers");

      const LV2_Worker_Interface * iface = (const LV2_Worker_Interface *)
        lilv_instance_get_extension_data (self->instance, LV2_WORKER__interface);

      lv2_worker_init (self, &self->worker, iface, true);
      if (self->safe_restore)
        {
          lv2_worker_init (self, &self->state_worker, iface, false);
        }
    }

  /* create options interface */
  if (lilv_plugin_has_extension_data (
        self->lilv_plugin, PM_GET_NODE (LV2_OPTIONS__interface)))
    {
      self->options_iface = (const LV2_Options_Interface *)
        lilv_instance_get_extension_data (self->instance, LV2_OPTIONS__interface);
    }

  /* --- Apply state --- */

  /* apply loaded state to plugin instance if
   * necessary */
  if (state)
    {
      g_message ("applying state");
      lv2_state_apply_state (self, state);
    }
  else
    {
      g_message ("no state to apply");
    }

  /* --- Connect ports --- */

  /* connect ports to buffers */
  g_message ("connecting %d LV2 (lilv) ports to buffers", pl->num_lilv_ports);
  for (int i = 0; i < pl->num_lilv_ports; ++i)
    {
      connect_port (self, (uint32_t) i);
    }

  /* Print initial control values */
  if (DEBUGGING)
    {
      for (int i = 0; i < pl->num_lilv_ports; i++)
        {
          Port * port = pl->lilv_ports[i];
          g_return_val_if_fail (IS_PORT_AND_NONNULL (port), -1);
          g_message ("%s = %f", port->id.sym, (double) port->control);
        }
    }

  g_message ("%s: done", __func__);

  return 0;
}

int
lv2_plugin_activate (Lv2Plugin * self, bool activate)
{
  /* Activate plugin */
  g_message ("%s lilv instance...", activate ? "Activating" : "Deactivating");
  if (activate && !self->plugin->activated)
    {
      if (!self->plugin->instantiated)
        {
          g_critical ("plugin not instantiated");
          return -1;
        }
      lilv_instance_activate (self->instance);
    }
  else if (!activate && self->plugin->activated)
    {
      lilv_instance_deactivate (self->instance);
    }

  self->plugin->activated = activate;

  return 0;
}

/**
 * Returns whether the plugin can be cleaned up
 * (some plugins crash on cleanup).
 */
bool
lv2_plugin_can_cleanup (const char * uri)
{
  /* helm hangs/deadlocks on free sometimes */
  if (
    string_is_equal (uri, "http://tytel.org/helm") ||
    /* swh-plugins crashes on free
     * https://github.com/swh/lv2/issues/13 */
    g_str_has_prefix (uri, "http://plugin.org.uk/swh-plugins"))
    return false;

  return true;
}

int
lv2_plugin_cleanup (Lv2Plugin * self)
{
  g_message (
    "Cleaning up LV2 plugin %s (%p)...", self->plugin->setting->descr->name,
    self);

  if (self->plugin->activated)
    {
      lv2_plugin_activate (self, false);
    }

  if (self->plugin->instantiated)
    {
      if (lv2_plugin_can_cleanup (self->plugin->setting->descr->uri))
        {
          object_free_w_func_and_null (lilv_instance_free, self->instance);
        }
      self->plugin->instantiated = false;
    }

  g_message ("done");

  return 0;
}

/**
 * Processes the plugin for this cycle.
 */
void
lv2_plugin_process (
  Lv2Plugin *                         self,
  const EngineProcessTimeInfo * const time_nfo)
{
  Plugin * pl = self->plugin;
  g_return_if_fail (IS_PLUGIN_AND_NONNULL (pl));

  if (pl->instantiation_failed)
    {
      /* ignore */
      return;
    }

  g_return_if_fail (pl->instantiated && pl->activated);

  /* If transport state is not as expected, then
   * something has changed */
  const bool xport_changed =
    self->rolling != (TRANSPORT_IS_ROLLING)
    || self->gframes != time_nfo->g_start_frame_w_offset
    || !math_floats_equal (
      self->bpm, tempo_track_get_current_bpm (P_TEMPO_TRACK));
#if 0
  if (xport_changed)
    {
      g_message (
        "xport changed lv2_plugin_rolling %d, "
        "self gframes vs g start frames %ld %ld, "
        "bpm %f %f",
        self->rolling,
        self->gframes, g_start_frame,
        (double) self->bpm,
        (double)
          tempo_track_get_current_bpm (
            P_TEMPO_TRACK));
    }
#endif

  /* let the plugin know if transport state
   * changed */
  uint8_t    pos_buf[256];
  LV2_Atom * lv2_pos = (LV2_Atom *) pos_buf;
  if (xport_changed && self->want_position)
    {
      /* Build an LV2 position object to report change to plugin */
      Position start_pos;
      position_from_frames (
        &start_pos, (signed_frame_t) time_nfo->g_start_frame_w_offset);
      LV2_Atom_Forge * forge = &self->dsp_forge;
      lv2_atom_forge_set_buffer (forge, pos_buf, sizeof (pos_buf));
      LV2_Atom_Forge_Frame frame;
      lv2_atom_forge_object (forge, &frame, 0, PM_URIDS.time_Position);
      lv2_atom_forge_key (forge, PM_URIDS.time_frame);
      lv2_atom_forge_long (forge, (long) time_nfo->g_start_frame_w_offset);
      lv2_atom_forge_key (forge, PM_URIDS.time_speed);
      lv2_atom_forge_float (
        forge, TRANSPORT->play_state == PLAYSTATE_ROLLING ? 1.0 : 0.0);
      lv2_atom_forge_key (forge, PM_URIDS.time_barBeat);
      int    bars = position_get_bars (&start_pos, true);
      int    beats = position_get_beats (&start_pos, true);
      double ticks = position_get_ticks (&start_pos);
      lv2_atom_forge_float (
        forge,
        ((float) beats - 1)
          + ((float) ticks / (float) TRANSPORT->ticks_per_beat));
      lv2_atom_forge_key (forge, PM_URIDS.time_bar);
      lv2_atom_forge_long (forge, bars - 1);
      lv2_atom_forge_key (forge, PM_URIDS.time_beatUnit);
      int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
      int beat_unit = tempo_track_get_beat_unit (P_TEMPO_TRACK);
      lv2_atom_forge_int (forge, beat_unit);
      lv2_atom_forge_key (forge, PM_URIDS.time_beatsPerBar);
      lv2_atom_forge_float (forge, (float) beats_per_bar);
      lv2_atom_forge_key (forge, PM_URIDS.time_beatsPerMinute);
      lv2_atom_forge_float (forge, tempo_track_get_current_bpm (P_TEMPO_TRACK));
    }

  /* Update transport state to expected values for next cycle */
  if (TRANSPORT_IS_ROLLING)
    {
      Position gpos;
      position_from_frames (
        &gpos,
        (signed_frame_t) (time_nfo->g_start_frame_w_offset + time_nfo->nframes));
      self->gframes = (unsigned_frame_t) gpos.frames;
      self->rolling = 1;
    }
  else
    {
      self->gframes = time_nfo->g_start_frame_w_offset;
      self->rolling = 0;
    }
  self->bpm = tempo_track_get_current_bpm (P_TEMPO_TRACK);

  /* Prepare port buffers */
  for (int p = 0; p < pl->num_lilv_ports; ++p)
    {
      Port * port = pl->lilv_ports[p];
      g_return_if_fail (IS_PORT_AND_NONNULL (port));

      PortIdentifier * id = &port->id;
      if (id->type == TYPE_AUDIO || id->type == TYPE_CV)
        {
          /* connect buffer */
          lilv_instance_connect_port (
            self->instance, (uint32_t) p, &port->buf[time_nfo->local_offset]);
        }
      else if (id->type == TYPE_EVENT && id->flow == FLOW_INPUT)
        {
          if (G_UNLIKELY (port->evbuf == NULL))
            {
              g_critical ("evbuf is NULL for %s", pl->setting->descr->uri);
              return;
            }
          lv2_evbuf_reset (port->evbuf, true);

          /* Write transport change event if
           * applicable */
          LV2_Evbuf_Iterator iter = lv2_evbuf_begin (port->evbuf);
          if (xport_changed && id->flags & PORT_FLAG_WANT_POSITION)
            {
              lv2_evbuf_write (
                &iter, 0, 0, lv2_pos->type, lv2_pos->size,
                (const uint8_t *) LV2_ATOM_BODY (lv2_pos));
            }

          if (self->request_update)
            {
              /* Plugin state has changed, request
               * an update */
              const LV2_Atom_Object get = {
                {sizeof (LV2_Atom_Object_Body), PM_URIDS.atom_Object},
                { 0,                            PM_URIDS.patch_Get  }
              };
              lv2_evbuf_write (
                &iter, 0, 0, get.atom.type, get.atom.size,
                (const uint8_t *) LV2_ATOM_BODY (&get));
            }

          if (port->midi_events->num_events > 0)
            {
              int num_events_written = 0;

              /* Write MIDI input */
              for (int i = 0; i < port->midi_events->num_events; i++)
                {
                  MidiEvent * ev = &port->midi_events->events[i];
                  if (
                    ev->time < time_nfo->local_offset
                    || ev->time >= time_nfo->local_offset + time_nfo->nframes)
                    {
                      /* skip events scheduled
                       * for another split within
                       * the processing cycle */
                      continue;
                    }

                  if (ZRYTHM_TESTING)
                    {
                      g_message (
                        "writing plugin input event %d at time %u - local frames %u nframes %u",
                        num_events_written, ev->time - time_nfo->local_offset,
                        time_nfo->local_offset, time_nfo->nframes);
                      midi_event_print (ev);
                    }

                  lv2_evbuf_write (
                    &iter,
                    /* event time is relative to
                     * the current zrythm full
                     * cycle (not split). it
                     * needs to be made relative
                     * to the current split */
                    ev->time - time_nfo->local_offset, 0,
                    PM_URIDS.midi_MidiEvent, 3, ev->raw_buffer);

                  num_events_written++;
                }
            }
        }
      else if (id->type == TYPE_CONTROL && id->flow == FLOW_INPUT)
        {
          /* let the plugin know if freewheeling */
          if (id->flags & PORT_FLAG_FREEWHEEL)
            {
              if (AUDIO_ENGINE->exporting)
                {
                  port->control = port->maxf;
                }
              else
                {
                  port->control = port->minf;
                }
            }
        }
    }
  self->request_update = false;

  /* Run plugin for this cycle */
  const bool send_ui_updates =
    run (self, time_nfo->nframes) && !AUDIO_ENGINE->exporting
    && self->plugin->ui_instantiated;

  /* Deliver MIDI output and UI events */
  for (int p = 0; p < pl->num_lilv_ports; ++p)
    {
      Port *           port = pl->lilv_ports[p];
      PortIdentifier * pi = &port->id;
      switch (pi->type)
        {
        case TYPE_CONTROL:
          if (G_UNLIKELY (pi->flow == FLOW_OUTPUT))
            {
              /* if latency changed, recalc graph */
              if (
                G_UNLIKELY (
                  pi->flags & PORT_FLAG_REPORTS_LATENCY
                  && self->plugin->latency != (nframes_t) port->control))
                {
                  g_message (
                    "%s: latency changed from %d "
                    "to %f",
                    pi->label, pl->latency, (double) port->control);
                  EVENTS_PUSH (ET_PLUGIN_LATENCY_CHANGED, NULL);
                  pl->latency = (nframes_t) port->control;
                }

              /* if UI is instantiated */
              if (
                G_UNLIKELY (
                  send_ui_updates && pl->visible && !port->received_ui_event
                  && !math_floats_equal (port->control, port->last_sent_control)))
                {
                  /* forward event to UI */
                  lv2_ui_send_control_val_event_from_plugin_to_ui (self, port);
                }
            }
          if (send_ui_updates)
            {
              /* ignore ports that received a UI
               * event at the start of a cycle
               * (otherwise these causes trembling
               * while changing them) */
              if (port->received_ui_event)
                {
                  port->received_ui_event = 0;
                  continue;
                }
            }
          break;
        case TYPE_EVENT:
          if (pi->flow == FLOW_OUTPUT)
            {
              for (
                LV2_Evbuf_Iterator iter = lv2_evbuf_begin (port->evbuf);
                lv2_evbuf_is_valid (iter); iter = lv2_evbuf_next (iter))
                {
                  // Get event from LV2 buffer
                  uint32_t  frames, subframes, type, size;
                  uint8_t * body;
                  lv2_evbuf_get (iter, &frames, &subframes, &type, &size, &body);

                  /* if midi event */
                  if (body && type == PM_URIDS.midi_MidiEvent)
                    {
                      if (size != 3)
                        {
                          g_message (
                            "unhandled event from "
                            "port %s of size %" PRIu32,
                            pi->label, size);
                        }
                      else
                        {
                          /* Write MIDI event to port */
                          midi_events_add_event_from_buf (
                            port->midi_events, frames, body, (int) size, 0);
                        }
                    }

                  /* if UI is instantiated */
                  if (pl->visible && !port->old_api)
                    {
                      /* forward event to UI */
                      lv2_ui_send_event_from_plugin_to_ui (
                        self, (uint32_t) p, type, size, body);
                    }
                }

              /* Clear event output for plugin to
               * write to next cycle */
              lv2_evbuf_reset (port->evbuf, false);
            }
        default:
          break;
        }
    }
}

/**
 * Populates the banks in the plugin instance.
 */
void
lv2_plugin_populate_banks (Lv2Plugin * self)
{
  /* add default bank and preset */
  PluginBank * pl_def_bank = plugin_add_bank_if_not_exists (
    self->plugin, LV2_ZRYTHM__defaultBank, _ ("Default bank"));
  PluginPreset * pl_def_preset = plugin_preset_new ();
  pl_def_preset->uri = g_strdup (LV2_ZRYTHM__initPreset);
  pl_def_preset->name = g_strdup (_ ("Init"));
  plugin_add_preset_to_bank (self->plugin, pl_def_bank, pl_def_preset);

  /*GString * presets_str = g_string_new (NULL);*/

  LilvNodes * presets = lilv_plugin_get_related (
    self->lilv_plugin, PM_GET_NODE (LV2_PRESETS__Preset));
  const LilvNode * preset_bank = PM_GET_NODE (LV2_PRESETS__bank);
  const LilvNode * rdfs_label = PM_GET_NODE (LILV_NS_RDFS "label");
  int              count = 0;
  LILV_FOREACH (nodes, i, presets)
    {
      const LilvNode * preset = lilv_nodes_get (presets, i);
      lilv_world_load_resource (LILV_WORLD, preset);

      LilvNodes * banks =
        lilv_world_find_nodes (LILV_WORLD, preset, preset_bank, NULL);
      PluginBank * pl_bank = NULL;
      if (banks)
        {
          const LilvNode * bank = lilv_nodes_get_first (banks);
          const LilvNode * bank_label =
            lilv_world_get (LILV_WORLD, bank, rdfs_label, NULL);
          pl_bank = plugin_add_bank_if_not_exists (
            self->plugin, lilv_node_as_string (bank),
            bank_label ? lilv_node_as_string (bank_label) : _ ("Unnamed bank"));
          lilv_nodes_free (banks);
        }
      else
        {
          pl_bank = pl_def_bank;
        }

      LilvNodes * labels =
        lilv_world_find_nodes (LILV_WORLD, preset, rdfs_label, NULL);
      if (labels)
        {
          const LilvNode * label = lilv_nodes_get_first (labels);
          PluginPreset *   pl_preset = plugin_preset_new ();
          pl_preset->uri = g_strdup (lilv_node_as_string (preset));
          pl_preset->name = g_strdup (lilv_node_as_string (label));
          plugin_add_preset_to_bank (self->plugin, pl_bank, pl_preset);
          lilv_nodes_free (labels);

#if 0
          g_string_append_printf (
            presets_str,
            "found preset %s (<%s>)\n",
            pl_preset->name, pl_preset->uri);
#endif
          count++;
        }
      else
        {
          g_message (
            "Skipping preset <%s> because it has "
            "no rdfs:label",
            lilv_node_as_string (preset));
        }
      /* some plugins (Helm LV2) get stuck in an
       * infinite loop when calling this */
      /*lilv_world_unload_resource (*/
      /*LILV_WORLD, preset);*/
    }
  lilv_nodes_free (presets);

  g_message ("found %d presets", count);

#if 0
  char * str = g_string_free (presets_str, false);
  g_message ("%s", str);
  g_free (str);
#endif
}

/**
 * Frees the Lv2Plugin and all its components.
 */
void
lv2_plugin_free (Lv2Plugin * self)
{
  /* Wait for finish signal from UI or signal
   * handler */
  /*zix_sem_wait (&self->exit_sem);*/
  self->exit = true;

  /* Terminate the worker */
  lv2_worker_finish (&self->worker);

  /* Deactivate suil instance */
  object_free_w_func_and_null (suil_instance_free, self->suil_instance);

  if (self->instance && self->plugin->activated)
    {
      lilv_instance_deactivate (self->instance);
      self->plugin->activated = false;
    }

  if (lv2_plugin_can_cleanup (self->plugin->setting->descr->uri))
    {
      g_debug (
        "attempting to free lilv instance for %s",
        self->plugin->setting->descr->uri);
      object_free_w_func_and_null (lilv_instance_free, self->instance);
    }

  /* Clean up */
  object_free_w_func_and_null (zix_ring_free, self->ui_to_plugin_events);
  object_free_w_func_and_null (zix_ring_free, self->plugin_to_ui_events);
  object_free_w_func_and_null (suil_host_free, self->suil_host);
  object_free_w_func_and_null (sratom_free, self->sratom);
  object_free_w_func_and_null (sratom_free, self->ui_sratom);

  /*zix_sem_destroy (&self->exit_sem);*/

  remove (self->temp_dir);
  object_zero_and_free_if_nonnull (self->temp_dir);

  object_free_w_func_and_null (free, self->ui_event_buf);

  if (self->extui.plugin_human_id)
    {
      g_free ((char *) self->extui.plugin_human_id);
      self->extui.plugin_human_id = NULL;
    }

  object_zero_and_free (self);
}
