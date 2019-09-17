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

/** \file
 * Implementation of Plugin. */

#define _GNU_SOURCE 1  /* To pick up REG_RIP */

#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/widgets/instrument_track.h"
#include "plugins/plugin.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/control.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/io.h"
#include "utils/flags.h"

#include <gtk/gtk.h>

static void
get_automation_tracks (
  Plugin * self)
{
  AutomationTracklist * atl =
    track_get_automation_tracklist (self->track);
  AutomationTrack * at;
  for (int i = 0; i < atl->num_ats; i++)
    {
      at = atl->ats[i];

      if (at->automatable->type !=
            AUTOMATABLE_TYPE_PLUGIN_CONTROL &&
          at->automatable->type !=
            AUTOMATABLE_TYPE_PLUGIN_ENABLED)
        continue;

      array_double_size_if_full (
        self->ats,
        self->num_ats,
        self->ats_size,
        AutomationTrack *);
      array_append (
        self->ats,
        self->num_ats,
        at);
    }
}

void
plugin_init_loaded (Plugin * pl)
{
  lv2_plugin_init_loaded (pl->lv2);
  pl->lv2->plugin = pl;

  pl->ats_size = 1;
  pl->ats =
    calloc (1, sizeof (AutomationTrack *));

  plugin_instantiate (pl);

  get_automation_tracks (pl);
}

static void
plugin_init (
  Plugin * plugin)
{
  plugin->in_ports_size = 1;
  plugin->out_ports_size = 1;
  plugin->unknown_ports_size = 1;
  plugin->ats_size = 1;

  plugin->in_ports =
    calloc (1, sizeof (Port *));
  /*plugin->in_port_ids =*/
    /*calloc (1, sizeof (PortIdentifier));*/
  plugin->out_ports =
    calloc (1, sizeof (Port *));
  /*plugin->out_port_ids =*/
    /*calloc (1, sizeof (PortIdentifier));*/
  plugin->unknown_ports =
    calloc (1, sizeof (Port *));
  /*plugin->unknown_port_ids =*/
    /*calloc (1, sizeof (PortIdentifier));*/
  plugin->ats =
    calloc (1, sizeof (AutomationTrack *));
}

/**
 * Creates/initializes a plugin and its internal
 * plugin (LV2, etc.)
 * using the given descriptor.
 */
Plugin *
plugin_new_from_descr (
  const PluginDescriptor * descr)
{
  Plugin * plugin = calloc (1, sizeof (Plugin));

  plugin->descr =
    plugin_clone_descr (descr);
  plugin_init (plugin);

  if (plugin->descr->protocol == PROT_LV2)
    {
      lv2_create_from_uri (plugin, descr->uri);
    }
  return plugin;
}

/**
 * Removes the automation tracks associated with
 * this plugin from the automation tracklist in the
 * corresponding track.
 *
 * Used e.g. when moving plugins.
 */
void
plugin_remove_ats_from_automation_tracklist (
  Plugin * pl)
{
  for (int i = 0; i < pl->num_ats; i++)
    {
      automation_tracklist_remove_at (
        &pl->track->automation_tracklist,
        pl->ats[i], F_NO_FREE);
    }
}

/**
 * Clones the plugin descriptor.
 */
void
plugin_copy_descr (
  const PluginDescriptor * src,
  PluginDescriptor * dest)
{
  dest->author = g_strdup (src->author);
  dest->name = g_strdup (src->name);
  dest->website = g_strdup (src->website);
  dest->category_str = g_strdup (src->category_str);
  dest->category = src->category;
  dest->num_audio_ins = src->num_audio_ins;
  dest->num_midi_ins = src->num_midi_ins;
  dest->num_audio_outs = src->num_audio_outs;
  dest->num_midi_outs = src->num_midi_outs;
  dest->num_ctrl_ins = src->num_ctrl_ins;
  dest->num_ctrl_outs = src->num_ctrl_outs;
  dest->num_cv_ins = src->num_cv_ins;
  dest->num_cv_outs = src->num_cv_outs;
  dest->arch = src->arch;
  dest->protocol = src->protocol;
  dest->path = g_strdup (src->path);
  dest->uri = g_strdup (src->uri);
}

/**
 * Clones the plugin descriptor.
 */
PluginDescriptor *
plugin_clone_descr (
  const PluginDescriptor * src)
{
  PluginDescriptor * self =
    calloc (1, sizeof (PluginDescriptor));

  plugin_copy_descr (
    src, self);

  return self;
}

