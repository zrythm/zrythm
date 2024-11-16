// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/automation_track.h"
#include "common/dsp/modulator_macro_processor.h"
#include "common/dsp/modulator_track.h"
#include "common/dsp/port.h"
#include "common/dsp/router.h"
#include "common/dsp/track.h"
#include "common/utils/flags.h"
#include "common/utils/logger.h"
#include "common/utils/objects.h"
#include "common/utils/string.h"
#include "gui/backend/backend/project.h"

ModulatorTrack::ModulatorTrack (int track_pos)
    : Track (
        Track::Type::Modulator,
        QObject::tr ("Modulators").toStdString (),
        track_pos,
        PortType::Unknown,
        PortType::Unknown)
{
  main_height_ = TRACK_DEF_HEIGHT / 2;

  color_ = Color (QColor ("#222222"));
  icon_name_ = "gnome-icon-library-encoder-knob-symbolic";

  /* set invisible */
  visible_ = false;

  automation_tracklist_->setParent (this);
}

bool
ModulatorTrack::initialize ()
{

  constexpr int max_macros = 8;
  for (int i = 0; i < max_macros; i++)
    {
      modulator_macro_processors_.emplace_back (
        std::make_unique<ModulatorMacroProcessor> (this, i));
    }

  generate_automation_tracks ();

  return true;
}

void
ModulatorTrack::init_loaded ()
{
  // ChannelTrack must be initialized before AutomatableTrack
  AutomatableTrack::init_loaded ();
  ProcessableTrack::init_loaded ();
  for (auto &modulator : modulators_)
    {
      modulator->init_loaded (this, nullptr);
    }
  for (auto &macro : modulator_macro_processors_)
    {
      macro->init_loaded (*this);
    }
}

struct ModulatorImportData
{
  ModulatorTrack *             track;
  int                          slot;
  ModulatorTrack::ModulatorPtr modulator;
  bool                         replace_mode;
  bool                         confirm;
  bool                         gen_automatables;
  bool                         recalc_graph;
  bool                         pub_events;
};

static void
do_insert (ModulatorImportData * data)
{
  auto self = data->track;
  if (data->replace_mode)
    {
      zrythm::plugins::Plugin * existing_pl =
        data->slot < (int) self->modulators_.size ()
          ? self->modulators_[data->slot].get ()
          : nullptr;
      if (existing_pl)
        {
          /* free current plugin */
          if (existing_pl)
            {
              self->remove_modulator (
                data->slot, F_DELETING_PLUGIN, F_NOT_DELETING_TRACK,
                F_NO_RECALC_GRAPH);
            }
        }
    }

  /* insert the modulator */
  z_debug (
    "Inserting modulator {} at {}:{}", data->modulator->get_name (),
    self->name_, data->slot);
  auto it = self->modulators_.insert (
    self->modulators_.cbegin () + data->slot, std::move (data->modulator));
  auto &added_mod = *it;

  /* adjust affected modulators */
  for (size_t i = data->slot; i < self->modulators_.size (); ++i)
    {
      auto &mod = self->modulators_[i];
      mod->set_track_and_slot (
        self->get_name_hash (), zrythm::plugins::PluginSlotType::Modulator, i);
    }

  if (data->gen_automatables)
    {
      added_mod->generate_automation_tracks (*self);
    }

  if (data->pub_events)
    {
      // EVENTS_PUSH (EventType::ET_MODULATOR_ADDED, added_mod.get ());
    }

  if (data->recalc_graph)
    {
      ROUTER->recalc_graph (false);
    }
}
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

template <typename T>
std::shared_ptr<T>
ModulatorTrack::insert_modulator (
  int                slot,
  std::shared_ptr<T> modulator,
  bool               replace_mode,
  bool               confirm,
  bool               gen_automatables,
  bool               recalc_graph,
  bool               pub_events)
{
  z_return_val_if_fail (slot <= (int) modulators_.size (), nullptr);

  ModulatorImportData data{};
  data.track = this;
  data.slot = slot;
  data.modulator = modulator;
  data.replace_mode = replace_mode;
  data.confirm = confirm;
  data.gen_automatables = gen_automatables;
  data.recalc_graph = recalc_graph;
  data.pub_events = pub_events;

  if (replace_mode)
    {
      auto existing_pl =
        slot < modulators_.empty () ? nullptr : modulators_[slot].get ();
      if (existing_pl && confirm)
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

  do_insert (&data);
  return modulator;
}

ModulatorTrack::ModulatorPtr
ModulatorTrack::remove_modulator (
  int  slot,
  bool deleting_modulator,
  bool deleting_track,
  bool recalc_graph)
{
  auto plugin = modulators_[slot];
  z_return_val_if_fail (
    plugin->id_.track_name_hash_ == get_name_hash (), nullptr);

  plugin->remove_ats_from_automation_tracklist (
    deleting_modulator, !deleting_track && !deleting_modulator);

  z_debug ("Removing {} from {}:{}", plugin->get_name (), name_, slot);

  /* unexpose all JACK ports */
  plugin->expose_ports (*AUDIO_ENGINE, F_NOT_EXPOSE, true, true);

  /* if deleting plugin disconnect the plugin entirely */
  if (deleting_modulator)
    {
      if (plugin->is_selected ())
        {
          MIXER_SELECTIONS->remove_slot (
            plugin->id_.slot_, zrythm::plugins::PluginSlotType::Modulator, true);
        }

      plugin->disconnect ();
    }

  auto it = modulators_.erase (modulators_.begin () + slot);
  for (; it != modulators_.end (); ++it)
    {
      auto &mod = *it;
      mod->set_track_and_slot (
        get_name_hash (), zrythm::plugins::PluginSlotType::Modulator,
        std::distance (modulators_.begin (), it));
    }

  if (recalc_graph)
    {
      ROUTER->recalc_graph (false);
    }

  return *it;
}

void
ModulatorTrack::init_after_cloning (const ModulatorTrack &other)
{
  ProcessableTrack::copy_members_from (other);
  AutomatableTrack::copy_members_from (other);
  Track::copy_members_from (other);
  for (auto &mod : other.modulators_)
    {
      modulators_.emplace_back (
        clone_shared_with_variant<zrythm::plugins::PluginVariant> (mod.get ()));
    }
  clone_unique_ptr_container (
    modulator_macro_processors_, other.modulator_macro_processors_);
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

  for (const auto &modulator : modulators_)
    modulator->append_ports (ports);
  for (const auto &macro : modulator_macro_processors_)
    {
      add_port (macro->macro_.get ());
      add_port (macro->cv_in_.get ());
      add_port (macro->cv_out_.get ());
    }
}

bool
ModulatorTrack::validate () const
{
  return Track::validate_base () && AutomatableTrack::validate_base ();
}

template std::shared_ptr<zrythm::plugins::Plugin>
ModulatorTrack::insert_modulator (
  int                                      slot,
  std::shared_ptr<zrythm::plugins::Plugin> modulator,
  bool                                     replace_mode,
  bool                                     confirm,
  bool                                     gen_automatables,
  bool                                     recalc_graph,
  bool                                     pub_events);
template std::shared_ptr<zrythm::plugins::CarlaNativePlugin>
ModulatorTrack::insert_modulator (
  int                                                 slot,
  std::shared_ptr<zrythm::plugins::CarlaNativePlugin> modulator,
  bool                                                replace_mode,
  bool                                                confirm,
  bool                                                gen_automatables,
  bool                                                recalc_graph,
  bool                                                pub_events);