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
 * Copyright (C) 2011-2019 Filipe Coelho <falktx@falktx.com>
 * Copyright (C) 2010 Paul Davis
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "config.h"

#include<stdio.h>
#include<stdlib.h>
#ifndef _WOE32
#include<sys/wait.h>
#endif
#include<unistd.h>

#include "audio/engine.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "plugins/plugin_gtk.h"
#include "plugins/vst_plugin.h"
#ifdef HAVE_X11
#include "plugins/vst/vst_x11.h"
#elif _WOE32
#include "plugins/vst/vst_windows.h"
#endif
#include "project.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/system.h"
#include "utils/windows_errors.h"

#ifdef HAVE_X11
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#endif

#include <dlfcn.h>

static int
can_do (
  AEffect *    effect,
  const char * str)
{
  return
    effect->dispatcher (
      effect, effCanDo, 0, 0, (char *) str, 0.f);
}

static void
get_name (
  VstPlugin * self,
  char *      name)
{
  AEffect * effect = self->aeffect;
  effect->dispatcher (
    effect, effGetEffectName, 0, 0, name, 0.f);
}

static void
get_author (
  VstPlugin * self,
  char *      str)
{
  AEffect * effect = self->aeffect;
  effect->dispatcher (
    effect, effGetVendorString, 0, 0, str, 0.f);
}

static void
get_param_name (
  VstPlugin * self,
  int         idx,
  char *      name)
{
  AEffect * effect = self->aeffect;
  effect->dispatcher (
    effect, effGetParamName, idx, 0, name, 0.f);
}

static intptr_t
host_can_do (
  const char * feature)
{
  if (!strcmp (feature, "supplyIdle"))
    return 1;
  if (!strcmp (feature, "sendVstEvents"))
    return 1;
  if (!strcmp (feature, "sendVstMidiEvent"))
    return 1;
  if (!strcmp (feature, "sendVstMidiEventFlagIsRealtime"))
    return 1;
  if (!strcmp (feature, "sendVstTimeInfo"))
    return 1;
  if (!strcmp (feature, "receiveVstEvents"))
    return -1;
  if (!strcmp (feature, "receiveVstMidiEvent"))
    return -1;
  if (!strcmp (feature, "receiveVstTimeInfo"))
    return -1;
  if (!strcmp (feature, "reportConnectionChanges"))
    return -1;
  if (!strcmp (feature, "acceptIOChanges"))
    return -1;
  if (!strcmp (feature, "sizeWindow"))
    return 1;
  if (!strcmp (feature, "offline"))
    return -1;
  if (!strcmp (feature, "openFileSelector"))
    return -1;
  if (!strcmp (feature, "closeFileSelector"))
    return -1;
  if (!strcmp (feature, "startStopProcess"))
    return 1;
  if (!strcmp (feature, "supportShell"))
    return -1;
  if (!strcmp (feature, "shellCategory"))
    return 1;
  if (!strcmp (feature, "NIMKPIVendorSpecificCallbacks"))
    return -1;

  g_return_val_if_reached (0);
}

/**
 * Host callback implementation.
 */