/**
 * Adds an AutomationTrack to the Plugin.
 */
void
plugin_add_automation_track (
  Plugin * self,
  AutomationTrack * at)
{
  g_warn_if_fail (self->track);

  array_double_size_if_full (
    self->ats,
    self->num_ats,
    self->ats_size,
    AutomationTrack *);
  array_append (
    self->ats,
    self->num_ats,
    at);
  AutomationTracklist * atl =
    track_get_automation_tracklist (self->track);
  automation_tracklist_add_at (atl, at);
}

/**
 * Updates the plugin's latency.
 *
 * Calls the plugin format's get_latency()
 * function and stores the result in the plugin.
 */
void
plugin_update_latency (
  Plugin * pl)
{
  if (pl->descr->protocol == PROT_LV2)
    {
      pl->latency =
        lv2_plugin_get_latency (pl->lv2);
      g_message ("%s latency: %d samples",
                 pl->descr->name,
                 pl->latency);
    }
}

/**
 * Loads the plugin from its state file.
 */
/*void*/
/*plugin_load (Plugin * plugin)*/
/*{*/
  /*switch (plugin->descr->protocol)*/
    /*{*/
    /*case PROT_LV2:*/

      /*lv2_load_from_state (plugin, descr->uri);*/
      /*break;*/
    /*}*/
  /*return plugin;*/

/*}*/

/**
 * Adds a port of the given type to the Plugin.
 */
