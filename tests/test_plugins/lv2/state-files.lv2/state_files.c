// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/* A plugin whose state lives in files, exercising the ways the LV2
 * state spec lets plugins create them: a runtime file in a
 * subdirectory created through the instantiate-time state:makePath
 * feature (kept in sync with the gain port by run()), a save-time
 * file created through the save-scoped makePath, and a file created
 * during restore() through the restore-scoped makePath (regenerated
 * on every restore, like a plugin rebuilding a cache). The paths are
 * stored through state:mapPath abstract_path() as the spec requires,
 * so the host must copy the files into saved state. restore() reads
 * the bytes back and run() emits their sum as a DC offset. */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lv2/atom/atom.h"
#include "lv2/core/lv2.h"
#include "lv2/state/state.h"
#include "lv2/urid/urid.h"

#define STATE_FILES_URI "https://www.zrythm.org/plugins/state-files"

/* Nested on purpose: state files created inside subdirectories must
 * survive a round trip too */
#define RUNTIME_FILE_NAME "sub/runtime.bin"
#define MARKER_FILE_NAME "marker.bin"
#define RESTORED_FILE_NAME "restored.bin"

typedef enum
{
  IN = 0,
  OUT = 1,
  GAIN = 2
} PortIndex;

typedef struct
{
  const float * in;
  float *       out;
  const float * gain;

  LV2_URID_Map * map;
  LV2_URID       atom_Path;
  LV2_URID       runtime_file;
  LV2_URID       marker_file;
  LV2_URID       restored_file;

  /* Absolute path of the runtime file (under the host-provided
   * makePath root), owned by this instance */
  char * runtime_path;

  /* Absolute path of the file created by the latest restore()
   * (under the restore-time makePath root), owned by this
   * instance */
  char * restored_path;

  /* Byte last written to the runtime file, so run() only rewrites it
   * when the gain actually changed */
  uint8_t runtime_file_byte;

  /* Bytes read back by restore(); emitted as a DC offset. Both are 0
   * until a state containing the files is restored. */
  float marker;
  float runtime;
} StateFiles;

static uint8_t
gain_byte (float gain)
{
  return (uint8_t) (gain * 255.0f + 0.5f);
}

static void
free_path (const LV2_Feature * const * features, char * path)
{
  if (path == NULL)
    return;

  for (const LV2_Feature * const * f = features; *f != NULL; ++f)
    {
      if (strcmp ((*f)->URI, LV2_STATE__freePath) == 0)
        {
          const LV2_State_Free_Path * free_path_feature =
            (const LV2_State_Free_Path *) (*f)->data;
          if (free_path_feature != NULL)
            {
              free_path_feature->free_path (free_path_feature->handle, path);
              return;
            }
        }
    }
  free (path);
}

/* Writes the gain-derived byte to @p path (an absolute file path) */
static int
write_marker_file (const char * path, uint8_t byte)
{
  FILE * file = fopen (path, "wb");
  if (file == NULL)
    return -1;
  const int status = fwrite (&byte, 1, 1, file) == 1 ? 0 : -1;
  fclose (file);
  return status;
}

/* Reads a single byte from @p path, or 0 when it cannot be read */
static uint8_t
read_marker_file (const char * path)
{
  uint8_t byte = 0;
  FILE * file = fopen (path, "rb");
  if (file != NULL)
    {
      if (fread (&byte, 1, 1, file) != 1)
        byte = 0;
      fclose (file);
    }
  return byte;
}

/* Resolves a stored (possibly relative) path against the state
 * directory through mapPath when the host provides it */
static char *
resolved_path (
  const LV2_Feature * const * features,
  const char *                path)
{
  for (const LV2_Feature * const * f = features; *f != NULL; ++f)
    {
      if (strcmp ((*f)->URI, LV2_STATE__mapPath) == 0)
        {
          const LV2_State_Map_Path * map_path =
            (const LV2_State_Map_Path *) (*f)->data;
          if (map_path != NULL)
            return map_path->absolute_path (map_path->handle, path);
        }
    }
  return NULL;
}

