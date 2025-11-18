// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/plugin_importer.h"
#include "commands/add_plugin_command.h"

namespace zrythm::actions
{

PluginImporter::PluginImporter (
  undo::UndoStack        &undo_stack,
  plugins::PluginFactory &plugin_factory,
  const TrackCreator     &track_creator,
  plugins::PluginFactory::InstantiationFinishedHandler
            instantiation_finished_handler,
  QObject * parent)
    : QObject (parent), undo_stack_ (undo_stack),
      plugin_factory_ (plugin_factory), track_creator_ (track_creator),
      instantiation_finished_handler_ (std::move (instantiation_finished_handler))
{
}

void
PluginImporter::importPluginToNewTrack (
  const plugins::PluginDescriptor * descriptor)
{
  import (descriptor, std::nullopt);
}

void
PluginImporter::importPluginToGroup (
  const plugins::PluginDescriptor * descriptor,
  plugins::PluginGroup *            group)
{
  import (descriptor, group);
}

void
PluginImporter::importPluginToTrack (
  const plugins::PluginDescriptor * descriptor,
  structure::tracks::Track *        track)
{
  import (descriptor, track);
}

void
PluginImporter::import (
  const plugins::PluginDescriptor * descriptor,
  std::optional<std::variant<plugins::PluginGroup *, structure::tracks::Track *>>
    track_or_group)
{
  if (descriptor == nullptr)
    {
      z_warning ("Cannot import null plugin descriptor");
      return;
    }

  z_debug ("Importing plugin: {}", descriptor->name ());

  // Create plugin configuration from descriptor
  auto config =
    plugins::PluginConfiguration::create_new_for_descriptor (*descriptor);

  // Create plugin instance using the plugin factory
  // Note that we don't actually do anything meaningful until the plugin is
  // instantiated
  plugin_factory_.create_plugin_from_setting (
    *config,
    plugins::PluginFactory::InstantiationFinishOptions{
      .handler_ =
        [this, track_or_group] (plugins::PluginUuidReference inner_plugin_ref) {
          const auto * descr =
            inner_plugin_ref.get_object_base ()->configuration ()->descriptor ();
          z_debug ("Plugin instance ready. Importing {}", descr->name ());

          // Begin macro for undo/redo
          undo_stack_.beginMacro (
            QObject::tr ("Import %1").arg (descr->name ()));

          // Determine target group
          auto * plugin_group = [&] () -> plugins::PluginGroup * {
            structure::tracks::Track * track = nullptr;

            if (track_or_group.has_value ())
              {
                if (std::holds_alternative<plugins::PluginGroup *> (
                      track_or_group.value ()))
                  {
                    return std::get<plugins::PluginGroup *> (
                      track_or_group.value ());
                  }
                if (std::holds_alternative<structure::tracks::Track *> (
                      track_or_group.value ()))
                  {
                    track = std::get<structure::tracks::Track *> (
                      track_or_group.value ());
                  }
              }

            if (descr->isInstrument ())
              {
                auto * t =
                  track != nullptr
                    ? track
                    : track_creator_
                        .addEmptyTrackFromType (
                          structure::tracks::Track::Type::Instrument)
                        .value<structure::tracks::InstrumentTrack *> ();
                return t->channel ()->instruments ();
              }
            if (descr->isMidiModifier ())
              {
                auto * t =
                  track != nullptr
                    ? track
                    : track_creator_
                        .addEmptyTrackFromType (
                          structure::tracks::Track::Type::MidiBus)
                        .value<structure::tracks::MidiBusTrack *> ();
                return t->channel ()->midiFx ();
              }
            auto * t =
              track != nullptr
                ? track
                : track_creator_
                    .addEmptyTrackFromType (structure::tracks::Track::Type::AudioBus)
                    .value<structure::tracks::AudioBusTrack *> ();
            return t->channel ()->inserts ();
          }();

          // Add plugin to the group using AddPluginCommand
          if (plugin_group != nullptr)
            {
              undo_stack_.push (new commands::AddPluginCommand (
                *plugin_group, inner_plugin_ref));
            }

          undo_stack_.endMacro ();

          instantiation_finished_handler_ (inner_plugin_ref);
        },
      .handler_context_ = this });
}

} // namespace zrythm::actions
