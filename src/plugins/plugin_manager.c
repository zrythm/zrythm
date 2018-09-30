/*
 * plugins/plugin_manager.c - Manages plugins
 *
 * Copyright (C) 2018 Alexandros Theodotou
 * Copyright (C) 2008-2012 Paul Davis
 * Author: David Robillard
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

#include <stdlib.h>
#include <ctype.h>

#include "zrythm_app.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "plugins/lv2_plugin.h"
#include "utils/string.h"

#include <gtk/gtk.h>

/**
 * If category not already set in the categories, add it.
 */
static void
add_category (char * _category)
{
  if (!is_ascii (_category))
    {
      g_warning ("Invalid LV2 category name, skipping...");
    }
  for (int i = 0; i < PLUGIN_MANAGER->num_plugin_categories; i++)
    {
      char * category = PLUGIN_MANAGER->plugin_categories[i];
      if (!strcmp (category, _category))
        {
          return;
        }
    }
    PLUGIN_MANAGER->plugin_categories[
      PLUGIN_MANAGER->num_plugin_categories++] =
        g_strdup (_category);
}

int sort_category_func (const void *a, const void *b) {
    char * pa = *(char * const *) a, * pb = *(char * const *) b;
    if (strcasecmp)
      {
        int r = strcasecmp(pa, pb);
        if (r) return r;
      }
    /* if equal ignoring case, use opposite of strcmp() result to get
     * lower before upper */
    return -strcmp(pa, pb); /* aka: return strcmp(b, a); */
}

int sort_plugin_func (const void *a, const void *b) {
    Plugin_Descriptor * pa = *(Plugin_Descriptor * const *) a,
                      * pb = *(Plugin_Descriptor * const *) b;
    if (strcasecmp)
      {
        int r = strcasecmp(pa->name, pb->name);
        if (r) return r;
      }
    /* if equal ignoring case, use opposite of strcmp() result to get
     * lower before upper */
    return -strcmp(pa->name, pb->name); /* aka: return strcmp(b, a); */
}

/**
 * scans for plugins.
 */
static void
scan_plugins ()
{
  g_message ("scanning plugins...");

  /* load all plugins with lilv */
  LilvWorld * world = LILV_WORLD;
  const LilvPlugins * plugins = lilv_world_get_all_plugins (world);
  const LilvPluginClasses * plugin_classes =
                              lilv_world_get_plugin_classes (world);
  LV2_SETTINGS.lilv_plugins = plugins;


  /* iterate plugins */
  LILV_FOREACH(plugins, i, plugins)
    {
      const LilvPlugin* p = lilv_plugins_get(plugins, i);

      Plugin_Descriptor * descriptor =
        lv2_create_descriptor_from_lilv (p);

      if (descriptor)
        {
          PLUGIN_MANAGER->plugin_descriptors[PLUGIN_MANAGER->num_plugins++] =
            descriptor;
          add_category (descriptor->category);
        }
    }

  /* sort alphabetically */
  qsort (PLUGIN_MANAGER->plugin_descriptors,
         PLUGIN_MANAGER->num_plugins,
         sizeof (Plugin *),
         sort_plugin_func);
  qsort (PLUGIN_MANAGER->plugin_categories,
         PLUGIN_MANAGER->num_plugin_categories,
         sizeof (char *),
         sort_category_func);

  g_message ("%d Plugins scanned.", PLUGIN_MANAGER->num_plugins);
}

