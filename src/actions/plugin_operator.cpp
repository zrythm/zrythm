// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

#include "actions/plugin_operator.h"
#include "commands/add_plugin_command.h"
#include "commands/move_plugins_command.h"
#include "commands/remove_plugins_command.h"
#include "plugins/plugin_instantiation_wait.h"
#include "utils/logger.h"
#include "utils/views.h"

namespace zrythm::actions
{

void
PluginOperator::movePlugins (
  QList<plugins::Plugin *>   plugins,
  plugins::PluginGroup *     source_group,
  structure::tracks::Track * source_track,
  plugins::PluginGroup *     target_group,
  structure::tracks::Track * target_track,
  int                        target_start_index)
{
  if (plugins.isEmpty () || !source_group || !target_group)
    {
      z_warning ("movePlugins: invalid arguments");
      return;
    }

  auto source_atl =
    source_track ? source_track->automationTracklist () : nullptr;
  auto target_atl =
    target_track ? target_track->automationTracklist () : nullptr;

  using PluginLocation = commands::MovePluginsCommand::PluginLocation;

  auto cmd = std::make_unique<commands::MovePluginsCommand> (
    plugins | std::views::filter ([] (auto * p) {
      return p != nullptr;
    }) | std::views::transform ([this, source_group, source_atl] (auto * plugin) {
      return commands::MovePluginsCommand::PluginMoveInfo{
        .plugin_ref =
          plugins::PluginUuidReference (plugin->get_uuid (), project_registry_),
        .source_location = PluginLocation{ source_group, source_atl }
      };
    }) | std::ranges::to<std::vector> (),
    PluginLocation{ target_group, target_atl },
    (target_start_index >= 0)
      ? std::optional<int> (target_start_index)
      : std::nullopt);

  undo_stack_.push (cmd.release ());
}

void
PluginOperator::removePlugins (
  QList<plugins::Plugin *>   plugins,
  plugins::PluginGroup *     group,
  structure::tracks::Track * track)
{
  if (plugins.isEmpty () || !group)
    {
      z_warning ("removePlugins: invalid arguments");
      return;
    }

  auto atl = track ? track->automationTracklist () : nullptr;

  auto cmd = std::make_unique<commands::RemovePluginsCommand> (
    plugins | std::views::filter ([] (auto * p) { return p != nullptr; })
    | std::views::transform ([this, group, atl] (auto * plugin) {
        return commands::RemovePluginsCommand::PluginRemoveInfo{
          .plugin_ref = plugins::PluginUuidReference (
            plugin->get_uuid (), project_registry_),
          .source_group = group,
          .source_atl = atl
        };
      })
    | std::ranges::to<std::vector> ());

  undo_stack_.push (cmd.release ());
}

bool
PluginOperator::copyPlugins (QList<plugins::Plugin *> plugins)
{
  auto payload = build_plugins_payload (plugins);
  if (!payload.has_value ())
    {
      refuse_operation (
        QObject::tr ("The plugins could not be copied to the clipboard"));
      return false;
    }

  try
    {
      clipboard_.setPayload (std::move (*payload));
    }
  catch (const std::exception &e)
    {
      z_warning ("Failed to store copied plugins: {}", e.what ());
      refuse_operation (
        QObject::tr ("The plugins could not be copied to the clipboard"));
      return false;
    }
  return true;
}

bool
PluginOperator::cutPlugins (
  QList<plugins::Plugin *>   plugins,
  plugins::PluginGroup *     group,
  structure::tracks::Track * track)
{
  if (plugins.isEmpty () || !group)
    {
      z_warning ("cutPlugins: invalid arguments");
      return false;
    }

  // Copy first: if it refuses, nothing is removed
  if (!copyPlugins (plugins))
    return false;

  undo::UndoStack::ScopedMacro macro (
    undo_stack_, QObject::tr ("Cut %1 Plugins").arg (plugins.size ()));
  removePlugins (plugins, group, track);
  return true;
}

QVariantList
PluginOperator::pastePlugins (plugins::PluginGroup * target_group, int index)
{
  if (target_group == nullptr)
    {
      z_warning ("pastePlugins: invalid arguments");
      return {};
    }

  const auto paste = prepare_plugin_paste ();
  if (!paste.has_value ())
    return {};

  return attach_paste (
    *paste, target_group, index, QObject::tr ("Paste %1 Plugins"));
}

QVariantList
PluginOperator::attach_paste (
  const PreparedPluginPaste &paste,
  plugins::PluginGroup *     target_group,
  int                        index,
  const QString             &label_pattern)
{
  std::vector<plugins::PluginUuidReference> targets;
  std::vector<QUuid>                        pasted_root_ids;
  // Set when a root must refuse the whole paste; handled after the loop:
  // the collected root references are dropped first so the discard can
  // sweep the imports
  bool    paste_refused = false;
  QString refusal_reason;

  for (const auto &root_id : paste.payload.roots ())
    {
      plugins::PluginUuidReference root_ref{
        plugins::Plugin::Uuid (root_id), project_registry_
      };
      auto * plugin = root_ref.get ();
      if (plugin == nullptr)
        {
          z_warning ("Clipboard: imported plugin {} not found", root_id);
          continue;
        }

      const auto group_type = plugins::PluginGroup::group_type_for_descriptor (
        plugin->get_descriptor ());
      if (group_type != target_group->type ())
        {
          z_warning (
            "Cannot paste: plugin {} does not fit the target group's "
            "category",
            root_id);
          paste_refused = true;
          refusal_reason =
            QObject::tr ("The clipboard plugins do not fit this slot category");
          break;
        }

      targets.push_back (std::move (root_ref));
      pasted_root_ids.push_back (root_id);
    }

  if (paste_refused)
    {
      // Drop the references of already-collected roots first: their
      // destruction releases (and cascades away) the imported objects,
      // so the cleanup below only needs to sweep what is left
      targets.clear ();
      paste.payload.cleanup_failed_import (
        project_registry_, paste.imported_ids);
      refuse_operation (refusal_reason);
      return {};
    }

  if (targets.empty ())
    {
      paste.payload.cleanup_failed_import (
        project_registry_, paste.imported_ids);
      z_warning ("None of the clipboard plugins could be pasted here");
      refuse_operation (
        QObject::tr ("The clipboard plugins could not be pasted here"));
      return {};
    }

  plugins::wait_for_plugin_instantiations (project_registry_, pasted_root_ids);
  const auto all_instantiated =
    std::ranges::all_of (targets, [] (const auto &root_ref) {
      return root_ref.get ()->instantiationStatus ()
             == plugins::Plugin::InstantiationStatus::Successful;
    });
  if (!all_instantiated)
    {
      z_warning (
        "Cannot paste: some pasted plugins are not instantiated (failed "
        "or timed out)");
      // Drop the collected root references first: their destruction
      // releases the imported objects, letting the cleanup below sweep
      // them. An in-flight creation callback for a deleted plugin is a
      // no-op: it is guarded by the plugin's weak self-reference
      targets.clear ();
      paste.payload.cleanup_failed_import (
        project_registry_, paste.imported_ids);
      refuse_operation (
        QObject::tr ("A clipboard plugin failed to instantiate"));
      return {};
    }

  // Sweep the imports the pasted roots do not need. Copied payloads
  // carry exactly their closure, so this is a no-op for them; payloads
  // carrying surplus entries get them deregistered instead of leaving
  // unowned objects behind
  paste.payload.discard_imports_except (
    project_registry_, paste.imported_ids, pasted_root_ids);

  QVariantList                 new_ids;
  undo::UndoStack::ScopedMacro macro (
    undo_stack_, label_pattern.arg (targets.size ()));
  for (auto [i, target] : utils::views::enumerate (targets))
    {
      undo_stack_.push (
        std::make_unique<commands::AddPluginCommand> (
          *target_group, target,
          (index >= 0)
            ? std::optional<int> (index + static_cast<int> (i))
            : std::nullopt)
          .release ());
      new_ids.push_back (
        type_safe::get (target.id ()).toString (QUuid::WithoutBraces));
    }
  return new_ids;
}

QVariantList
PluginOperator::duplicatePlugins (
  QList<plugins::Plugin *> plugins,
  plugins::PluginGroup *   group)
{
  if (plugins.isEmpty () || !group)
    {
      z_warning ("duplicatePlugins: invalid arguments");
      return {};
    }

  // Paste right after the last duplicated plugin still in the group; if
  // none of them is in the group anymore, append
  std::vector<plugins::PluginUuidReference> group_plugins;
  group->get_plugins (group_plugins, /*recursive=*/false);
  int last_source_index = -1;
  for (const auto [i, plugin_ref] : utils::views::enumerate (group_plugins))
    {
      const auto is_source =
        std::ranges::any_of (plugins, [&plugin_ref] (const auto * plugin) {
          return plugin != nullptr && plugin->get_uuid () == plugin_ref.id ();
        });
      if (is_source)
        last_source_index = static_cast<int> (i);
    }
  const int insert_index = last_source_index >= 0 ? last_source_index + 1 : -1;

  // Clone the plugins directly instead of routing through the
  // clipboard: duplicating must not replace the clipboard contents
  const auto payload = build_plugins_payload (plugins);
  if (!payload.has_value ())
    {
      refuse_operation (QObject::tr ("The plugins could not be duplicated"));
      return {};
    }
  const auto paste = import_payload_for_paste (*payload);
  if (!paste.has_value ())
    return {};

  return attach_paste (
    *paste, group, insert_index, QObject::tr ("Duplicate %1 Plugins"));
}

std::optional<structure::project::ClipboardPayload>
PluginOperator::build_plugins_payload (
  const QList<plugins::Plugin *> &plugins) const
{
  std::vector<const plugins::Plugin *> copyable;
  for (const auto * plugin : plugins)
    {
      if (plugin != nullptr)
        copyable.push_back (plugin);
    }
  if (copyable.empty ())
    {
      z_warning ("No copyable plugins selected");
      return std::nullopt;
    }

  std::vector<QUuid> roots;
  roots.reserve (copyable.size ());
  for (const auto * plugin : copyable)
    roots.push_back (type_safe::get (plugin->get_uuid ()));

  try
    {
      return structure::project::ClipboardPayload::create (
        project_registry_, structure::project::ClipboardPayload::Type::Plugins,
        roots, project_id_provider_ ());
    }
  catch (const std::exception &e)
    {
      z_warning ("Failed to serialize the plugins: {}", e.what ());
      return std::nullopt;
    }
}

std::optional<PluginOperator::PreparedPluginPaste>
PluginOperator::prepare_plugin_paste ()
{
  const auto &clipboard_payload = clipboard_.payload ();
  if (!clipboard_payload.has_value ())
    {
      z_debug ("No clipboard payload to paste");
      return std::nullopt;
    }
  if (
    clipboard_payload->type ()
    != structure::project::ClipboardPayload::Type::Plugins)
    {
      z_warning ("The clipboard does not contain plugins");
      refuse_operation (
        QObject::tr ("The clipboard contents could not be pasted"));
      return std::nullopt;
    }

  // Plugin pastes cannot attach tracks or arranger objects: refuse
  // payloads that carry any, or their objects would stay unowned
  {
    const auto &registry = clipboard_payload->registry_json ();
    for (
      const auto bucket_key :
      { structure::project::ProjectRegistry::kTracksKey,
        structure::project::ProjectRegistry::kArrangerObjectsKey })
      {
        const auto bucket_it = registry.find (bucket_key);
        if (bucket_it != registry.end () && !bucket_it->empty ())
          {
            z_warning (
              "The clipboard payload contains {} and cannot be pasted as "
              "plugins",
              bucket_key);
            refuse_operation (
              QObject::tr ("The clipboard contents could not be pasted"));
            return std::nullopt;
          }
      }
  }

  return import_payload_for_paste (*clipboard_payload);
}

std::optional<PluginOperator::PreparedPluginPaste>
PluginOperator::import_payload_for_paste (
  const structure::project::ClipboardPayload &payload)
{
  PreparedPluginPaste prepared;
  try
    {
      auto filtered = payload.filtered_for_target (project_registry_);
      if (!filtered.payload.has_value ())
        {
          z_warning (
            "The clipboard contents cannot be pasted into this project");
          refuse_operation (
            QObject::tr ("The clipboard contents could not be pasted"));
          return std::nullopt;
        }

      prepared.payload = structure::project::ClipboardPayload::
        with_regenerated_uuids (std::move (*filtered.payload));
      prepared.imported_ids = prepared.payload.import_into (project_registry_);
    }
  catch (const std::exception &e)
    {
      z_warning (
        "Failed to prepare the clipboard contents for pasting: {}", e.what ());
      refuse_operation (
        QObject::tr ("The clipboard contents could not be pasted"));
      return std::nullopt;
    }
  return prepared;
}

void
PluginOperator::refuse_operation (const QString &reason)
{
  z_warning ("Refusing operation: {}", reason);
  Q_EMIT operationRefused (reason);
}

} // namespace zrythm::actions
