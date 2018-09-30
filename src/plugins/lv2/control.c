/*
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
*/

#include "zrythm_app.h"
#include "audio/engine.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin_manager.h"
#include "plugins/lv2/control.h"

#include <lilv/lilv.h>

#include <gtk/gtk.h>

int
scale_point_cmp(const Lv2ScalePoint* a, const Lv2ScalePoint* b)
{
  if (a->value < b->value) {
          return -1;
  } else if (a->value == b->value) {
          return 0;
  }
  return 1;
}

Lv2ControlID*
lv2_new_port_control(LV2_Plugin* plugin, uint32_t index)
{
  LV2_Port*      port  = &plugin->ports[index];
  const LilvPort*   lport = port->lilv_port;
  const LilvPlugin* plug  = plugin->lilv_plugin;
  const LV2_Nodes*  nodes = &plugin->nodes;

  Lv2ControlID* id = (Lv2ControlID*)calloc(1, sizeof(Lv2ControlID));
  id->plugin           = plugin;
  id->type           = PORT;
  id->node           = lilv_node_duplicate(lilv_port_get_node(plug, lport));
  id->symbol         = lilv_node_duplicate(lilv_port_get_symbol(plug, lport));
  id->label          = lilv_port_get_name(plug, lport);
  id->index          = index;
  id->group          = lilv_port_get(plug, lport, plugin->nodes.pg_group);
  id->value_type     = plugin->forge.Float;
  id->is_writable    = lilv_port_is_a(plug, lport, nodes->lv2_InputPort);
  id->is_readable    = lilv_port_is_a(plug, lport, nodes->lv2_OutputPort);
  id->is_toggle      = lilv_port_has_property(plug, lport, nodes->lv2_toggled);
  id->is_integer     = lilv_port_has_property(plug, lport, nodes->lv2_integer);
  id->is_enumeration = lilv_port_has_property(plug, lport, nodes->lv2_enumeration);
  id->is_logarithmic = lilv_port_has_property(plug, lport, nodes->pprops_logarithmic);

  lilv_port_get_range(plug, lport, &id->def, &id->min, &id->max);
  if (lilv_port_has_property(plug, lport, plugin->nodes.lv2_sampleRate)) {
          /* Adjust range for lv2:sampleRate controls */
          if (lilv_node_is_float(id->min) || lilv_node_is_int(id->min)) {
                  const float min = lilv_node_as_float(id->min) * AUDIO_ENGINE->sample_rate;
                  lilv_node_free(id->min);
                  id->min = lilv_new_float(LILV_WORLD, min);
          }
          if (lilv_node_is_float(id->max) || lilv_node_is_int(id->max)) {
                  const float max = lilv_node_as_float(id->max) * AUDIO_ENGINE->sample_rate;
                  lilv_node_free(id->max);
                  id->max = lilv_new_float(LILV_WORLD, max);
          }
  }

  /* Get scale points */
  LilvScalePoints* sp = lilv_port_get_scale_points(plug, lport);
  if (sp) {
          id->points = (Lv2ScalePoint*)malloc(
                  lilv_scale_points_size(sp) * sizeof(Lv2ScalePoint));
          size_t np = 0;
          LILV_FOREACH(scale_points, s, sp) {
                  const LilvScalePoint* p = lilv_scale_points_get(sp, s);
                  if (lilv_node_is_float(lilv_scale_point_get_value(p)) ||
                      lilv_node_is_int(lilv_scale_point_get_value(p))) {
                          id->points[np].value = lilv_node_as_float(
                                  lilv_scale_point_get_value(p));
                          id->points[np].label = strdup(
                                  lilv_node_as_string(lilv_scale_point_get_label(p)));
                          ++np;
                  }
                  /* TODO: Non-float scale points? */
          }

          qsort(id->points, np, sizeof(Lv2ScalePoint),
                (int (*)(const void*, const void*))scale_point_cmp);
          id->n_points = np;

          lilv_scale_points_free(sp);
  }

  return id;
}

