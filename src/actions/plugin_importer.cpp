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
PluginImporter::importPlugin (const plugins::PluginDescriptor * descriptor)
{
  if (descriptor == nullptr)
    {
      z_warning ("Cannot import null plugin descriptor");
      return;
    }

  z_debug ("Importing plugin: {}", descriptor->getName ());

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
        [this] (plugins::PluginUuidReference inner_plugin_ref) {
          const auto * descr =
            inner_plugin_ref.get_object_base ()->configuration ()->descriptor ();
          z_debug ("Plugin instance ready. Importing {}", descr->getName ());

          // Begin macro for undo/redo
          undo_stack_.beginMacro (
            QObject::tr ("Import %1").arg (descr->getName ()));

          // Determine target track based on plugin type
          const auto &[_, plugin_group] = [&] ()
            -> std::pair<structure::tracks::Track *, plugins::PluginGroup *> {
            if (descr->is_instrument ())
              {
                auto * t =
                  track_creator_
                    .addEmptyTrackFromType (
                      structure::tracks::Track::Type::Instrument)
                    .value<structure::tracks::InstrumentTrack *> ();
                return std::make_pair (t, t->channel ()->instruments ());
              }
            if (descr->is_midi_modifier ())
              {
                auto * t =
                  track_creator_
                    .addEmptyTrackFromType (
                      structure::tracks::Track::Type::MidiBus)
                    .value<structure::tracks::MidiBusTrack *> ();
                return std::make_pair (t, t->channel ()->midiFx ());
              }
            auto * t =
              track_creator_
                .addEmptyTrackFromType (structure::tracks::Track::Type::AudioBus)
                .value<structure::tracks::AudioBusTrack *> ();
            return std::make_pair (t, t->channel ()->inserts ());
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
