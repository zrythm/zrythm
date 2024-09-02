// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "doctest_wrapper.h"

#ifdef HAVE_CARLA

#  include "plugins/cached_plugin_descriptors.h"
#  include "plugins/carla_discovery.h"
#  include "plugins/plugin_descriptor.h"
#  include "plugins/plugin_manager.h"
#  include "utils/string.h"
#  include "zrythm.h"
#  include "zrythm_app.h"

#  include "carla_wrapper.h"

ZCarlaDiscovery::ZCarlaDiscovery (PluginManager &owner)
{
  owner_ = &owner;
}

fs::path
ZCarlaDiscovery::get_discovery_path (PluginArchitecture arch)
{
  std::string carla_discovery_filename;
  carla_discovery_filename =
#  ifdef _WIN32
    arch == PluginArchitecture::ARCH_32_BIT
      ? "carla-discovery-win32"
      : "carla-discovery-native";
#  else
    "carla-discovery-native";
#  endif
  carla_discovery_filename += BIN_SUFFIX;
  fs::path carla_discovery_parent_dir;
  if (ZRYTHM_TESTING || !ZRYTHM_BENCHMARKING)
    {
      carla_discovery_parent_dir = fs::path (CARLA_BINARIES_DIR);
    }
  else
    {
      auto *      dir_mgr = ZrythmDirectoryManager::getInstance ();
      std::string zrythm_libdir =
        dir_mgr->get_dir (ZrythmDirType::SYSTEM_ZRYTHM_LIBDIR);
      z_debug ("using zrythm_libdir: {}", zrythm_libdir);
      carla_discovery_parent_dir = fs::path (zrythm_libdir) / "carla";
    }

  fs::path carla_discovery_abs_path =
    carla_discovery_parent_dir / carla_discovery_filename;
  z_return_val_if_fail (
    carla_discovery_abs_path.is_absolute ()
      && fs::exists (carla_discovery_abs_path),
    "");
  return carla_discovery_abs_path;
}

std::unique_ptr<PluginDescriptor>
ZCarlaDiscovery::create_au_descriptor_from_info (
  const CarlaCachedPluginInfo * info)
{
  if (!info || !info->valid)
    return NULL;

  auto ret = std::make_unique<PluginDescriptor> ();
  ret->name_ = info->name;
  z_return_val_if_fail (!ret->name_.empty (), nullptr);
  ret->author_ = info->maker;
  ret->num_audio_ins_ = info->audioIns;
  ret->num_audio_outs_ = info->audioOuts;
  ret->num_cv_ins_ = info->cvIns;
  ret->num_cv_outs_ = info->cvOuts;
  ret->num_ctrl_ins_ = info->parameterIns;
  ret->num_ctrl_outs_ = info->parameterOuts;
  ret->num_midi_ins_ = info->midiIns;
  ret->num_midi_outs_ = info->midiOuts;

  /* get category */
  if (info->hints & CarlaBackend::PLUGIN_IS_SYNTH)
    {
      ret->category_ = ZPluginCategory::INSTRUMENT;
    }
  else
    {
      ret->category_ =
        PluginDescriptor::get_category_from_carla_category (info->category);
    }
  ret->category_str_ = PluginDescriptor::category_to_string (ret->category_);

  ret->protocol_ = PluginProtocol::AU;
  ret->arch_ = PluginArchitecture::ARCH_64_BIT;
  ret->path_ = "";
  ret->min_bridge_mode_ = ret->get_min_bridge_mode ();
  ret->has_custom_ui_ = info->hints & CarlaBackend::PLUGIN_HAS_CUSTOM_UI;

  return ret;
}