static intptr_t
host_callback (
  AEffect * effect,
  int32_t   opcode,
  int32_t   index,
  intptr_t  value,
  void *    ptr,
  float     opt)
{
  VstPlugin * self = NULL;
  if (effect && effect->user)
    {
      self = (VstPlugin *) effect->user;
    }

  switch ((int) opcode)
    {
    case audioMasterVersion:
      return kVstVersion;
    case audioMasterGetVendorString:
      strcpy ((char *) ptr, "Alexandros Theodotou");
      return 1;
    case audioMasterGetProductString:
      strcpy ((char *) ptr, "Zrythm");
      return 1;
    case audioMasterGetVendorVersion:
      return 900;
    case audioMasterWantMidi:
      /* deprecated */
      break;
    case audioMasterGetCurrentProcessLevel:
      if (router_is_processing_thread (
            &MIXER->router))
        {
          if (AUDIO_ENGINE->run)
            return kVstProcessLevelRealtime;
          else
            return kVstProcessLevelOffline;
        }
      else
        {
          return kVstProcessLevelUser;
        }
      break;
    case audioMasterCurrentId:
      return effect->uniqueID;
    case audioMasterGetTime:
      return (intptr_t) &self->time_info;
    case audioMasterIdle:
      g_message (
        "calling VST plugin idle (requested)");
      effect->dispatcher (
        effect, effEditIdle, 0, 0, 0, 0);
      break;
    case audioMasterBeginEdit:
      {
        Port * port =
          vst_plugin_get_port_from_param_id (
            self, index);
        g_message (
          "grabbing %s",
          port->identifier.label);
      }
      break;
    case audioMasterEndEdit:
      {
        Port * port =
          vst_plugin_get_port_from_param_id (
            self, index);
        g_message (
          "ungrabbing %s",
          port->identifier.label);
      }
      break;
    case audioMasterCanDo:
      return host_can_do ((const char *) ptr);
    case audioMasterAutomate:
      /* some plugins call this during the test
       * for some reason */
      if (self)
        {
          Port * port =
            vst_plugin_get_port_from_param_id (
              self, index);
          g_return_val_if_fail (port, -1);
          port->control = opt;
          g_message (
            "setting %s to %f",
            port->identifier.label, (double) opt);
        }
      break;
    default:
      g_message ("Plugin requested value of opcode %d", opcode);
      break;
  }
  return 0;
}

static int
close_lib (
  void * handle)
{
#ifdef _WOE32
  if (FreeLibrary ((HMODULE) handle) == 0)
#else
  if (dlclose (handle) != 0)
#endif
    {
      g_warning (
        "An error occurred closing the VST plugin "
        "handle");
      return -1;
    }

  return 0;
}

/**
 * Loads the VST library.
 *
 * @param[out] effect The AEffect address to fill.
 * @param test Test the plugin first to make sure
 *   that it won't crash. This is useful when only
 *   extracting info from it.
 *
 * @return The library handle, or NULL if error.
 */
static void *
load_lib (
  const char *  path,
  AEffect **    effect,
  int           test)
{
#ifdef _WOE32
  HMODULE handle = LoadLibraryA (path);
#else
  void * handle = dlopen (path, RTLD_NOW);
#endif
  if (!handle)
    {
#ifdef _WOE32
      char str[2000];
      windows_errors_get_last_error_str (str);
      g_warning (
        "Failed to load VST plugin from %s: %s",
        path, str);
#else
      g_warning (
        "Failed to load VST plugin from %s: %s",
        path, dlerror ());
#endif
      return NULL;
    }

  VstPluginMain entry_point =
    (VstPluginMain)
#ifdef _WOE32
    GetProcAddress (handle, "VSTPluginMain");
#else
    dlsym (handle, "VSTPluginMain");
#endif
  if (!entry_point)
    entry_point =
      (VstPluginMain)
#ifdef _WOE32
      GetProcAddress (handle, "main");
#else
      dlsym (handle, "main");
#endif
  if (!entry_point)
    {
#ifdef _WOE32
      g_warning (
        "Failed to get entry point from VST plugin "
        "from %s",
        path);
#else
      g_warning (
        "Failed to get entry point from VST plugin "
        "from %s: %s",
        path, dlerror ());
#endif
      close_lib (handle);
      return NULL;
    }

  if (test)
    {
      char cmd[3000];
#ifdef _WOE32
#ifdef WINDOWS_RELEASE
      char * prog =
        g_find_program_in_path (
          "zrythm_vst_check.exe");
      g_return_val_if_fail (prog, NULL);
      sprintf (
        cmd, "%s \"%s\"", prog, path);
#else // if not WINDOWS_RELEASE
      sprintf (
        cmd, "zrythm_vst_check.exe \"%s\"", path);
#endif
#else // if not _WOE32
      sprintf (
        cmd, "zrythm_vst_check \"%s\"", path);
#endif
      g_message ("running cmd: %s", cmd);
      if (system_run_cmd (cmd, 10000) != 0)
        {
          g_warning (
            "%s: VST plugin failed the test", path);
          close_lib (handle);
          return NULL;
        }
    }
  *effect = entry_point (host_callback);

  if (!(*effect))
    {
      g_warning (
        "%s: Could not get entry point for "
        "VST plugin", path);
      close_lib (handle);
      return NULL;
    }

  /* check plugin's magic number */
  if ((*effect)->magic != kEffectMagic)
    {
      g_warning (
        "Plugin's magic number is invalid");
      close_lib (handle);
      return NULL;
    }

  return handle;
}

