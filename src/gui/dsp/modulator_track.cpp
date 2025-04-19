// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "gui/backend/backend/project.h"
#include "gui/dsp/automation_track.h"
#include "gui/dsp/modulator_macro_processor.h"
#include "gui/dsp/modulator_track.h"
#include "gui/dsp/port.h"
#include "gui/dsp/router.h"
#include "gui/dsp/track.h"
#include "utils/logger.h"

ModulatorTrack::ModulatorTrack (
  TrackRegistry          &track_registry,
  PluginRegistry         &plugin_registry,
  PortRegistry           &port_registry,
  ArrangerObjectRegistry &obj_registry,
  bool                    new_identity)
    : Track (
        Track::Type::Modulator,
        PortType::Unknown,
        PortType::Unknown,
        plugin_registry,
        port_registry,
        obj_registry),
      AutomatableTrack (port_registry, new_identity),
      ProcessableTrack (port_registry, new_identity)
{
  if (new_identity)
    {
      main_height_ = DEF_HEIGHT / 2;

      color_ = Color (QColor ("#222222"));
      icon_name_ = "gnome-icon-library-encoder-knob-symbolic";

      /* set invisible */
      visible_ = false;
    }

  automation_tracklist_->setParent (this);
}

bool
ModulatorTrack::initialize ()
{

  constexpr int max_macros = 8;
  for (int i = 0; i < max_macros; i++)
    {
      modulator_macro_processors_.emplace_back (
        std::make_unique<ModulatorMacroProcessor> (
          port_registry_, this, i, true));
    }

  generate_automation_tracks ();

  return true;
}

void
ModulatorTrack::init_loaded (
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry)
{
  // ChannelTrack must be initialized before AutomatableTrack
  AutomatableTrack::init_loaded (plugin_registry, port_registry);
  ProcessableTrack::init_loaded (plugin_registry, port_registry);
  for (auto &modulator_id : modulators_)
    {
      auto modulator = modulator_id.get_object ();
      std::visit ([&] (auto &&pl) { pl->init_loaded (this); }, modulator);
    }
  for (auto &macro : modulator_macro_processors_)
    {
      macro->init_loaded (*this);
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
            self->remove_modulator (this->slot);
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
            mod->generate_automation_tracks (*self);
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
  dsp::PluginSlot::SlotNo slot,
  PluginUuidReference     modulator_id,
  bool                    replace_mode,
  bool                    confirm,
  bool                    gen_automatables,
  bool                    recalc_graph,
  bool                    pub_events)
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

ModulatorTrack::PluginPtrVariant
ModulatorTrack::remove_modulator (dsp::PluginSlot::SlotNo slot)
{
  auto plugin_id = modulators_[slot];
  auto plugin_var = plugin_id.get_object ();
  return std::visit (
    [&] (auto &&plugin) -> PluginPtrVariant {
      assert (plugin->get_track_id () == get_uuid ());

      plugin->remove_ats_from_automation_tracklist (true, !false && !true);

      z_debug ("Removing {} from {}:{}", plugin->get_name (), name_, slot);

      /* unexpose all JACK ports */
      plugin->expose_ports (*AUDIO_ENGINE, false, true, true);

      /* if deleting plugin disconnect the plugin entirely */
      plugin->set_selected (false);
      plugin->Plugin::disconnect ();

      auto it = modulators_.erase (modulators_.begin () + slot);
      for (; it != modulators_.end (); ++it)
        {
          const auto &mod_id = *it;
          auto        mod_var = mod_id.get_object ();
          std::visit (
            [&] (auto &&mod) { mod->set_track (get_uuid ()); }, mod_var);
        }

      auto ret_id = *it;
      return ret_id.get_object ();
    },
    plugin_var);
}

void
ModulatorTrack::init_after_cloning (
  const ModulatorTrack &other,
  ObjectCloneType       clone_type)
{
  ProcessableTrack::copy_members_from (other, clone_type);
  AutomatableTrack::copy_members_from (other, clone_type);
  Track::copy_members_from (other, clone_type);
  if (clone_type == ObjectCloneType::NewIdentity)
    {
      const auto clone_from_registry = [] (auto &vec, const auto &other_vec) {
        for (const auto &other_el : other_vec)
          {
            vec.push_back (other_el.clone_new_identity ());
          }
      };

      clone_from_registry (modulators_, other.modulators_);
    }
  else if (clone_type == ObjectCloneType::Snapshot)
    {
      modulators_ = other.modulators_;
    }
  clone_unique_ptr_container (
    modulator_macro_processors_, other.modulator_macro_processors_);
}

std::optional<ModulatorTrack::PluginPtrVariant>
ModulatorTrack::get_modulator (dsp::PluginSlot::SlotNo slot) const
{
  auto modulator_var = modulators_[slot].get_object ();
  return modulator_var;
}

dsp::PluginSlot
ModulatorTrack::get_plugin_slot (const PluginUuid &plugin_id) const
{
  const auto it =
    std::ranges::find (modulators_, plugin_id, &PluginUuidReference::id);
  if (it == modulators_.end ())
    throw std::runtime_error ("Plugin not found");
  return dsp::PluginSlot (
    dsp::PluginSlotType::Modulator, std::distance (modulators_.begin (), it));
}

void
ModulatorTrack::append_ports (std::vector<Port *> &ports, bool include_plugins)
  const
{
  ProcessableTrack::append_member_ports (ports, include_plugins);
  auto add_port = [&ports] (Port * port) {
    if (port != nullptr)
      ports.push_back (port);
    else
      z_warning ("Port is null");
  };

  for (const auto &modulator_id : modulators_)
    {
      auto modulator_var = modulator_id.get_object ();
      std::visit (
        [&] (auto &&modulator) { modulator->append_ports (ports); },
        modulator_var);
    }
  for (const auto &macro : modulator_macro_processors_)
    {
      add_port (&macro->get_macro_port ());
      add_port (&macro->get_cv_in_port ());
      add_port (&macro->get_cv_out_port ());
    }
}

bool
ModulatorTrack::validate () const
{
  return Track::validate_base () && AutomatableTrack::validate_base ();
}
