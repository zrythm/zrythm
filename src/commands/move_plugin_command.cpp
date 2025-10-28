// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/move_plugin_command.h"

namespace zrythm::commands
{
MovePluginCommand::MovePluginCommand (
  plugins::PluginUuidReference plugin_ref,
  PluginLocation               source_locaction,
  PluginLocation               target_locaction,
  std::optional<int>           index)
    : QUndoCommand (QObject::tr ("Move Plugin")),
      plugin_ref_ (std::move (plugin_ref)), target_location_ (target_locaction),
      source_location_ (source_locaction), index_ (index)
{
  std::vector<plugins::PluginPtrVariant> plugins;
  source_locaction.first->get_plugins (plugins);
  if (
    !std::ranges::contains (
      plugins | std::views::transform (plugins::plugin_ptr_variant_to_base),
      plugin_ref_.id (), &plugins::Plugin::get_uuid))
    {
      throw std::invalid_argument ("Source list does not include the plugin");
    }
}

void
MovePluginCommand::undo ()
{
  // move plugin
  auto plugin_ref = target_location_.first->remove_plugin (plugin_ref_.id ());
  source_location_.first->insert_plugin (plugin_ref, index_in_previous_list_);

  // move automation for plugin
  auto * source_atl = source_location_.second;
  auto * target_atl = target_location_.second;
  if ((source_atl != nullptr) && (target_atl != nullptr))
    {
      move_plugin_automation (*target_atl, *source_atl);
    }
}
void
MovePluginCommand::redo ()
{
  // remember index in previous list
  for (const auto i : std::views::iota (0, source_location_.first->rowCount ()))
    {
      const auto qvar = source_location_.first->element_at_idx (i);
      if (!qvar.canConvert<plugins::Plugin *> ())
        continue;

      const auto * pl = qvar.value<plugins::Plugin *> ();
      if (pl->get_uuid () == plugin_ref_.id ())
        {
          index_in_previous_list_ = i;
          break;
        }
    }

  // move plugin
  auto plugin_ref = source_location_.first->remove_plugin (plugin_ref_.id ());
  auto index_to_insert = index_.value_or (-1);
  target_location_.first->insert_plugin (plugin_ref, index_to_insert);

  // move automation for plugin
  auto * source_atl = source_location_.second;
  auto * target_atl = target_location_.second;
  if ((source_atl != nullptr) && (target_atl != nullptr))
    {
      move_plugin_automation (*source_atl, *target_atl);
    }
}

void
MovePluginCommand::move_plugin_automation (
  structure::tracks::AutomationTracklist &from_atl,
  structure::tracks::AutomationTracklist &to_atl)
{
  auto * plugin =
    plugins::plugin_ptr_variant_to_base (plugin_ref_.get_object ());
  for (auto * at : from_atl.automation_tracks ())
    {
      auto * at_param = at->parameter ();
      for (const auto &pl_param_ref : plugin->get_parameters ())
        {
          const auto * pl_param =
            pl_param_ref.get_object_as<dsp::ProcessorParameter> ();
          if (pl_param == at_param)
            {
              auto removed_at = from_atl.remove_automation_track (*at);
              to_atl.add_automation_track (std::move (removed_at));
            }
        }
    }
}

} // namespace zrythm::commands