/**
 * Copies the parameter values.
 */
void
vst_plugin_copy_params (
  VstPlugin * dest,
  VstPlugin * src)
{
  g_return_if_fail (dest && src && dest->aeffect);
  AEffect * effect = dest->aeffect;

  for (int i = 0; i < effect->numParams; i++)
    {
      for (int j = 0;
           j < dest->plugin->num_in_ports; j++)
        {
          Port * port = dest->plugin->in_ports[j];
          Port * src_port =
            src->plugin->in_ports[j];
          if (port->identifier.type ==
                TYPE_CONTROL &&
              port->vst_param_id == i)
            {
              port->control = src_port->control;
              effect->getParameter (effect, i);
              effect->setParameter (
                effect, i, port->control);
            }
        }
    }
}

void
vst_plugin_init_loaded (
  VstPlugin * self)
{
  AEffect * effect = NULL;
  void * handle =
    load_lib (
      self->plugin->descr->path, &effect, 0);
  g_warn_if_fail (handle);
  self->aeffect = effect;

  /* load program if any */
  if (self->program_name &&
        effect->numPrograms > 0 &&
        self->program_index >= 0 &&
        self->program_index < effect->numPrograms)
    {
      self->aeffect->dispatcher (
        self->aeffect, effSetProgram,
        0, self->program_index, NULL, 0.f);

      char prog_name[600];
      self->aeffect->dispatcher (
        self->aeffect, effGetProgramName,
        0, 0, prog_name, 0.f);
      g_message (
        "loaded VST program %s", prog_name);
    }

  effect->user = self;

  for (int i = 0; i < effect->numParams; i++)
    {
      for (int j = 0;
           j < self->plugin->num_in_ports; j++)
        {
          Port * port = self->plugin->in_ports[j];
          if (port->identifier.type ==
                TYPE_CONTROL &&
              port->vst_param_id == i)
            {
              effect->setParameter (
                effect, i, port->control);
            }
        }
    }
}

VstPlugin *
vst_plugin_new_from_descriptor (
  Plugin *                 plugin,
  const PluginDescriptor * descr)
{
  g_return_val_if_fail (
    plugin && descr &&
    descr->protocol == PROT_VST && descr->path,
    NULL);

  AEffect * effect = NULL;
  void * handle = load_lib (descr->path, &effect, 0);
  g_return_val_if_fail (handle, NULL);

  VstPlugin * self = calloc (1, sizeof (VstPlugin));
  self->aeffect = effect;
  effect->user = self;

  plugin->vst = self;
  self->plugin = plugin;

  return self;
}

/**
 * @param test 1 if testing.
 */
