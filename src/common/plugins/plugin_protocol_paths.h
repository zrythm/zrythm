// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "common/plugins/plugin_descriptor.h"
#include "common/utils/file_path_list.h"

namespace zrythm::plugins
{

class PluginProtocolPaths
{
public:
  static std::unique_ptr<utils::FilePathList>
  get_for_protocol (Protocol::ProtocolType protocol);

  static std::string
  get_for_protocol_separated (Protocol::ProtocolType protocol);

  static std::unique_ptr<utils::FilePathList> get_lv2_paths ();
  static std::unique_ptr<utils::FilePathList> get_vst2_paths ();
  static std::unique_ptr<utils::FilePathList> get_vst3_paths ();
  static std::unique_ptr<utils::FilePathList> get_sf_paths (bool sf2);
  static std::unique_ptr<utils::FilePathList> get_dssi_paths ();
  static std::unique_ptr<utils::FilePathList> get_ladspa_paths ();
  static std::unique_ptr<utils::FilePathList> get_clap_paths ();
  static std::unique_ptr<utils::FilePathList> get_jsfx_paths ();
  static std::unique_ptr<utils::FilePathList> get_au_paths ();
};

} // namespace zrythm::plugins