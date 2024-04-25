// clang-format off
// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

#include "zrythm-config.h"

#ifdef HAVE_CARLA

#  include <stdlib.h>

#  include "plugins/cached_plugin_descriptors.h"
#  include "plugins/carla_discovery.h"
#  include "plugins/plugin_descriptor.h"
#  include "plugins/plugin_manager.h"
#  include "utils/file.h"
#  include "utils/objects.h"
#  include "utils/string.h"
#  include "utils/system.h"
#  include "zrythm.h"

#  include <CarlaBackend.h>

static char *
z_carla_discovery_get_discovery_path (PluginArchitecture arch)
{
  char carla_discovery_filename[60];
  strcpy (
    carla_discovery_filename,
#  ifdef _WOE32
    arch == ARCH_32 ? "carla-discovery-win32" : "carla-discovery-native"
#  else
    "carla-discovery-native"
#  endif
  );
  strcat (carla_discovery_filename, BIN_SUFFIX);
  char * carla_discovery;
  if (ZRYTHM_TESTING)
    {
      carla_discovery =
        g_build_filename (CARLA_BINARIES_DIR, carla_discovery_filename, NULL);
    }
  else
    {
      char * zrythm_libdir = zrythm_get_dir (ZRYTHM_DIR_SYSTEM_ZRYTHM_LIBDIR);
      g_debug ("using zrythm_libdir: %s", zrythm_libdir);
      carla_discovery = g_build_filename (
        zrythm_libdir, "carla", carla_discovery_filename, NULL);
      g_free (zrythm_libdir);
    }
  g_return_val_if_fail (file_exists (carla_discovery), NULL);

  return carla_discovery;
}

/**
 * Create a descriptor for the given AU plugin.
 *
 * FIXME merge with
 * carla_native_plugin_get_descriptor_from_cached().
 */
PluginDescriptor *
z_carla_discovery_create_au_descriptor_from_info (
  const CarlaCachedPluginInfo * info)
{
  if (!info || !info->valid)
    return NULL;

  PluginDescriptor * descr = plugin_descriptor_new ();
  descr->name = g_strdup (info->name);
  g_return_val_if_fail (descr->name, NULL);
  descr->author = g_strdup (info->maker);
  descr->num_audio_ins = (int) info->audioIns;
  descr->num_audio_outs = (int) info->audioOuts;
  descr->num_cv_ins = (int) info->cvIns;
  descr->num_cv_outs = (int) info->cvOuts;
  descr->num_ctrl_ins = (int) info->parameterIns;
  descr->num_ctrl_outs = (int) info->parameterOuts;
  descr->num_midi_ins = (int) info->midiIns;
  descr->num_midi_outs = (int) info->midiOuts;

  /* get category */
  if (info->hints & PLUGIN_IS_SYNTH)
    {
      descr->category = PC_INSTRUMENT;
    }
  else
    {
      descr->category =
        plugin_descriptor_get_category_from_carla_category (info->category);
    }
  descr->category_str = plugin_descriptor_category_to_string (descr->category);

  descr->protocol = Z_PLUGIN_PROTOCOL_AU;
  descr->arch = ARCH_64;
  descr->path = NULL;
  descr->min_bridge_mode = plugin_descriptor_get_min_bridge_mode (descr);
  descr->has_custom_ui = info->hints & PLUGIN_HAS_CUSTOM_UI;

  return descr;
}

static PluginDescriptor *
descriptor_from_discovery_info (
  const CarlaPluginDiscoveryInfo * info,
  const char *                     sha1)
{
  g_return_val_if_fail (info, NULL);
  const CarlaPluginDiscoveryMetadata * meta = &info->metadata;

  PluginDescriptor * descr = plugin_descriptor_new ();
  descr->protocol =
    plugin_descriptor_get_protocol_from_carla_plugin_type (info->ptype);
  const char * path = NULL;
  const char * uri = NULL;
  if (descr->protocol == Z_PLUGIN_PROTOCOL_SFZ)
    {
      path = info->label;
      uri = meta->name;
    }
  else
    {
      path = info->filename;
      uri = info->label;
    }

  if (path && !string_is_equal (path, ""))
    {
      descr->path = g_strdup (path);
      if (descr->path)
        {
          /* ghash only used for compatibility with older projects */
          GFile * file = g_file_new_for_path (descr->path);
          descr->ghash = g_file_hash (file);
          g_object_unref (file);
        }
    }
  descr->uri = g_strdup (uri);
  descr->unique_id = (int64_t) info->uniqueId;
  descr->name = g_strdup (meta->name);
  descr->author = g_strdup (meta->maker);
  descr->category =
    plugin_descriptor_get_category_from_carla_category (meta->category);
  if (meta->hints & PLUGIN_IS_SYNTH && descr->category == ZPLUGIN_CATEGORY_NONE)
    {
      descr->category = PC_INSTRUMENT;
    }
  descr->category_str = plugin_descriptor_category_to_string (descr->category);
  descr->sha1 = g_strdup (sha1);
  const CarlaPluginDiscoveryIO * io = &info->io;
  descr->num_audio_ins = (int) io->audioIns;
  descr->num_audio_outs = (int) io->audioOuts;
  descr->num_cv_ins = (int) io->cvIns;
  descr->num_cv_outs = (int) io->cvOuts;
  descr->num_midi_ins = (int) io->midiIns;
  descr->num_midi_outs = (int) io->midiOuts;
  descr->num_ctrl_ins = (int) io->parameterIns;
  descr->num_ctrl_outs = (int) io->parameterOuts;
  descr->arch = info->btype == BINARY_NATIVE ? ARCH_64 : ARCH_32;
  descr->has_custom_ui = meta->hints & PLUGIN_HAS_CUSTOM_UI;
  descr->min_bridge_mode = CARLA_BRIDGE_FULL;

  return descr;
}