PluginDescriptor *
vst_plugin_create_descriptor_from_path (
  const char * path,
  int          test)
{
  PluginDescriptor * descr =
    calloc (1, sizeof (PluginDescriptor));

  descr->path = g_strdup (path);
  descr->protocol = PROT_VST;

  AEffect * effect = NULL;
  void * handle = load_lib (path, &effect, !test);
  g_return_val_if_fail (handle, NULL);

  VstPlugin * self = calloc (1, sizeof (VstPlugin));
  self->aeffect = effect;

  char str[600];
  get_name (self, str);
  descr->name = g_strdup (str);

  get_author (self, str);
  descr->author = g_strdup (str);

  descr->num_audio_ins = effect->numInputs;
  descr->num_audio_outs = effect->numOutputs;
  descr->num_ctrl_ins = effect->numParams;

  if (can_do (effect, "receiveVstEvents") ||
      can_do (effect, "receiveVstMidiEvent") ||
      (effect->flags & effFlagsIsSynth) > 0)
    {
      descr->num_midi_ins = 1;
    }
  if (can_do (effect, "sendVstEvents") ||
      can_do (effect, "sendVstMidiEvent"))
    {
      descr->num_midi_outs = 1;
    }

  /* get category */
  if ((effect->flags & effFlagsIsSynth) > 0)
    {
      descr->category = PC_INSTRUMENT;
      descr->category_str = g_strdup ("Instrument");
    }
  else
    {
      const intptr_t category =
        effect->dispatcher (
          effect, effGetPlugCategory, 0, 0,
          NULL, 0.f);
      switch (category)
        {
        case kPlugCategAnalysis:
          descr->category = PC_ANALYZER;
          descr->category_str =
            g_strdup ("Analyzer");
          break;
        case kPlugCategGenerator:
          descr->category = PC_GENERATOR;
          descr->category_str =
            g_strdup ("Generator");
          break;
        case kPlugCategShell:
          g_warn_if_reached ();
          break;
        default:
          descr->category = PLUGIN_CATEGORY_NONE;
          descr->category_str =
            g_strdup ("Plugin");
          break;
        }
    }

  effect->dispatcher (
    effect, effClose, 0, 0, NULL, 0.f);

  /* give the plugin some time to switch off */
  g_usleep (1000);

  if (close_lib (handle))
    return NULL;

  return descr;
}

Port *
vst_plugin_get_port_from_param_id (
  VstPlugin * self,
  int         param_id)
{
  g_return_val_if_fail (self, NULL);
  for (int i = 0; i < self->plugin->num_in_ports;
       i++)
    {
      Port * port = self->plugin->in_ports[i];
      if (port->identifier.type == TYPE_CONTROL &&
          port->vst_param_id == param_id)
        return port;
    }

  g_return_val_if_reached (NULL);
}

void
vst_plugin_process (
  VstPlugin * self,
  const long  g_start_frames,
  const nframes_t  local_offset,
  const nframes_t   nframes)
{
  AEffect * effect = self->aeffect;

  /* If transport state is not as expected, then
   * something has changed */
  const int xport_changed =
    self->rolling !=
      (TRANSPORT_IS_ROLLING) ||
    self->gframes !=
      g_start_frames ||
    !math_floats_equal (self->bpm, TRANSPORT->bpm);

  /* Update transport state to expected values for
   * next cycle */
  if (TRANSPORT_IS_ROLLING)
    {
      self->gframes =
        transport_frames_add_frames (
          TRANSPORT, self->gframes,
          nframes);
      self->rolling = 1;
    }
  else
    {
      self->rolling = 0;
    }
  self->bpm = TRANSPORT->bpm;

  /* prepare time info */
  VstTimeInfo * time_info = &self->time_info;
  time_info->flags = 0;
  time_info->samplePos = (double) g_start_frames;
  time_info->sampleRate =
    (double) AUDIO_ENGINE->sample_rate;
  time_info->tempo = (double) TRANSPORT->bpm;
  time_info->ppqPos =
    time_info->samplePos /
    (time_info->sampleRate * 60.0 /
       time_info->tempo);
  time_info->barStartPos =
    (double) TRANSPORT->beats_per_bar *
    (double) (PLAYHEAD->bars - 1);
  time_info->timeSigNumerator =
    (int32_t) TRANSPORT->beats_per_bar;
  time_info->timeSigDenominator =
    (int32_t) TRANSPORT->beat_unit;
  time_info->flags |= kVstPpqPosValid;
  time_info->flags |= kVstTempoValid;
  time_info->flags |= kVstBarsValid;
  time_info->flags |= kVstTimeSigValid;
  if (xport_changed)
    time_info->flags |= kVstTransportChanged;
  if (TRANSPORT_IS_ROLLING)
    time_info->flags |= kVstTransportPlaying;

  /* prepare audio bufs */
  float * inputs[effect->numInputs];
  float * outputs[effect->numOutputs];
  for (int i = 0; i < effect->numInputs; i++)
    {
      inputs[i] =
        &self->plugin->in_ports[i]->
          buf[local_offset];
    }
  for (int i = 0; i < effect->numOutputs; i++)
    {
      outputs[i] =
        &self->plugin->out_ports[i]->
          buf[local_offset];
    }

  /* process midi */
  if (self->plugin->descr->num_midi_ins > 0)
    {
      Port * port =
        self->plugin->in_ports[effect->numInputs];

      if (port->midi_events->num_events > 0)
        {
          self->fEvents.numEvents =
            port->midi_events->num_events;
          self->fEvents.reserved = 0;
          VstMidiEvent * e;
          for (int i = 0;
               i < port->midi_events->num_events;
               ++i)
            {
              MidiEvent * ev =
                &port->midi_events->events[i];
              e = &self->fMidiEvents[i];
              g_message (
                "writing VST plugin event %d",
                i);
              e->type = kVstMidiType;
              e->byteSize = sizeof (VstMidiEvent);
              /*e.flags = kVstMidiEventIsRealtime;*/
              e->midiData[0] =
                (char) ev->raw_buffer[0];
              e->midiData[1] =
                (char) ev->raw_buffer[1];
              e->midiData[2] =
                (char) ev->raw_buffer[2];
            }
          effect->dispatcher (
            effect, effProcessEvents, 0, 0,
            &self->fEvents, 0.f);
        }
    }

  effect->processReplacing (
    effect, inputs, outputs, (int32_t) nframes);
}

