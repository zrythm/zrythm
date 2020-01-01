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

/**
 * \file
 *
 * VST plugin.
 */

#ifndef __PLUGINS_VST_PLUGIN_H__
#define __PLUGINS_VST_PLUGIN_H__

#include "config.h"

#include "audio/port.h"
#include "plugins/vst/vestige.h"
#include "utils/types.h"

/**
 * @addtogroup plugins
 *
 * @{
 */

/** Plugin's entry point. */
typedef AEffect *(*VstPluginMain)(audioMasterCallback host);

typedef struct ERect
{
  int16_t top, left, bottom, right;
} ERect;

typedef struct VstPlugin
{
  /** Vst plugin handle. */
  AEffect *          aeffect;

  /** Base Plugin instance (parent). */
  Plugin *           plugin;

  int                has_ui;

  /**
   * Window (if applicable) (GtkWindow).
   *
   * This is used by both generic UIs and X11/etc
   * UIs.
   */
  void *             window;

  /** ID of the delete-event signal so that we can
   * deactivate before freeing the plugin. */
  gulong             delete_event_id;
} VstPlugin;

PluginDescriptor *
vst_plugin_create_descriptor_from_path (
  const char * path);

void
vst_plugin_new_from_path (
  Plugin *     plugin,
  const char * path);

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
  float     opt);

void
vst_plugin_start (
  VstPlugin * self);

void
vst_plugin_resume (
  VstPlugin * self);

void
vst_plugin_suspend (
  VstPlugin * self);

void
vst_plugin_process (
  VstPlugin * self,
  const long  g_start_frames,
  const nframes_t   nframes);

void
vst_plugin_open_ui (
  VstPlugin * self);

/**
 * @}
 */

#endif