static LV2_Handle
instantiate (
  const LV2_Descriptor *      descriptor,
  double                      rate,
  const char *                bundle_path,
  const LV2_Feature * const * features)
{
  (void) descriptor;
  (void) rate;
  (void) bundle_path;

  StateFiles * self = (StateFiles *) calloc (1, sizeof (StateFiles));
  if (self == NULL)
    return NULL;

  const LV2_State_Make_Path * make_path = NULL;
  for (const LV2_Feature * const * f = features; *f != NULL; ++f)
    {
      if (strcmp ((*f)->URI, LV2_URID__map) == 0)
        {
          self->map = (LV2_URID_Map *) (*f)->data;
        }
      else if (strcmp ((*f)->URI, LV2_STATE__makePath) == 0)
        {
          make_path = (const LV2_State_Make_Path *) (*f)->data;
        }
    }
  if (self->map == NULL || make_path == NULL)
    {
      free (self);
      return NULL;
    }

  self->atom_Path = self->map->map (self->map->handle, LV2_ATOM__Path);
  self->runtime_file =
    self->map->map (self->map->handle, STATE_FILES_URI "#runtimeFile");
  self->marker_file =
    self->map->map (self->map->handle, STATE_FILES_URI "#markerFile");
  self->restored_file =
    self->map->map (self->map->handle, STATE_FILES_URI "#restoredFile");

  /* Create the runtime file through the instantiate-time makePath */
  char * const runtime_path =
    make_path->path (make_path->handle, RUNTIME_FILE_NAME);
  if (runtime_path == NULL)
    {
      free (self);
      return NULL;
    }
  self->runtime_path = strdup (runtime_path);
  free_path (features, runtime_path);
  if (self->runtime_path == NULL || write_marker_file (self->runtime_path, 0))
    {
      free (self->runtime_path);
      free (self);
      return NULL;
    }

  return (LV2_Handle) self;
}

static void
connect_port (LV2_Handle instance, uint32_t port, void * data)
{
  StateFiles * self = (StateFiles *) instance;

  switch ((PortIndex) port)
    {
    case IN:
      self->in = (const float *) data;
      break;
    case OUT:
      self->out = (float *) data;
      break;
    case GAIN:
      self->gain = (const float *) data;
      break;
    }
}

static void
activate (LV2_Handle instance)
{
}

static void
run (LV2_Handle instance, uint32_t n_samples)
{
  StateFiles * self = (StateFiles *) instance;

  /* Keep the runtime file in sync with the gain port; it only holds
   * the current gain byte, so rewriting it on change is enough */
  if (self->gain != NULL)
    {
      const uint8_t byte = gain_byte (*self->gain);
      if (byte != self->runtime_file_byte)
        {
          if (write_marker_file (self->runtime_path, byte) == 0)
            self->runtime_file_byte = byte;
        }
    }

  if (self->out == NULL)
    return;

  const float dc = self->marker + self->runtime;
  for (uint32_t i = 0; i < n_samples; ++i)
    self->out[i] = dc;
}

static void
deactivate (LV2_Handle instance)
{
}

static void
cleanup (LV2_Handle instance)
{
  StateFiles * self = (StateFiles *) instance;
  free (self->runtime_path);
  free (self->restored_path);
  free (self);
}

/* Stores the abstract form of @p path (an absolute file path) under
 * @p key with type @p path_type, mapping it through mapPath as the
 * state spec requires */
static LV2_State_Status
store_path (
  const LV2_Feature * const * features,
  LV2_State_Store_Function    store,
  LV2_State_Handle            handle,
  uint32_t                    key,
  uint32_t                    path_type,
  const char *                path)
{
  char * abstract = NULL;
  for (const LV2_Feature * const * f = features; *f != NULL; ++f)
    {
      if (strcmp ((*f)->URI, LV2_STATE__mapPath) == 0)
        {
          const LV2_State_Map_Path * map_path =
            (const LV2_State_Map_Path *) (*f)->data;
          if (map_path != NULL)
            {
              abstract = map_path->abstract_path (map_path->handle, path);
              break;
            }
        }
    }

  const char * const stored = abstract != NULL ? abstract : path;
  /* Paths are plain old data but never portable (they are
   * filesystem-specific strings) */
  const LV2_State_Status status =
    store (handle, key, stored, strlen (stored) + 1, path_type,
           LV2_STATE_IS_POD);
  free_path (features, abstract);
  return status;
}