int
vst_plugin_instantiate (
  VstPlugin * self,
  int         loading)
{
  AEffect * effect = self->aeffect;

  effect->dispatcher (
    effect, effIdentify, 0, 0, NULL, 0.0f);
  effect->dispatcher (
    effect, effSetProcessPrecision, 0,
    kVstProcessPrecision32, NULL, 0.0f);

  /* initialize midi events */
  for (int i = 0; i < kPluginMaxMidiEvents * 2; i++)
    {
      self->fEvents.data[i] =
        (VstEvent *) &self->fMidiEvents[i];
    }

  g_return_val_if_fail (
    effect->uniqueID > 0, -1);

  g_message ("opening VST plugin");
  effect->dispatcher (
    effect, effOpen, 0, 0, NULL, 0.0f);
  effect->dispatcher (
    effect, effMainsChanged, 0, 1, NULL, 0.0f);

  float sampleRate = AUDIO_ENGINE->sample_rate;
  effect->dispatcher (
    effect, effSetSampleRate, 0, 0, NULL,
    sampleRate);
  int blocksize = (int) AUDIO_ENGINE->block_length;
  effect->dispatcher (
    effect, effSetBlockSize, 0, blocksize, NULL,
    0.0f);
  g_message (
    "set VST sample rate to %f and block size "
    "to %d", (double) sampleRate, blocksize);

  /* create and connect ports */
  char lbl[400];
  for (int i = 0; i < effect->numInputs; i++)
    {
      if (!loading)
        {
          sprintf (lbl, "Audio Input %d", i);
          Port * port =
            port_new_with_type (
              TYPE_AUDIO, FLOW_INPUT, lbl);
          port->tmp_plugin = self->plugin;
          plugin_add_in_port (
            self->plugin, port);
        }
      effect->dispatcher (
        effect, effConnectInput, i, 1, NULL, 0.f);
    }
  for (int i = 0; i < effect->numOutputs; i++)
    {
      if (!loading)
        {
          sprintf (lbl, "Audio Output %d", i);
          Port * port =
            port_new_with_type (
              TYPE_AUDIO, FLOW_OUTPUT, lbl);
          port->tmp_plugin = self->plugin;
          plugin_add_out_port (
            self->plugin, port);
        }
      effect->dispatcher (
        effect, effConnectOutput, i, 1, NULL, 0.f);
    }
  if (self->plugin->descr->num_midi_ins > 0)
    {
      if (!loading)
        {
          strcpy (lbl, "MIDI Input");
          Port * port =
            port_new_with_type (
              TYPE_EVENT, FLOW_INPUT, lbl);
          port->tmp_plugin = self->plugin;
          plugin_add_in_port (
            self->plugin, port);
        }
    }
  if (self->plugin->descr->num_midi_outs > 0)
    {
      if (!loading)
        {
          strcpy (lbl, "MIDI Output");
          Port * port =
            port_new_with_type (
              TYPE_EVENT, FLOW_OUTPUT, lbl);
          port->tmp_plugin = self->plugin;
          plugin_add_out_port (
            self->plugin, port);
        }
    }
  for (int i = 0; i < effect->numParams; i++)
    {
      if (!loading)
        {
          get_param_name (self, i, lbl);
          Port * port =
            port_new_with_type (
              TYPE_CONTROL, FLOW_INPUT, lbl);
          port->vst_param_id = i;
          port->tmp_plugin = self->plugin;
          plugin_add_in_port (
            self->plugin, port);
          port->control =
            effect->getParameter (effect, i);
        }
    }

  g_message (
    "start process %s", self->plugin->descr->name);
  effect->dispatcher (
    self->aeffect, effStartProcess, 0, 0,
    NULL, 0.f);

  /* load the parameter values. this must be done
   * after starting processing */
  if (loading)
    {
      for (int i = 0; i < effect->numParams; i++)
        {
          Port * port =
            vst_plugin_get_port_from_param_id (
              self, i);
          effect->setParameter (
            effect, i, port->control);
        }
    }

  return 0;
}

