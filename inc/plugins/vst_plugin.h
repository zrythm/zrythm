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
 * Copyright (C) 2010 Paul Davis
 * Copyright (C) 2011-2019 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/**
 * \file
 *
 * VST plugin.
 */

#ifndef __PLUGINS_VST_PLUGIN_H__
#define __PLUGINS_VST_PLUGIN_H__

#include "config.h"

#include <pthread.h>

#include "audio/port.h"
#include "plugins/vst/vestige.h"
#include "utils/types.h"

#ifdef HAVE_X11
#include <gtk/gtkx.h>
#endif

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

typedef struct PluginDescriptor PluginDescriptor;

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

#define kPluginMaxMidiEvents 512

/**
 * Modified VstEvents taken from Carla.
 */
typedef struct FixedVstEvents
{
  int32_t numEvents;
  intptr_t reserved;
  VstEvent * data[kPluginMaxMidiEvents * 2];
} FixedVstEvents;

/**
 * VST plugin.
 */
typedef struct VstPlugin
{
  /** Vst plugin handle. */
  AEffect *          aeffect;

  /** Base Plugin instance (parent). */
  Plugin *           plugin;

#ifdef HAVE_X11
  /** Socket to house the plugin's X11 window in
   * \ref VstPlugin.xid. */
  GtkSocket *        socket;
#endif

  /** This is the plugin state in base64 encoding. */
  //char *             chunk;

  /** For saving/loading the state. */
  char *             state_file;

  /** Taken from Carla. */
  FixedVstEvents     fEvents;

  VstMidiEvent       fMidiEvents[kPluginMaxMidiEvents*2];

  /** This should be updated during processing and
   * sent to the plugin during sendVstTimeInfo. */
  VstTimeInfo        time_info;

  /** Last known global frames */
  long               gframes;

  /** Last known rolling status. */
  int                rolling;

  /** Program index, if program name exists. */
  int                program_index;

  /** Program name, to be used when loading. */
  char *             program_name;

  /** Last known BPM. */
  bpm_t              bpm;

  /**
   * Window (if applicable) (GtkWindow).
   *
   * This is used by both generic UIs and X11/etc
   * UIs.
   */
  void *             gtk_window_parent;

  /** X11 XWindow (winw + lxvst). */
  int                xid;

  /* The plugin's parent X11 XWindow (LXVST/X11). */
  int                linux_window;

  /** The ID of the plugin UI window created by
   * the plugin. */
  int                linux_plugin_ui_window;

  /** X11 UI _XEventProc. */
  void  (* eventProc) (void * event);

  /* Windows. */
  void *             windows_window;

  int width;
  int height;
  int wantIdle;

  int voffset;
  int hoffset;
  int gui_shown;
  int destroy;
  int vst_version;
  int has_editor;

  int     program_set_without_editor;
  int     want_program;
  int     want_chunk;
  int     n_pending_keys;
  unsigned char* wanted_chunk;
  int     wanted_chunk_size;
  float*  want_params;
  float*  set_params;

  int     dispatcher_wantcall;
  int     dispatcher_opcode;
  int     dispatcher_index;
  int     dispatcher_val;
  void*   dispatcher_ptr;
  float   dispatcher_opt;
  int     dispatcher_retval;

#ifdef HAVE_X11
  pthread_mutex_t   state_lock;
  pthread_cond_t    window_status_change;
  pthread_cond_t    plugin_dispatcher_called;
  pthread_cond_t    window_created;
#endif

  struct VstPlugin * next;
  pthread_mutex_t   lock;
  int               been_activated;

  /** ID of the delete-event signal so that we can
   * deactivate before freeing the plugin. */
  gulong             delete_event_id;
} VstPlugin;

static const cyaml_schema_field_t
  vst_plugin_fields_schema[] =
{
  CYAML_FIELD_STRING_PTR (
    "state_file",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    VstPlugin, state_file,
    0, CYAML_UNLIMITED),
  CYAML_FIELD_INT (
    "program_index", CYAML_FLAG_DEFAULT,
    VstPlugin, program_index),
  CYAML_FIELD_STRING_PTR (
    "program_name",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    VstPlugin, program_name,
    0, CYAML_UNLIMITED),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  vst_plugin_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER, VstPlugin,
    vst_plugin_fields_schema),
};

void
vst_plugin_init_loaded (
  VstPlugin * self);

/**
 * @param test 1 if testing.
 */
PluginDescriptor *
vst_plugin_create_descriptor_from_path (
  const char * path,
  int          test);

VstPlugin *
vst_plugin_new_from_descriptor (
  Plugin *                 plugin,
  const PluginDescriptor * descr);

/**
 * Copies the parameter values.
 */
void
vst_plugin_copy_params (
  VstPlugin * dest,
  VstPlugin * src);

int
vst_plugin_instantiate (
  VstPlugin * self,
  int         loading);

void
vst_plugin_suspend (
  VstPlugin * self);

void
vst_plugin_process (
  VstPlugin * self,
  const long  g_start_frames,
  const nframes_t  local_offset,
  const nframes_t   nframes);

Port *
vst_plugin_get_port_from_param_id (
  VstPlugin * self,
  int         param_id);

void
vst_plugin_open_ui (
  VstPlugin * self);

void
vst_plugin_close_ui (
  VstPlugin * self);

/**
 * Saves the current state in given dir.
 *
 * Used when saving the project.
 */
int
vst_plugin_save_state_to_file (
  VstPlugin *  self,
  const char * dir);

/**
 * Loads the state (chunk) from \ref
 * VstPlugin.state_file.
 *
 * TODO not used at the moment.
 */
int
vst_plugin_load_state_from_file (
  VstPlugin * self);

/**
 * @}
 */

#endif
