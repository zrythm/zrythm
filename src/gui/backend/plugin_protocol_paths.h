// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/plugin_protocol.h"
#include "utils/file_path_list.h"

namespace zrythm::plugins
{

class PluginProtocolPaths
{
public:
  explicit PluginProtocolPaths (utils::AppSettings &app_settings);

  std::unique_ptr<utils::FilePathList>
  get_for_protocol (Protocol::ProtocolType protocol);

  utils::Utf8String
  get_for_protocol_separated (Protocol::ProtocolType protocol);

  std::unique_ptr<utils::FilePathList> get_lv2_paths ();
  std::unique_ptr<utils::FilePathList> get_vst2_paths ();
  std::unique_ptr<utils::FilePathList> get_vst3_paths ();
  std::unique_ptr<utils::FilePathList> get_sf_paths (bool sf2);
  std::unique_ptr<utils::FilePathList> get_dssi_paths ();
  std::unique_ptr<utils::FilePathList> get_ladspa_paths ();
  std::unique_ptr<utils::FilePathList> get_clap_paths ();
  std::unique_ptr<utils::FilePathList> get_jsfx_paths ();
  std::unique_ptr<utils::FilePathList> get_au_paths ();

private:
  utils::AppSettings &app_settings_;
};

} // namespace zrythm::plugins