void
vst_plugin_suspend (
  VstPlugin * self)
{
  AEffect * effect = self->aeffect;
  effect->dispatcher (
    effect, effMainsChanged, 0, 0, NULL, 0.0f);
}

static void
on_realize (
  GtkWidget * widget,
  VstPlugin * self)
{
#ifdef HAVE_X11
  gtk_socket_add_id (
    self->socket, (Window) self->xid);
  gtk_widget_set_size_request (
    GTK_WIDGET (self->socket),
    self->width, self->height);
#endif
  gtk_window_set_default_size (
    GTK_WINDOW (widget),
    self->width, self->height);

  self->plugin->ui_instantiated = 1;
  self->plugin->visible = 1;
  EVENTS_PUSH (
    ET_PLUGIN_VISIBILITY_CHANGED,
    self->plugin);
}

void
vst_plugin_close_ui (
  VstPlugin * self)
{
#ifdef HAVE_X11
  vst_x11_destroy_editor (self);
#endif

  if (self->plugin->visible)
    {
      plugin_gtk_close_window (self->plugin);
    }
}

/**
 * This actually opens the UI later, when the
 * newly created window gets realized.
 */
void
vst_plugin_open_ui (
  VstPlugin * self)
{
  plugin_gtk_create_window (self->plugin);

#ifdef _WOE32
  vst_windows_run_editor (
    self, GTK_WIDGET (self->plugin->window));
#elif HAVE_X11
  vst_x11_run_editor (self);
#endif

  if (g_settings_get_int (
        S_PREFERENCES, "plugin-uis-stay-on-top"))
    {
#ifdef _WOE32
      gtk_window_set_transient_for (
        GTK_WINDOW (self->plugin->window),
        GTK_WINDOW (
          self->plugin->black_window));
      gtk_window_set_transient_for (
        GTK_WINDOW (
          self->plugin->black_window),
        GTK_WINDOW (MAIN_WINDOW));
#else
      gtk_window_set_transient_for (
        GTK_WINDOW (self->plugin->window),
        GTK_WINDOW (MAIN_WINDOW));
#endif
    }

  /*self->gtk_window_parent = window;*/
  g_signal_connect (
    self->plugin->window, "realize",
    G_CALLBACK (on_realize), self);

  if (plugin_has_supported_custom_ui (self->plugin))
    {
#ifdef HAVE_X11
      self->socket =
        GTK_SOCKET (gtk_socket_new ());
      gtk_container_add (
        GTK_CONTAINER (self->plugin->ev_box),
        GTK_WIDGET (self->socket));
      gtk_widget_set_visible (
        GTK_WIDGET (self->socket), 1);
#endif
    }
  else
    {
      /* TODO create generic */
    }

  gtk_window_set_resizable (
    GTK_WINDOW (self->plugin->window), 1);
  gtk_window_present (
    GTK_WINDOW (self->plugin->window));
  gtk_widget_show_all (
    GTK_WIDGET (self->plugin->window));
}