void
plugin_manager_init ()
{
    g_message ("Initializing plugin manager...");
    Plugin_Manager * plugin_manager =
      malloc (sizeof (Plugin_Manager));

    plugin_manager->num_plugins = 0;
    plugin_manager->num_plugin_categories = 0;

    /* set plugin manager to the project */
    zrythm_system->plugin_manager = plugin_manager;

    /* init lv2 settings */
    g_message ("Creating Lilv World...");
    LilvWorld * world = lilv_world_new ();
    LV2_SETTINGS.lilv_world = world;
    lilv_world_load_all (world);

    g_message ("Initializing LV2 settings...");
    LV2_Defaults * opts = &plugin_manager->lv2_settings.opts;
    opts->uuid = NULL;
    opts->buffer_size = 0;
    opts->controls = NULL;
    opts->update_rate = 0.0;
    opts->dump = 1;
    opts->trace = 1;
    opts->generic_ui = 0;
    opts->show_hidden = 1;
    opts->no_menu = 0;
    opts->show_ui = 1;
    opts->print_controls = 1;
    opts->non_interactive = 1;

    LV2_Settings * settings = &plugin_manager->lv2_settings;
    settings->atom_AtomPort      = lilv_new_uri(LILV_WORLD, LV2_ATOM__AtomPort);
    settings->atom_Chunk         = lilv_new_uri(LILV_WORLD,LV2_ATOM__Chunk);
    settings->atom_Sequence      = lilv_new_uri(LILV_WORLD,LV2_ATOM__Sequence);
    settings->atom_bufferType    = lilv_new_uri(LILV_WORLD,LV2_ATOM__bufferType);
    settings->atom_supports      = lilv_new_uri(LILV_WORLD,LV2_ATOM__supports);
    settings->atom_eventTransfer = lilv_new_uri(LILV_WORLD,LV2_ATOM__eventTransfer);
    settings->ev_EventPort       = lilv_new_uri(LILV_WORLD,LILV_URI_EVENT_PORT);
    settings->ext_logarithmic    = lilv_new_uri(LILV_WORLD,LV2_PORT_PROPS__logarithmic);
    settings->ext_notOnGUI       = lilv_new_uri(LILV_WORLD,LV2_PORT_PROPS__notOnGUI);
    settings->ext_expensive      = lilv_new_uri(LILV_WORLD,LV2_PORT_PROPS__expensive);
    settings->ext_causesArtifacts= lilv_new_uri(LILV_WORLD,LV2_PORT_PROPS__causesArtifacts);
    settings->ext_notAutomatic   = lilv_new_uri(LILV_WORLD,LV2_PORT_PROPS__notAutomatic);
    settings->ext_rangeSteps     = lilv_new_uri(LILV_WORLD,LV2_PORT_PROPS__rangeSteps);
    settings->groups_group       = lilv_new_uri(LILV_WORLD,LV2_PORT_GROUPS__group);
    settings->groups_element     = lilv_new_uri(LILV_WORLD,LV2_PORT_GROUPS__element);
    settings->lv2_AudioPort      = lilv_new_uri(LILV_WORLD,LILV_URI_AUDIO_PORT);
    settings->lv2_ControlPort    = lilv_new_uri(LILV_WORLD,LILV_URI_CONTROL_PORT);
    settings->lv2_InputPort      = lilv_new_uri(LILV_WORLD,LILV_URI_INPUT_PORT);
    settings->lv2_OutputPort     = lilv_new_uri(LILV_WORLD,LILV_URI_OUTPUT_PORT);
    settings->lv2_inPlaceBroken  = lilv_new_uri(LILV_WORLD,LV2_CORE__inPlaceBroken);
    settings->lv2_isSideChain    = lilv_new_uri(LILV_WORLD, LV2_CORE_PREFIX"isSideChain");
    settings->lv2_index          = lilv_new_uri(LILV_WORLD,LV2_CORE__index);
    settings->lv2_integer        = lilv_new_uri(LILV_WORLD,LV2_CORE__integer);
    settings->lv2_default        = lilv_new_uri(LILV_WORLD,LV2_CORE__default);
    settings->lv2_minimum        = lilv_new_uri(LILV_WORLD,LV2_CORE__minimum);
    settings->lv2_maximum        = lilv_new_uri(LILV_WORLD,LV2_CORE__maximum);
    settings->lv2_reportsLatency = lilv_new_uri(LILV_WORLD,LV2_CORE__reportsLatency);
    settings->lv2_sampleRate     = lilv_new_uri(LILV_WORLD,LV2_CORE__sampleRate);
    settings->lv2_toggled        = lilv_new_uri(LILV_WORLD,LV2_CORE__toggled);
    settings->lv2_designation    = lilv_new_uri(LILV_WORLD,LV2_CORE__designation);
    settings->lv2_enumeration    = lilv_new_uri(LILV_WORLD,LV2_CORE__enumeration);
    settings->lv2_freewheeling   = lilv_new_uri(LILV_WORLD,LV2_CORE__freeWheeling);
    settings->midi_MidiEvent     = lilv_new_uri(LILV_WORLD,LILV_URI_MIDI_EVENT);
    settings->rdfs_comment       = lilv_new_uri(LILV_WORLD, LILV_NS_RDFS"comment");
    settings->rdfs_label         = lilv_new_uri(LILV_WORLD, LILV_NS_RDFS"label");
    settings->rdfs_range         = lilv_new_uri(LILV_WORLD, LILV_NS_RDFS"range");
    settings->rsz_minimumSize    = lilv_new_uri(LILV_WORLD,LV2_RESIZE_PORT__minimumSize);
    settings->time_Position      = lilv_new_uri(LILV_WORLD,LV2_TIME__Position);
    settings->ui_GtkUI           = lilv_new_uri(LILV_WORLD,LV2_UI__GtkUI);
    settings->ui_external        = lilv_new_uri(LILV_WORLD,"http://lv2plug.in/ns/extensions/ui#external");
    settings->ui_externalkx      = lilv_new_uri(LILV_WORLD,"http://kxstudio.sf.net/ns/lv2ext/external-ui#Widget");
    settings->units_unit         = lilv_new_uri(LILV_WORLD,LV2_UNITS__unit);
    settings->units_render       = lilv_new_uri(LILV_WORLD,LV2_UNITS__render);
    settings->units_hz           = lilv_new_uri(LILV_WORLD,LV2_UNITS__hz);
    settings->units_midiNote     = lilv_new_uri(LILV_WORLD,LV2_UNITS__midiNote);
    settings->units_db           = lilv_new_uri(LILV_WORLD,LV2_UNITS__db);
    settings->patch_writable     = lilv_new_uri(LILV_WORLD,LV2_PATCH__writable);
    settings->patch_Message      = lilv_new_uri(LILV_WORLD,LV2_PATCH__Message);
#ifdef LV2_EXTENDED
    settings->lv2_noSampleAccurateCtrl    = lilv_new_uri(LILV_WORLD, "http://ardour.org/lv2/ext#noSampleAccurateControls"); // deprecated2016-09-18
    settings->auto_can_write_automatation = lilv_new_uri(LILV_WORLD,LV2_AUTOMATE_URI__can_write);
    settings->auto_automation_control     = lilv_new_uri(LILV_WORLD,LV2_AUTOMATE_URI__control);
    settings->auto_automation_controlled  = lilv_new_uri(LILV_WORLD,LV2_AUTOMATE_URI__controlled);
    settings->auto_automation_controller  = lilv_new_uri(LILV_WORLD,LV2_AUTOMATE_URI__controller);
    settings->inline_display_in_gui       = lilv_new_uri(LILV_WORLD,LV2_INLINEDISPLAY__in_gui);
#endif
#ifdef HAVE_LV2_1_2_0
    settings->bufz_powerOf2BlockLength = lilv_new_uri(LILV_WORLD,LV2_BUF_SIZE__powerOf2BlockLength);
    settings->bufz_fixedBlockLength    = lilv_new_uri(LILV_WORLD,LV2_BUF_SIZE__fixedBlockLength);
    settings->bufz_nominalBlockLength  = lilv_new_uri(LILV_WORLD,"http://lv2plug.in/ns/ext/buf-size#nominalBlockLength");
    settings->bufz_coarseBlockLength   = lilv_new_uri(LILV_WORLD,"http://lv2plug.in/ns/ext/buf-size#coarseBlockLength");
#endif
    scan_plugins ();
}