#define ADD_PORT(type) \
  if (pl->num_##type##_ports == \
        (int) pl->type##_ports_size) \
    { \
      pl->type##_ports_size *= 2; \
      pl->type##_ports = \
        realloc ( \
          pl->type##_ports, \
          sizeof (Port *) * \
            pl->type##_ports_size); \
    } \
  port->identifier.port_index = \
    pl->num_##type##_ports; \
  array_append ( \
    pl->type##_ports, \
    pl->num_##type##_ports, \
    port)

/**
 * Adds an in port to the plugin's list.
 */
void
plugin_add_in_port (
  Plugin * pl,
  Port *   port)
{
  ADD_PORT (in);
}

/**
 * Adds an out port to the plugin's list.
 */
void
plugin_add_out_port (
  Plugin * pl,
  Port *   port)
{
  ADD_PORT (out);
}

/**
 * Adds an unknown port to the plugin's list.
 */
void
plugin_add_unknown_port (
  Plugin * pl,
  Port *   port)
{
  ADD_PORT (unknown);
}
#undef ADD_PORT

/**
 * Moves the Plugin's automation from one Channel
 * to another.
 */
void
plugin_move_automation (
  Plugin *  pl,
  Channel * prev_ch,
  Channel * ch)
{
  AutomationTracklist * prev_atl =
    &prev_ch->track->automation_tracklist;
  AutomationTracklist * atl =
    &ch->track->automation_tracklist;

  int i;
  AutomationTrack * at;
  for (i = 0; i < prev_atl->num_ats; i++)
    {
      at = prev_atl->ats[i];

      if (!at->automatable->port ||
          at->automatable->port->plugin != pl)
        continue;

      /* delete from prev channel */
      automation_tracklist_delete_at (
        prev_atl, at, F_NO_FREE);

      /* add to new channel */
      automation_tracklist_add_at (
        atl, at);
    }
}

/**
 * Generates automatables for the plugin.
 *
 *
 * Plugin must be instantiated already.
 */
void
plugin_generate_automation_tracks (
  Plugin * plugin)
{
  g_message ("generating automatables for %s...",
             plugin->descr->name);

  AutomationTrack * at;
  Automatable * a;

  /* add plugin enabled automatable */
  a =
    automatable_create_plugin_enabled (plugin);
  at = automation_track_new (a);
  plugin_add_automation_track (
    plugin, at);

  /* add plugin control automatables */
  if (plugin->descr->protocol == PROT_LV2)
    {
      Lv2Plugin * lv2_plugin = (Lv2Plugin *) plugin->lv2;
      for (int j = 0; j < lv2_plugin->controls.n_controls; j++)
        {
          Lv2Control * control =
            lv2_plugin->controls.controls[j];
          a =
            automatable_create_lv2_control (
              plugin, control);
          at = automation_track_new (a);
          plugin_add_automation_track (
            plugin, at);
        }
    }
  else
    {
      g_warning ("Plugin protocol not supported yet (gen automatables)");
    }
}

#define IS_CAT(x) \
  (descr->category == PC_##x)

/**
 * Returns if the Plugin is an instrument or not.
 */
int
plugin_descriptor_is_instrument (
  const PluginDescriptor * descr)
{
  return
    (descr->category > PC_NONE &&
     descr->category == PC_INSTRUMENT) ||
    (descr->category == PC_NONE &&
     descr->num_midi_ins > 0 &&
     descr->num_audio_outs > 0);
}

/**
 * Returns if the Plugin is an effect or not.
 */
int
plugin_descriptor_is_effect (
  PluginDescriptor * descr)
{

  return
    (descr->category > PC_NONE &&
     (IS_CAT (DELAY) ||
      IS_CAT (REVERB) ||
      IS_CAT (DISTORTION) ||
      IS_CAT (WAVESHAPER) ||
      IS_CAT (DYNAMICS) ||
      IS_CAT (AMPLIFIER) ||
      IS_CAT (COMPRESSOR) ||
      IS_CAT (ENVELOPE) ||
      IS_CAT (EXPANDER) ||
      IS_CAT (GATE) ||
      IS_CAT (LIMITER) ||
      IS_CAT (FILTER) ||
      IS_CAT (ALLPASS_FILTER) ||
      IS_CAT (BANDPASS_FILTER) ||
      IS_CAT (COMB_FILTER) ||
      IS_CAT (EQ) ||
      IS_CAT (MULTI_EQ) ||
      IS_CAT (PARA_EQ) ||
      IS_CAT (HIGHPASS_FILTER) ||
      IS_CAT (LOWPASS_FILTER) ||
      IS_CAT (GENERATOR) ||
      IS_CAT (CONSTANT) ||
      IS_CAT (OSCILLATOR) ||
      IS_CAT (MODULATOR) ||
      IS_CAT (CHORUS) ||
      IS_CAT (FLANGER) ||
      IS_CAT (PHASER) ||
      IS_CAT (SIMULATOR) ||
      IS_CAT (SIMULATOR_REVERB) ||
      IS_CAT (SPATIAL) ||
      IS_CAT (SPECTRAL) ||
      IS_CAT (PITCH) ||
      IS_CAT (UTILITY) ||
      IS_CAT (ANALYZER) ||
      IS_CAT (CONVERTER) ||
      IS_CAT (FUNCTION) ||
      IS_CAT (MIXER))) ||
    (descr->category == PC_NONE &&
     descr->num_audio_ins > 0 &&
     descr->num_audio_outs > 0);
}

/**
 * Returns if the Plugin is a modulator or not.
 */
int
plugin_descriptor_is_modulator (
  PluginDescriptor * descr)
{
  return
    (descr->category == PC_NONE ||
     (descr->category > PC_NONE &&
      (IS_CAT (ENVELOPE) ||
       IS_CAT (GENERATOR) ||
       IS_CAT (CONSTANT) ||
       IS_CAT (OSCILLATOR) ||
       IS_CAT (MODULATOR) ||
       IS_CAT (UTILITY) ||
       IS_CAT (CONVERTER) ||
       IS_CAT (FUNCTION)))) &&
    descr->num_cv_outs > 0;
}

/**
 * Returns if the Plugin is a midi modifier or not.
 */
int
plugin_descriptor_is_midi_modifier (
  PluginDescriptor * descr)
{
  return
    (descr->category > PC_NONE &&
     descr->category == PC_MIDI) ||
    (descr->category == PC_NONE &&
     descr->num_midi_ins > 0 &&
     descr->num_midi_outs > 0);
}

#undef IS_CAT

/**
 * Returns the PluginCategory matching the given
 * string.
 */
PluginCategory
plugin_descriptor_string_to_category (
  const char * str)
{
  PluginCategory category = PC_NONE;

#define CHECK_CAT(term,cat) \
  if (g_strrstr (str, term)) \
    category = PC_##cat

  /* add category */
  CHECK_CAT ("Delay", DELAY);
  CHECK_CAT ("Reverb", REVERB);
  CHECK_CAT ("Distortion", DISTORTION);
  CHECK_CAT ("Waveshaper", WAVESHAPER);
  CHECK_CAT ("Dynamics", DYNAMICS);
  CHECK_CAT ("Amplifier", AMPLIFIER);
  CHECK_CAT ("Compressor", COMPRESSOR);
  CHECK_CAT ("Envelope", ENVELOPE);
  CHECK_CAT ("Expander", EXPANDER);
  CHECK_CAT ("Gate", GATE);
  CHECK_CAT ("Limiter", LIMITER);
  CHECK_CAT ("Filter", FILTER);
  CHECK_CAT ("Allpass", ALLPASS_FILTER);
  CHECK_CAT ("Bandpass", BANDPASS_FILTER);
  CHECK_CAT ("Comb", COMB_FILTER);
  CHECK_CAT ("Equaliser", EQ);
  CHECK_CAT ("Equalizer", EQ);
  CHECK_CAT ("Multiband", MULTI_EQ);
  CHECK_CAT ("Para", PARA_EQ);
  CHECK_CAT ("Highpass", HIGHPASS_FILTER);
  CHECK_CAT ("Lowpass", LOWPASS_FILTER);
  CHECK_CAT ("Generator", GENERATOR);
  CHECK_CAT ("Constant", CONSTANT);
  CHECK_CAT ("Instrument", INSTRUMENT);
  CHECK_CAT ("Oscillator", OSCILLATOR);
  CHECK_CAT ("MIDI", MIDI);
  CHECK_CAT ("Modulator", MODULATOR);
  CHECK_CAT ("Chorus", CHORUS);
  CHECK_CAT ("Flanger", FLANGER);
  CHECK_CAT ("Phaser", PHASER);
  CHECK_CAT ("Simulator", SIMULATOR);
  CHECK_CAT ("SimulatorReverb", SIMULATOR_REVERB);
  CHECK_CAT ("Spatial", SPATIAL);
  CHECK_CAT ("Spectral", SPECTRAL);
  CHECK_CAT ("Pitch", PITCH);
  CHECK_CAT ("Utility", UTILITY);
  CHECK_CAT ("Analyser", ANALYZER);
  CHECK_CAT ("Analyzer", ANALYZER);
  CHECK_CAT ("Converter", CONVERTER);
  CHECK_CAT ("Function", FUNCTION);
  CHECK_CAT ("Mixer", MIXER);

#undef CHECK_CAT

  return category;
}

/**
 * Sets the track and track_pos on the plugin.
 */
void
plugin_set_track (
  Plugin * pl,
  Track * tr)
{
  g_return_if_fail (tr);
  pl->track = tr;
  pl->track_pos = tr->pos;

  /* set port identifier track poses */
  int i;
  for (i = 0; i < pl->num_in_ports; i++)
    {
      /*pl->in_port_ids[i].track_pos = tr->pos;*/
    }
  for (i = 0; i < pl->num_out_ports; i++)
    {
      /*pl->out_port_ids[i].track_pos = tr->pos;*/
    }
  for (i = 0; i < pl->num_unknown_ports; i++)
    {
      /*pl->unknown_port_ids[i].track_pos = tr->pos;*/
    }
}

/**
 * Instantiates the plugin (e.g. when adding to a
 * channel).
 */
int
plugin_instantiate (
  Plugin * pl)
{
  g_message ("Instantiating %s...",
             pl->descr->name);

  switch (pl->descr->protocol)
    {
    case PROT_LV2:
      {
        g_message ("state file: %s",
                   pl->lv2->state_file);
        if (lv2_plugin_instantiate (
              pl->lv2, NULL))
          {
            g_warning ("lv2 instantiate failed");
            return -1;
          }
      }
      break;
    default:
      g_warn_if_reached ();
      return -1;
      break;
    }
  pl->enabled = 1;

  return 0;
}

/**
 * Process plugin.
 *
 * @param g_start_frames The global start frames.
 * @param nframes The number of frames to process.
 */
void
plugin_process (
  Plugin *        plugin,
  const long      g_start_frames,
  const nframes_t nframes)
{
  /* if has MIDI input port */
  if (plugin->descr->num_midi_ins > 0)
    {
      /* if recording, write MIDI events to the region TODO */

        /* if there is a midi note in this buffer range TODO */
          /* add midi events to input port */
    }

  if (plugin->descr->protocol == PROT_LV2)
    {
      lv2_plugin_process (
        plugin->lv2, g_start_frames, nframes);
    }

  /*g_atomic_int_set (&plugin->processed, 1);*/
  /*zix_sem_post (&plugin->processed_sem);*/
}

/**
 * shows plugin ui and sets window close callback
 */
void
plugin_open_ui (Plugin *plugin)
{
  if (plugin->descr->protocol == PROT_LV2)
    {
      Lv2Plugin * lv2_plugin = (Lv2Plugin *) plugin->lv2;
      if (GTK_IS_WINDOW (lv2_plugin->window))
        {
          gtk_window_present (
            GTK_WINDOW (lv2_plugin->window));
        }
      else
        {
          lv2_open_ui (lv2_plugin);
        }
    }
}

/**
 * Returns if Plugin exists in MixerSelections.
 */
int
plugin_is_selected (
  Plugin * pl)
{
  return mixer_selections_contains_plugin (
    MIXER_SELECTIONS,
    pl);
}

/**
 * Clones the given plugin.
 */
Plugin *
plugin_clone (
  Plugin * pl)
{
  Plugin * clone = NULL;
  if (pl->descr->protocol == PROT_LV2)
    {
      /* NOTE from rgareus:
       * I think you can use   lilv_state_restore (lilv_state_new_from_instance (..), ...)
       * and skip  lilv_state_new_from_file() ; lilv_state_save ()
       * lilv_state_new_from_instance() handles files and externals, too */
      /* save state to file */
      char * tmp =
        g_strdup_printf (
          "tmp_%s_XXXXXX",
          pl->descr->name);
      char * states_dir =
        project_get_states_dir (
          PROJECT, 0);
      char * state_dir_plugin =
        g_build_filename (states_dir,
                          tmp,
                          NULL);
      g_free (states_dir);
      io_mkdir (state_dir_plugin);
      g_free (tmp);
      lv2_plugin_save_state_to_file (
        pl->lv2,
        state_dir_plugin);
      g_free (state_dir_plugin);
      if (!pl->lv2->state_file)
        {
          g_warn_if_reached ();
          return NULL;
        }

      /* create a new plugin with same descriptor */
      clone = plugin_new_from_descr (pl->descr);
      g_return_val_if_fail (clone, NULL);

      /* set the state file on the new Lv2Plugin
       * as the state filed saved on the original
       * so that it can be used when
       * instantiating */
      clone->lv2->state_file =
        g_strdup (pl->lv2->state_file);

      /* instantiate */
      int ret = plugin_instantiate (clone);
      g_return_val_if_fail (!ret, NULL);

      /* delete the state file */
      io_remove (pl->lv2->state_file);
    }

  g_return_val_if_fail (clone, NULL);
  clone->slot = pl->slot;
  clone->track_pos = pl->track_pos;

  return clone;
}

/**
 * hides plugin ui
 */
void
plugin_close_ui (Plugin *plugin)
{
  if (plugin->descr->protocol == PROT_LV2)
    {
      Lv2Plugin * lv2_plugin =
        (Lv2Plugin *) plugin->lv2;
      if (GTK_IS_WINDOW (lv2_plugin->window))
        gtk_window_close (
          GTK_WINDOW (lv2_plugin->window));
      else
        lv2_close_ui (lv2_plugin);
    }
}

/**
 * To be called immediately when a channel or plugin
 * is deleted.
 *
 * A call to plugin_free can be made at any point
 * later just to free the resources.
 */
void
plugin_disconnect (Plugin * plugin)
{
  plugin->deleting = 1;

  /* disconnect all ports */
  ports_disconnect (
    plugin->in_ports,
    plugin->num_in_ports, 1);
  ports_disconnect (
    plugin->out_ports,
    plugin->num_out_ports, 1);
  g_message (
    "DISCONNECTED ALL PORTS OF %p PLUGIN %d %d",
    plugin,
    plugin->num_in_ports,
    plugin->num_out_ports);
}

/**
 * Frees given plugin, frees its ports
 * and other internal pointers
 */
void
plugin_free (Plugin *plugin)
{
  g_message ("FREEING PLUGIN %s",
             plugin->descr->name);
  g_warn_if_fail (plugin);

  ports_remove (
    plugin->in_ports,
    &plugin->num_in_ports);
  ports_remove (
    plugin->out_ports,
    &plugin->num_out_ports);

  /* delete automation tracks */
  plugin_remove_ats_from_automation_tracklist (
    plugin);
  for (int i = 0; i < plugin->num_ats; i++)
    {
      automation_track_free (plugin->ats[i]);
    }
  free (plugin->ats);

  free (plugin);
}