/**
 * Returns a base64-encoded state.
 *
 * Must be free'd by caller.
 *
 * @param single 1 for single program, 0 for all
 *   programs.
 */
static char *
get_base64_chunk (
  VstPlugin * self,
  int         single)
{
  guchar * data;
  int32_t data_size =
    self->aeffect->dispatcher (
      self->aeffect, effGetChunk,
      single ? 1 : 0, 0, &data, 0);
  if (data_size == 0)
    return NULL;

  return g_base64_encode (data, (gsize) data_size);
}

/**
 * Loads the state (chunk) from \ref
 * VstPlugin.state_file.
 */
int
vst_plugin_load_state_from_file (
  VstPlugin * self)
{
  if (!self->state_file)
    return 0;

  char * chunk;
  GError *err = NULL;
  g_file_get_contents (
    self->state_file, &chunk, NULL, &err);
  gsize size = 0;
  guchar * raw_data =
    g_base64_decode (chunk, &size);
  int single = 0;
  int r =
    self->aeffect->dispatcher (
      self->aeffect, effSetChunk, single ? 1 : 0,
      (intptr_t) size, raw_data, 0);
  g_free (chunk);
  g_free (raw_data);

  return r;
}

/**
 * Saves the current state in given dir.
 *
 * Used when saving the project.
 */
int
vst_plugin_save_state_to_file (
  VstPlugin *  self,
  const char * dir)
{
  /* save the program */
  if (self->aeffect->numPrograms > 0)
    {
      char prog_name[600];
      self->aeffect->dispatcher (
        self->aeffect, effGetProgramName,
        0, 0, prog_name, 0.f);
      if (g_str_is_ascii (prog_name))
        {
          self->program_name = g_strdup (prog_name);
          self->program_index =
            (int)
            self->aeffect->dispatcher (
              self->aeffect, effGetProgram, 0, 0,
              NULL, 0.f);
          g_message (
            "VST prog name is %s",
            self->program_name);
        }
      else
        {
          g_message (
            "skipping non-ascii VST prog name %s",
            prog_name);
        }
    }

  /* save the parameters just in case because we
   * get no notification when the program changes */
  for (int i = 0; i < self->aeffect->numParams;
       i++)
    {
      Port * port =
        vst_plugin_get_port_from_param_id (
          self, i);
      g_return_val_if_fail (port, -1);
      port->control =
        self->aeffect->getParameter (
          self->aeffect, i);
    }

  if (!(self->aeffect->flags &
          effFlagsProgramChunks))
    return 0;

  char * chunk = get_base64_chunk (self, 0);
  g_return_val_if_fail (chunk, -1);

  io_mkdir (dir);

  char * label =
    g_strdup_printf (
      "%s.base64",
      self->plugin->descr->name);
  char * tmp = g_path_get_basename (dir);
  self->state_file =
    g_build_filename (tmp, label, NULL);
  char * state_file_path =
    g_build_filename (dir, label, NULL);
  GError *err = NULL;
  g_file_set_contents (
    state_file_path, chunk, -1, &err);
  if (err)
    {
      // Report error to user, and free error
      g_critical (
        "Unable to write VST state: %s",
        err->message);
      g_error_free (err);
      return -1;
    }
  g_free (tmp);
  g_free (label);
  g_free (chunk);

  return 0;
}