static LV2_State_Status
save (
  LV2_Handle                 instance,
  LV2_State_Store_Function   store,
  LV2_State_Handle           handle,
  uint32_t                   flags,
  const LV2_Feature * const *features)
{
  (void) flags;

  StateFiles * self = (StateFiles *) instance;

  const LV2_State_Make_Path * make_path = NULL;
  for (const LV2_Feature * const * f = features; *f != NULL; ++f)
    {
      if (strcmp ((*f)->URI, LV2_STATE__makePath) == 0)
        {
          make_path = (const LV2_State_Make_Path *) (*f)->data;
        }
    }
  if (make_path == NULL)
    return LV2_STATE_ERR_NO_FEATURE;

  char * const path = make_path->path (make_path->handle, MARKER_FILE_NAME);
  if (path == NULL)
    return LV2_STATE_ERR_UNKNOWN;

  const float gain = (self->gain != NULL) ? *self->gain : 0.0f;
  const int   write_status =
    write_marker_file (path, gain_byte (gain));

  /* The abstract form of the save-time file is stored like any other
   * path */
  const LV2_State_Status status =
    write_status == 0
      ? store_path (
          features, store, handle, self->marker_file, self->atom_Path, path)
      : LV2_STATE_ERR_UNKNOWN;

  /* The runtime file was created at instantiate time: storing its
   * abstract path is what makes the host carry it into saved state */
  const LV2_State_Status runtime_status =
    store_path (
      features, store, handle, self->runtime_file, self->atom_Path,
      self->runtime_path);

  /* Same for the file created by the latest restore() */
  const LV2_State_Status restored_status =
    self->restored_path != NULL
      ? store_path (
          features, store, handle, self->restored_file, self->atom_Path,
          self->restored_path)
      : LV2_STATE_SUCCESS;

  free_path (features, path);
  if (status != LV2_STATE_SUCCESS)
    return status;
  if (runtime_status != LV2_STATE_SUCCESS)
    return runtime_status;
  return restored_status;
}

static LV2_State_Status
restore (
  LV2_Handle                   instance,
  LV2_State_Retrieve_Function  retrieve,
  LV2_State_Handle             handle,
  uint32_t                     flags,
  const LV2_Feature * const *  features)
{
  (void) flags;

  StateFiles * self = (StateFiles *) instance;

  const struct
  {
    uint32_t key;
    float *  value;
  } entries[] = {
    { self->marker_file, &self->marker },
    { self->runtime_file, &self->runtime },
  };
  for (size_t i = 0; i < sizeof (entries) / sizeof (entries[0]); ++i)
    {
      size_t   size = 0;
      uint32_t type = 0;
      uint32_t value_flags = 0;
      const void * value =
        retrieve (handle, entries[i].key, &size, &type, &value_flags);
      if (value == NULL || type != self->atom_Path)
        {
          /* No file in this state: keep the default */
          continue;
        }

      char * const resolved = resolved_path (features, (const char *) value);
      const char * path =
        resolved != NULL ? resolved : (const char *) value;
      *entries[i].value = (float) read_marker_file (path);
      free_path (features, resolved);
    }

  /* Create this instance's file through the restore-scoped
   * makePath, like a plugin rebuilding a cache from the restored
   * state */
  const LV2_State_Make_Path * make_path = NULL;
  for (const LV2_Feature * const * f = features; *f != NULL; ++f)
    {
      if (strcmp ((*f)->URI, LV2_STATE__makePath) == 0)
        {
          make_path = (const LV2_State_Make_Path *) (*f)->data;
        }
    }
  if (make_path != NULL && self->restored_path == NULL)
    {
      char * const path = make_path->path (make_path->handle,
                                           RESTORED_FILE_NAME);
      if (path == NULL)
        return LV2_STATE_ERR_UNKNOWN;
      self->restored_path = strdup (path);
      free_path (features, path);
      if (self->restored_path == NULL
          || write_marker_file (self->restored_path, 255))
        return LV2_STATE_ERR_UNKNOWN;
    }

  return LV2_STATE_SUCCESS;
}

static const LV2_State_Interface state_interface = { save, restore };

static const void *
extension_data (const char * uri)
{
  if (strcmp (uri, LV2_STATE__interface) == 0)
    return &state_interface;

  return NULL;
}

static const LV2_Descriptor state_files_descriptor = {
  STATE_FILES_URI, instantiate, connect_port, activate,
  run,             deactivate,  cleanup,       extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor *
lv2_descriptor (uint32_t index)
{
  return index == 0 ? &state_files_descriptor : NULL;
}