static void
z_carla_discovery_plugin_scanned_cb (
  void *                           ptr,
  const CarlaPluginDiscoveryInfo * nfo,
  const char *                     sha1)
{
  g_return_if_fail (ptr);
  ZCarlaDiscovery * self = (ZCarlaDiscovery *) ptr;

  /*g_debug ("nfo: %p, sha1: %s", nfo, sha1);*/

  if (!nfo && !sha1)
    {
      /* no plugin and discovery is trivial */
      return;
    }

  /* if discovery is expensive and this binary contains no plugins, remember it
   * (as blacklisted) */
  if (!nfo && sha1)
    {
      if (!cached_plugin_descriptors_is_blacklisted (
            self->owner->cached_plugin_descriptors, sha1))
        {
          cached_plugin_descriptors_blacklist (
            self->owner->cached_plugin_descriptors, sha1, false);
        }
      return;
    }

  PluginDescriptor * descr = descriptor_from_discovery_info (nfo, sha1);
  g_debug ("scanned %s", descr->uri);
  plugin_manager_add_descriptor (self->owner, descr);

  /* only cache expensive plugins */
  if (sha1)
    {
      cached_plugin_descriptors_add (
        self->owner->cached_plugin_descriptors, descr, false);
    }
}

static bool
z_carla_discovery_plugin_check_cache_cb (
  void *       ptr,
  const char * filename,
  const char * sha1)
{
  g_return_val_if_fail (ptr && filename && sha1, true);
  ZCarlaDiscovery * self = (ZCarlaDiscovery *) ptr;
  g_debug ("check cache for: filename: %s | sha1: %s", filename, sha1);
  const PluginDescriptor * descr = cached_plugin_descriptors_find (
    self->owner->cached_plugin_descriptors, NULL, sha1, true, true);
  if (descr)
    {
      if (!cached_plugin_descriptors_is_blacklisted (
            self->owner->cached_plugin_descriptors, sha1))
        {
          PluginDescriptor * descr_clone = plugin_descriptor_clone (descr);
          plugin_manager_add_descriptor (self->owner, descr_clone);
        }
      return true;
    }

  plugin_manager_set_currently_scanning_plugin (self->owner, filename, sha1);

  return false;
}

bool
z_carla_discovery_idle (ZCarlaDiscovery * self)
{
  bool all_done = true;
  for (size_t i = 0; i < self->handles->len; i++)
    {
      void * handle = g_ptr_array_index (self->handles, i);
      bool * done = &g_array_index (self->handles_done, bool, i);
      g_return_val_if_fail (handle, true);
      if (*done)
        continue;

      *done = carla_plugin_discovery_idle (handle) == false;
      if (*done)
        {
          carla_plugin_discovery_stop (handle);
        }
      else
        {
          all_done = false;
        }
    }

  return all_done;
}

void
z_carla_discovery_start (
  ZCarlaDiscovery * self,
  BinaryType        btype,
  ZPluginProtocol   protocol)
{
  char * discovery_tool = z_carla_discovery_get_discovery_path (
    btype == BINARY_NATIVE ? ARCH_64 : ARCH_32);
  char * paths_separated =
    plugin_manager_get_paths_for_protocol_separated (self->owner, protocol);
  PluginType ptype =
    plugin_descriptor_get_carla_plugin_type_from_protocol (protocol);
  void * handle = carla_plugin_discovery_start (
    discovery_tool, btype, ptype, paths_separated,
    z_carla_discovery_plugin_scanned_cb,
    z_carla_discovery_plugin_check_cache_cb, self);
  if (!handle)
    {
      g_message (
        "no plugins to scan for %s (given paths: %s)",
        plugin_protocol_strings[protocol].str, paths_separated);
      g_free (paths_separated);
      return;
    }
  g_free (paths_separated);
  g_return_if_fail (handle);
  g_ptr_array_add (self->handles, handle);
  bool var = false;
  g_array_append_val (self->handles_done, var);
}

ZCarlaDiscovery *
z_carla_discovery_new (PluginManager * owner)
{
  ZCarlaDiscovery * self = object_new (ZCarlaDiscovery);
  self->owner = owner;
  self->handles = g_ptr_array_new ();
  self->handles_done = g_array_new (false, true, sizeof (bool));
  return self;
}

void
z_carla_discovery_free (ZCarlaDiscovery * self)
{
  object_free_w_func_and_null (g_ptr_array_unref, self->handles);
  object_free_w_func_and_null (g_array_unref, self->handles_done);

  object_zero_and_free (self);
}

#endif
