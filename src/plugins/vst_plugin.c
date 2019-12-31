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
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "plugins/vst_plugin.h"
#include "project.h"

#include <gdk/gdkx.h>

#include <dlfcn.h>
#include <X11/Xlib.h>

void
vst_plugin_new_from_path (
  Plugin *     plugin,
  const char * path)
{
  void * handle = dlopen (path, RTLD_NOW);
  if (!handle)
    {
      g_warning (
        "Failed to load VST plugin from %s: %s",
        path, dlerror ());
      return;
    }

  VstPluginMain entry_point =
    (VstPluginMain) dlsym (handle, "VSTPluginMain");
  if (!entry_point)
    entry_point =
      (VstPluginMain) dlsym (handle, "main");
  if (!entry_point)
    {
      g_warning (
        "Failed to get entry point from VST plugin from %s: %s",
        path, dlerror ());
      return;
    }

  VstPlugin * self = calloc (1, sizeof (VstPlugin));
  AEffect * effect =
    entry_point (vst_plugin_host_callback);
  self->aeffect = effect;

  /* check plugin's magic number */
  if (effect->magic != kEffectMagic)
    {
      g_warning ("Plugin's magic number is invalid");
      return;
    }

  plugin->vst = self;
  self->plugin = plugin;

  vst_plugin_start (self);

  for (int i = 0; i < effect->numParams; i++)
    {
      g_message ("effect %d is %f", i, (double)
      effect->getParameter (effect, i));
    }
}

/**
 * Host callback implementation.
 */
intptr_t
vst_plugin_host_callback (
  AEffect * effect,
  int32_t   opcode,
  int32_t   index,
  intptr_t  value,
  void *    ptr,
  float     opt)
{
  g_message ("host callback");

  switch (opcode)
    {
    case audioMasterVersion:
      g_message ("master version");
      return 2400;
    case audioMasterIdle:
      g_message ("idle");
      effect->dispatcher (
        effect, effEditIdle, 0, 0, 0, 0);
      break;
    // Handle other opcodes here... there will be lots of them
    default:
      g_message ("Plugin requested value of opcode %d", opcode);
      break;
  }
  return 0;
}

void
vst_plugin_process (
  VstPlugin * self,
  const long  g_start_frames,
  const nframes_t   nframes)
{
  int channels = 2;
  float ** inputs =
    (float**)malloc(sizeof(float**) * (size_t) channels);
  float ** outputs =
    (float**)malloc(sizeof(float**) * (size_t) channels);
  for(int i = 0; i < channels; i++)
    {
      inputs[i] =
        (float*)malloc(sizeof(float*) *
          (size_t) AUDIO_ENGINE->block_length);
      outputs[i] =
        (float*)malloc(sizeof(float*) *
          (size_t) AUDIO_ENGINE->block_length);
    }

  /* process midi */
  void * events = NULL;
  self->aeffect->dispatcher (
    self->aeffect, effProcessEvents, 0, 0, events, 0.f);

  self->aeffect->processReplacing (
    self->aeffect, inputs, outputs, (int) nframes);
}

void
vst_plugin_start (
  VstPlugin * self)
{
  AEffect * effect = self->aeffect;

  g_message ("effOpen");
  effect->dispatcher (
    effect, effOpen, 0, 0, NULL, 0.0f);

  g_message ("setting VST sample rate and block size");
  float sampleRate = AUDIO_ENGINE->sample_rate;
  effect->dispatcher (
    effect, effSetSampleRate, 0, 0, NULL, sampleRate);
  int blocksize = (int) AUDIO_ENGINE->block_length;
  effect->dispatcher (
    effect, effSetBlockSize, 0, blocksize, NULL, 0.0f);

  g_message ("resume");
  vst_plugin_resume (self);

  g_message ("start process");
  effect->dispatcher (
    self->aeffect, effStartProcess, 0, 0, NULL, 0.f);

  g_message ("open ui");
  vst_plugin_open_ui (self);
}

void
vst_plugin_resume (
  VstPlugin * self)
{
  AEffect * effect = self->aeffect;
  effect->dispatcher (
    effect, effMainsChanged, 0, 1, NULL, 0.0f);
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
  /* Attempt to instantiate custom UI if
   * necessary */
  Window xid = 0;
  Display * display =
    gdk_x11_display_get_xdisplay (
      gtk_widget_get_display (
        widget));
  self->has_ui = 1;
  if (self->has_ui)
    {
      g_message ("Instantiating UI...");
      XSetWindowAttributes attr;
      attr.border_pixel = 0;
      attr.event_mask   = KeyPressMask|KeyReleaseMask;
      xid =
        XCreateWindow (
          /* display */
          display,
          /* parent */
          gdk_x11_window_get_xid (
            gtk_widget_get_window (
              GTK_WIDGET (widget))),
          /* x, y, width, height */
          0, 0, 600, 600, 0,
          CopyFromParent, InputOutput,
          CopyFromParent,
          CWBorderPixel | CWEventMask, &attr);

      /*xid =*/
        /*gdk_x11_window_get_xid (*/
          /*gtk_widget_get_window (*/
            /*GTK_WIDGET (window)));*/
    }

  AEffect * effect = self->aeffect;
  if (effect->dispatcher (
        effect, effEditOpen, 0, 0,
        (void *) xid, 0.f) != 0 || true)
    {

      XMapRaised (display, xid);
    }
  else
    {
      g_message ("No UI found, building native..");
      /* TODO */
    }

  self->plugin->ui_instantiated = 1;

  /*g_message (*/
    /*"plugin window shown, adding idle timeout. "*/
    /*"Update frequency (Hz): %.01f",*/
    /*(double) self->ui_update_hz);*/

#if 0
  g_timeout_add (
    (int) (1000.f / 60.f),
    (GSourceFunc) update_plugin_ui, plugin);
#endif
}

/**
 * This actually opens the UI later, when the
 * newly created window gets realized.
 */
void
vst_plugin_open_ui (
  VstPlugin * self)
{
  GtkWidget* window =
    gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_icon_name (
    GTK_WINDOW (window), "zrythm");

  if (g_settings_get_int (
        S_PREFERENCES, "plugin-uis-stay-on-top"))
    gtk_window_set_transient_for (
      GTK_WINDOW (window),
      GTK_WINDOW (MAIN_WINDOW));

  self->window = window;
  /*self->delete_event_id =*/
    /*g_signal_connect (*/
      /*G_OBJECT (window), "delete-event",*/
      /*G_CALLBACK (on_delete_event), self);*/

  /* TODO get name */

  /* connect destroy signal */
  /*g_signal_connect (*/
    /*window, "destroy",*/
    /*G_CALLBACK (on_window_destroy), self);*/

  g_signal_connect (
    window, "realize",
    G_CALLBACK (on_realize), self);

  /*set_window_title (self);*/

  GtkWidget* vbox =
    gtk_box_new (
      GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_window_set_role (
    GTK_WINDOW (window), "plugin_ui");
  gtk_container_add (
    GTK_CONTAINER (window), vbox);

  gtk_window_set_resizable (
    GTK_WINDOW (window), 1);
  gtk_window_present(GTK_WINDOW(window));
  gtk_widget_show_all (GTK_WIDGET (window));

  /*build_menu (self, window, vbox);*/

  /* Create/show alignment to contain UI (whether
   * custom or generic) */
  /*GtkWidget* alignment =*/
    /*gtk_alignment_new (0.5, 0.5, 1.0, 1.0);*/
  /*gtk_box_pack_start (*/
    /*GTK_BOX (vbox), alignment, TRUE, TRUE, 0);*/
  /*gtk_widget_show (alignment);*/
}
