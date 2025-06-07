// SPDX-FileCopyrightText: © 2021-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_configuration.h"

namespace zrythm::plugins
{

void
PluginConfiguration::copy_fields_from (const PluginConfiguration &other)
{
  descr_ = utils::clone_unique (*other.descr_);
  hosting_type_ = other.hosting_type_;
  force_generic_ui_ = other.force_generic_ui_;
  bridge_mode_ = other.bridge_mode_;
}

void
init_from (
  PluginConfiguration       &obj,
  const PluginConfiguration &other,
  utils::ObjectCloneType     clone_type)
{
  obj.copy_fields_from (other);
}

std::unique_ptr<PluginConfiguration>
PluginConfiguration::create_new_for_descriptor (
  const zrythm::plugins::PluginDescriptor &descr)
{
  auto ret = std::make_unique<PluginConfiguration> ();
  ret->descr_ = utils::clone_unique (descr);
  ret->validate ();
  return ret;
}

void
PluginConfiguration::print () const
{
  z_debug ("{}", *this);
}

void
PluginConfiguration::validate ()
{
  /* if no custom UI, force generic */
  if (!descr_->has_custom_ui_)
    {
      force_generic_ui_ = true;
    }
}

void
from_json (const nlohmann::json &j, PluginConfiguration &p)
{
  p.descr_ = std::make_unique<zrythm::plugins::PluginDescriptor> ();
  j.at (PluginConfiguration::kDescriptorKey).get_to (*p.descr_);
  j.at (PluginConfiguration::kForceGenericUIKey).get_to (p.force_generic_ui_);
  j.at (PluginConfiguration::kBridgeModeKey).get_to (p.bridge_mode_);
}
}
