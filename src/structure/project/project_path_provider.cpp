// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/project_path_provider.h"

namespace zrythm::gui
{

fs::path
ProjectPathProvider::get_path (ProjectPath path)
{
  switch (path)
    {
    case ProjectPath::BackupsDir:
      return PROJECT_BACKUPS_DIR;
    case ProjectPath::ExportsDir:
      return PROJECT_EXPORTS_DIR;
    case ProjectPath::ExportStemsDir:
      return utils::Utf8String::from_utf8_encoded_string (PROJECT_EXPORTS_DIR)
               .to_path ()
             / PROJECT_STEMS_DIR;
    case ProjectPath::PluginsDir:
      return PROJECT_PLUGINS_DIR;
    case ProjectPath::PluginStates:
      {
        auto plugins_dir = get_path (ProjectPath::PluginsDir);
        return plugins_dir / PROJECT_PLUGIN_STATES_DIR;
      }
      break;
    case ProjectPath::PLUGIN_EXT_COPIES:
      {
        auto plugins_dir = get_path (ProjectPath::PluginsDir);
        return plugins_dir / PROJECT_PLUGIN_EXT_COPIES_DIR;
      }
      break;
    case ProjectPath::PLUGIN_EXT_LINKS:
      {
        auto plugins_dir = get_path (ProjectPath::PluginsDir);
        return plugins_dir / PROJECT_PLUGIN_EXT_LINKS_DIR;
      }
      break;
    case ProjectPath::AudioFilePoolDir:
      return PROJECT_POOL_DIR;
    case ProjectPath::ProjectFile:
      return PROJECT_FILE;
    }
  throw std::runtime_error ("Invalid path type.");
}
}
