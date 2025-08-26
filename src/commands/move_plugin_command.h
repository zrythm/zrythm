// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "plugins/plugin_list.h"
#include "structure/tracks/automation_tracklist.h"

#include <QPointer>
#include <QUndoCommand>

namespace zrythm::commands
{
class MovePluginCommand : public QUndoCommand
{
public:
  using PluginLocation =
    std::pair<plugins::PluginList *, structure::tracks::AutomationTracklist *>;

  MovePluginCommand (
    plugins::PluginUuidReference plugin_ref,
    PluginLocation               source_locaction,
    PluginLocation               target_locaction,
    std::optional<int>           index = std::nullopt)
      : QUndoCommand (QObject::tr ("Move Plugin")),
        plugin_ref_ (std::move (plugin_ref)), target_location_ (target_locaction),
        source_location_ (source_locaction), index_ (index)
  {
    if (
      !std::ranges::contains (
        source_locaction.first->plugins (), plugin_ref_.id (),
        &plugins::PluginUuidReference::id))
      {
        throw std::invalid_argument ("Source list does not include the plugin");
      }
  }

  void undo () override
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
  void redo () override
  {
    // remember index in previous list
    const auto &prev_plugins = source_location_.first->plugins ();
    auto        prev_it = std::ranges::find (
      prev_plugins, plugin_ref_.id (), &plugins::PluginUuidReference::id);
    assert (prev_it != prev_plugins.end ());
    index_in_previous_list_ =
      static_cast<int> (std::ranges::distance (prev_plugins.begin (), prev_it));

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

private:
  void move_plugin_automation (
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

private:
  plugins::PluginUuidReference plugin_ref_;
  PluginLocation               target_location_;
  PluginLocation               source_location_;
  int                          index_in_previous_list_{};
  std::optional<int>           index_;
};

} // namespace zrythm::commands
