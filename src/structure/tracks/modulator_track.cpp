// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "engine/session/router.h"
#include "gui/backend/backend/project.h"
#include "structure/tracks/automation_track.h"
#include "structure/tracks/modulator_track.h"
#include "structure/tracks/track.h"
#include "utils/logger.h"

namespace zrythm::structure::tracks
{
ModulatorTrack::ModulatorTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::Modulator,
        PortType::Unknown,
        PortType::Unknown,
        dependencies.plugin_registry_,
        dependencies.port_registry_,
        dependencies.param_registry_,
        dependencies.obj_registry_),
      ProcessableTrack (
        Dependencies{
          dependencies.tempo_map_, dependencies.file_audio_source_registry_,
          dependencies.port_registry_, dependencies.param_registry_,
          dependencies.obj_registry_ })
{
  main_height_ = DEF_HEIGHT / 2;

  color_ = Color (QColor ("#222222"));
  icon_name_ = u8"gnome-icon-library-encoder-knob-symbolic";

  /* set invisible */
  visible_ = false;

  automatableTrackMixin ()->setParent (this);
}

bool
ModulatorTrack::initialize ()
{
  constexpr int max_macros = 8;
  for (int i = 0; i < max_macros; i++)
    {
      modulator_macro_processors_.emplace_back (
        utils::make_qobject_unique<dsp::ModulatorMacroProcessor> (
          dsp::ModulatorMacroProcessor::ProcessorBaseDependencies{
            .port_registry_ = port_registry_, .param_registry_ = param_registry_ },
          i, this));
    }

  generate_automation_tracks (*this);

  return true;
}

void
ModulatorTrack::init_loaded (
  PluginRegistry                  &plugin_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry)
{
  // ChannelTrack must be initialized before AutomatableTrack
  ProcessableTrack::init_loaded (plugin_registry, port_registry, param_registry);
  for (auto &modulator_id : modulators_)
    {
      auto modulator = modulator_id.get_object ();
      std::visit ([&] (auto &&pl) { pl->init_loaded (); }, modulator);
    }
}

struct ModulatorImportData
{
  ModulatorImportData (PluginUuidReference ref) : modulator_id (std::move (ref))
  {
  }

  ModulatorTrack *    track{};
  int                 slot{};
  PluginUuidReference modulator_id;
  bool                replace_mode{};
  bool                confirm{};
  bool                gen_automatables{};
  bool                recalc_graph{};
  bool                pub_events{};

  void do_insert ()
  {
    auto self = this->track;
    if (this->replace_mode)
      {
        // remove the existing modulator
        if (this->slot < (decltype (this->slot)) self->modulators_.size ())
          {
            auto existing_id = self->modulators_.at (this->slot);
            self->remove_plugin (
              plugins::PluginSlot{
                plugins::PluginSlotType::Modulator,
                static_cast<plugins::PluginSlot::SlotNo> (this->slot) });
          }
      }

    std::visit (
      [&] (auto &&mod) {
        /* insert the new modulator */
        z_debug (
          "Inserting modulator {} at {}:{}", mod->get_name (), self->name_,
          this->slot);
        self->plugin_registry_.register_object (mod);
        self->modulators_.insert (
          self->modulators_.cbegin () + this->slot, modulator_id);

        /* adjust affected modulators */
        for (size_t i = this->slot; i < self->modulators_.size (); ++i)
          {
            const auto &cur_mod_id = self->modulators_[i];
            auto        cur_mod = cur_mod_id.get_object ();
            std::visit (
              [&] (auto &&cur_mod_ptr) {
                cur_mod_ptr->set_track (self->get_uuid ());
              },
              cur_mod);
          }

        if (this->gen_automatables)
          {
            // TODO
            // self->generate_automation_tracks_for_processor (mod->get_uuid ());
          }

        if (this->pub_events)
          {
            // EVENTS_PUSH (EventType::ET_MODULATOR_ADDED, added_mod.get ());
          }

        if (this->recalc_graph)
          {
            ROUTER->recalc_graph (false);
          }
      },
      modulator_id.get_object ());
  }
};

#if 0
[[maybe_unused]] static void
overwrite_plugin_response_cb (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           user_data)
{
  ModulatorImportData * data = (ModulatorImportData *) user_data;
  if (!string_is_equal (response, "overwrite"))
    {
      return;
    }

  do_insert (data);
}
#endif

