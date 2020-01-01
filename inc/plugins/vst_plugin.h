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

#define audioMasterGetOutputSpeakerArrangement audioMasterGetSpeakerArrangement
#define effFlagsProgramChunks (1 << 5)
#define effSetProgramName 4
#define effGetParamLabel 6
#define effGetParamDisplay 7
#define effGetVu 9
#define effEditDraw 16
#define effEditMouse 17
#define effEditKey 18
#define effEditSleep 21
#define effIdentify 22
#define effGetChunk 23
#define effSetChunk 24
#define effCanBeAutomated 26
#define effString2Parameter 27
#define effGetNumProgramCategories 28
#define effGetProgramNameIndexed 29
#define effCopyProgram 30
#define effConnectInput 31
#define effConnectOutput 32
#define effGetInputProperties 33
#define effGetOutputProperties 34
#define effGetCurrentPosition 36
#define effGetDestinationBuffer 37
#define effOfflineNotify 38
#define effOfflinePrepare 39
#define effOfflineRun 40
#define effProcessVarIo 41
#define effSetSpeakerArrangement 42
#define effSetBlockSizeAndSampleRate 43
#define effSetBypass 44
#define effGetErrorText 46
#define effVendorSpecific 50
#define effGetTailSize 52
#define effGetIcon 54
#define effSetViewPosition 55
#define effKeysRequired 57
#define effEditKeyDown 59
#define effEditKeyUp 60
#define effSetEditKnobMode 61
#define effGetMidiProgramName 62
#define effGetCurrentMidiProgram 63
#define effGetMidiProgramCategory 64
#define effHasMidiProgramsChanged 65
#define effGetMidiKeyName 66
#define effGetSpeakerArrangement 69
#define effSetTotalSampleToProcess 73
#define effSetPanLaw 74
#define effBeginLoadBank 75
#define effBeginLoadProgram 76
#define effSetProcessPrecision 77
#define effGetNumMidiInputChannels 78
#define effGetNumMidiOutputChannels 79
#define kVstAutomationOff 1
#define kVstAutomationReadWrite 4
#define kVstProcessLevelUnknown 0
#define kVstProcessLevelUser 1
#define kVstProcessLevelRealtime 2
#define kVstProcessLevelOffline 4
#define kVstProcessPrecision32 0
#define kVstVersion 2400

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
vst_plugin_new_from_descriptor (
  Plugin *                 plugin,
  const PluginDescriptor * descr);

int
vst_plugin_instantiate (
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