static bool
has_range(LV2_Plugin* plugin, const LilvNode* subject, const char* range_uri)
{
  LilvNode*  range  = lilv_new_uri(LILV_WORLD, range_uri);
  const bool result = lilv_world_ask(
          LILV_WORLD, subject, plugin->nodes.rdfs_range, range);
  lilv_node_free(range);
  return result;
}

Lv2ControlID*
lv2_new_property_control(LV2_Plugin* plugin, const LilvNode* property)
{
	Lv2ControlID* id = (Lv2ControlID*)calloc(1, sizeof(Lv2ControlID));
	id->plugin     = plugin;
	id->type     = PROPERTY;
	id->node     = lilv_node_duplicate(property);
	id->symbol   = lilv_world_get_symbol(LILV_WORLD, property);
	id->label    = lilv_world_get(LILV_WORLD, property, plugin->nodes.rdfs_label, NULL);
	id->property = plugin->map.map(plugin, lilv_node_as_uri(property));

	id->min = lilv_world_get(LILV_WORLD, property, plugin->nodes.lv2_minimum, NULL);
	id->max = lilv_world_get(LILV_WORLD, property, plugin->nodes.lv2_maximum, NULL);
	id->def = lilv_world_get(LILV_WORLD, property, plugin->nodes.lv2_default, NULL);

	const char* const types[] = {
		LV2_ATOM__Int, LV2_ATOM__Long, LV2_ATOM__Float, LV2_ATOM__Double,
		LV2_ATOM__Bool, LV2_ATOM__String, LV2_ATOM__Path, NULL
	};

	for (const char*const* t = types; *t; ++t) {
		if (has_range(plugin, property, *t)) {
			id->value_type = plugin->map.map(plugin, *t);
			break;
		}
	}

	id->is_toggle  = (id->value_type == plugin->forge.Bool);
	id->is_integer = (id->value_type == plugin->forge.Int ||
	                  id->value_type == plugin->forge.Long);

	const size_t sym_len = strlen(lilv_node_as_string(id->symbol));
	if (sym_len > plugin->longest_sym) {
		plugin->longest_sym = sym_len;
	}

	if (!id->value_type) {
		fprintf(stderr, "Unknown value type for property <%s>\n",
		        lilv_node_as_string(property));
	}

	return id;
}

void
lv2_add_control(Lv2Controls* controls, Lv2ControlID* control)
{
  controls->controls = (Lv2ControlID**)realloc(
          controls->controls, (controls->n_controls + 1) * sizeof(Lv2ControlID*));
  controls->controls[controls->n_controls++] = control;
}

Lv2ControlID*
lv2_get_property_control(const Lv2Controls* controls, LV2_URID property)
{
	for (size_t i = 0; i < controls->n_controls; ++i) {
		if (controls->controls[i]->property == property) {
			return controls->controls[i];
		}
	}

	return NULL;
}

void
lv2_set_control(const Lv2ControlID* control,
                 uint32_t         size,
                 LV2_URID         type,
                 const void*      body)
{
	LV2_Plugin* plugin = control->plugin;
	if (control->type == PORT && type == plugin->forge.Float) {
		LV2_Port* port = &control->plugin->ports[control->index];
		port->control = *(float*)body;
	} else if (control->type == PROPERTY) {
		// Copy forge since it is used by process thread
		LV2_Atom_Forge       forge = plugin->forge;
		LV2_Atom_Forge_Frame frame;
		uint8_t              buf[1024];
		lv2_atom_forge_set_buffer(&forge, buf, sizeof(buf));

		lv2_atom_forge_object(&forge, &frame, 0, plugin->urids.patch_Set);
		lv2_atom_forge_key(&forge, plugin->urids.patch_property);
		lv2_atom_forge_urid(&forge, control->property);
		lv2_atom_forge_key(&forge, plugin->urids.patch_value);
		lv2_atom_forge_atom(&forge, size, type);
		lv2_atom_forge_write(&forge, body, size);

		const LV2_Atom* atom = lv2_atom_forge_deref(&forge, frame.ref);
		lv2_ui_write(plugin,
		              plugin->control_in,
		              lv2_atom_total_size(atom),
		              plugin->urids.atom_eventTransfer,
		              atom);
	}
}