ModulatorTrack::PluginPtrVariant
ModulatorTrack::insert_modulator (
  plugins::PluginSlot::SlotNo slot,
  PluginUuidReference         modulator_id,
  bool                        replace_mode,
  bool                        confirm,
  bool                        gen_automatables,
  bool                        recalc_graph,
  bool                        pub_events)
{
  z_return_val_if_fail (slot <= (int) modulators_.size (), nullptr);

  ModulatorImportData data{ modulator_id };
  data.track = this;
  data.slot = slot;
  data.replace_mode = replace_mode;
  data.confirm = confirm;
  data.gen_automatables = gen_automatables;
  data.recalc_graph = recalc_graph;
  data.pub_events = pub_events;

  return std::visit (
    [&] (auto &&modulator) {
      if (replace_mode)
        {
          if (slot < modulators_.size () && confirm)
            {
#if 0
          AdwMessageDialog * dialog =
            dialogs_get_overwrite_plugin_dialog (GTK_WINDOW (MAIN_WINDOW));
          gtk_window_present (GTK_WINDOW (dialog));
          g_signal_connect_data (
            dialog, "response", G_CALLBACK (overwrite_plugin_response_cb),
            new ModulatorImportData (std::move (data)),
            ModulatorImportData::free, G_CONNECT_DEFAULT);
#endif
              return modulator;
            }
        }

      data.do_insert ();
      return modulator;
    },
    modulator_id.get_object ());
}

void
init_from (
  ModulatorTrack        &obj,
  const ModulatorTrack  &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<ProcessableTrack &> (obj),
    static_cast<const ProcessableTrack &> (other), clone_type);
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
// TODO
#if 0
    if (clone_type == utils::ObjectCloneType::NewIdentity)
    {
      const auto clone_from_registry = [] (auto &vec, const auto &other_vec) {
        for (const auto &other_el : other_vec)
          {
            vec.push_back (other_el.clone_new_identity ());
          }
      };

      clone_from_registry (obj.modulators_, other.modulators_);
    }
  else if (clone_type == utils::ObjectCloneType::Snapshot)
    {
      obj.modulators_ = other.modulators_;
    }
#endif
  obj.modulators_ = other.modulators_;
  utils::clone_unique_ptr_container (
    obj.modulator_macro_processors_, other.modulator_macro_processors_);
}

std::optional<ModulatorTrack::PluginPtrVariant>
ModulatorTrack::get_modulator (plugins::PluginSlot::SlotNo slot) const
{
  auto modulator_var = modulators_[slot].get_object ();
  return modulator_var;
}

plugins::PluginSlot
ModulatorTrack::get_plugin_slot (const PluginUuid &plugin_id) const
{
  const auto it =
    std::ranges::find (modulators_, plugin_id, &PluginUuidReference::id);
  if (it == modulators_.end ())
    throw std::runtime_error ("Plugin not found");
  return plugins::PluginSlot (
    plugins::PluginSlotType::Modulator,
    std::distance (modulators_.begin (), it));
}

void
from_json (const nlohmann::json &j, ModulatorTrack &track)
{
  from_json (j, static_cast<Track &> (track));
  from_json (j, static_cast<ProcessableTrack &> (track));
  for (const auto &modulator_json : j.at (ModulatorTrack::kModulatorsKey))
    {
      PluginUuidReference modulator_id_ref{ track.plugin_registry_ };
      from_json (modulator_json, modulator_id_ref);
      track.modulators_.push_back (std::move (modulator_id_ref));
    }
  for (
    const auto &[index, macro_proc_json] : utils::views::enumerate (
      j.at (ModulatorTrack::kModulatorMacroProcessorsKey)))
    {
      auto macro_proc = utils::make_qobject_unique<dsp::ModulatorMacroProcessor> (
        dsp::ModulatorMacroProcessor::ProcessorBaseDependencies{
          .port_registry_ = track.port_registry_,
          .param_registry_ = track.param_registry_ },
        index, &track);
      from_json (macro_proc_json, *macro_proc);
      track.modulator_macro_processors_.push_back (std::move (macro_proc));
    }
}
}