std::unique_ptr<PluginDescriptor>
ZCarlaDiscovery::descriptor_from_discovery_info (
  const CarlaPluginDiscoveryInfo * info,
  const std::string_view           sha1)
{
  z_return_val_if_fail (info, nullptr);
  const CarlaPluginDiscoveryMetadata * meta = &info->metadata;

  auto descr = std::make_unique<PluginDescriptor> ();
  descr->protocol_ =
    PluginDescriptor::get_protocol_from_carla_plugin_type (info->ptype);
  z_return_val_if_fail (descr->protocol_ > PluginProtocol::DUMMY, nullptr);
  const char * path = NULL;
  const char * uri = NULL;
  if (descr->protocol_ == PluginProtocol::SFZ)
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
      descr->path_ = g_strdup (path);
      if (!descr->path_.empty ())
        {
          /* ghash only used for compatibility with older projects */
          GFile * file = g_file_new_for_path (descr->path_.c_str ());
          descr->ghash_ = g_file_hash (file);
          g_object_unref (file);
        }
    }
  descr->uri_ = uri;
  descr->unique_id_ = (int64_t) info->uniqueId;
  descr->name_ = meta->name;
  descr->author_ = meta->maker;
  descr->category_ =
    PluginDescriptor::get_category_from_carla_category (meta->category);
  if (
    meta->hints & CarlaBackend::PLUGIN_IS_SYNTH
    && descr->category_ == ZPluginCategory::NONE)
    {
      descr->category_ = ZPluginCategory::INSTRUMENT;
    }
  descr->category_str_ = PluginDescriptor::category_to_string (descr->category_);
  descr->sha1_ = sha1;
  const CarlaPluginDiscoveryIO * io = &info->io;
  descr->num_audio_ins_ = (int) io->audioIns;
  descr->num_audio_outs_ = (int) io->audioOuts;
  descr->num_cv_ins_ = (int) io->cvIns;
  descr->num_cv_outs_ = (int) io->cvOuts;
  descr->num_midi_ins_ = (int) io->midiIns;
  descr->num_midi_outs_ = (int) io->midiOuts;
  descr->num_ctrl_ins_ = (int) io->parameterIns;
  descr->num_ctrl_outs_ = (int) io->parameterOuts;
  descr->arch_ =
    info->btype == CarlaBackend::BINARY_NATIVE
      ? PluginArchitecture::ARCH_64_BIT
      : PluginArchitecture::ARCH_32_BIT;
  descr->has_custom_ui_ = meta->hints & CarlaBackend::PLUGIN_HAS_CUSTOM_UI;
  descr->min_bridge_mode_ = CarlaBridgeMode::Full;

  return descr;
}

static void
z_carla_discovery_plugin_scanned_cb (
  void *                           ptr,
  const CarlaPluginDiscoveryInfo * nfo,
  const char *                     sha1)
{
  z_return_if_fail (ptr);
  ZCarlaDiscovery * self = (ZCarlaDiscovery *) ptr;

  /*z_debug ("nfo: {:p}, sha1: {}", nfo, sha1);*/

  if (!nfo && !sha1)
    {
      /* no plugin and discovery is trivial */
      return;
    }

  auto  pm = self->owner_;
  auto &cached_descriptors = pm->cached_plugin_descriptors_;

  /* if discovery is expensive and this binary contains no plugins, remember it
   * (as blacklisted) */
  if (!nfo && sha1)
    {
      if (!cached_descriptors->is_blacklisted (sha1))
        {
          cached_descriptors->blacklist (sha1, false);
        }
      return;
    }

  std::string sha1_str = sha1 ? sha1 : "";
  auto descr = ZCarlaDiscovery::descriptor_from_discovery_info (nfo, sha1_str);
  z_debug ("scanned {}", descr->uri_);
  pm->add_descriptor (*descr);

  /* only cache expensive plugins */
  if (sha1)
    {
      cached_descriptors->add (*descr, false);
    }
}

static bool
z_carla_discovery_plugin_check_cache_cb (
  void *       ptr,
  const char * filename,
  const char * sha1)
{
  z_return_val_if_fail (ptr && filename && sha1, true);
  ZCarlaDiscovery * self = (ZCarlaDiscovery *) ptr;
  z_debug ("check cache for: filename: {} | sha1: {}", filename, sha1);
  auto  pl_mgr = self->owner_;
  auto &caches = pl_mgr->cached_plugin_descriptors_;
  if (caches->contains_sha1 (sha1, true, true))
    {
      auto descrs = caches->get_valid_descriptors_for_sha1 (sha1);
      for (auto &descr : descrs)
        {
          pl_mgr->add_descriptor (descr);
        }
      return true;
    }

  pl_mgr->set_currently_scanning_plugin (filename, sha1);

  return false;
}

bool
ZCarlaDiscovery::idle ()
{
  bool all_done = true;
  for (auto &[handle, done] : handles_)
    {
      z_return_val_if_fail (handle, true);
      if (done)
        continue;

      done = carla_plugin_discovery_idle (handle) == false;
      if (done)
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
ZCarlaDiscovery::start (BinaryType btype, PluginProtocol protocol)
{
  auto discovery_tool = get_discovery_path (
    btype == CarlaBackend::BINARY_NATIVE
      ? PluginArchitecture::ARCH_64_BIT
      : PluginArchitecture::ARCH_32_BIT);
  auto paths_separated = owner_->get_paths_for_protocol_separated (protocol);
  PluginType ptype =
    PluginDescriptor::get_carla_plugin_type_from_protocol (protocol);
  void * handle = carla_plugin_discovery_start (
    discovery_tool.c_str (), btype, ptype, paths_separated.c_str (),
    z_carla_discovery_plugin_scanned_cb,
    z_carla_discovery_plugin_check_cache_cb, this);
  if (!handle)
    {
      z_debug (
        "no plugins to scan for {} (given paths: {})", ENUM_NAME (protocol),
        paths_separated);
      return;
    }
  handles_.emplace_back (handle, false);
}

#endif
