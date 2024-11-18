// SPDX-FileCopyrightText: © 2021-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/audio_bus_track.h"
#include "common/dsp/channel_track.h"
#include "common/dsp/tracklist.h"
#include "common/plugins/plugin_descriptor.h"
#include "common/utils/directory_manager.h"
#include "common/utils/gtest_wrapper.h"
#include "common/utils/io.h"
#include "common/utils/string.h"
#include "gui/backend/backend/actions/port_connection_action.h"
#include "gui/backend/backend/actions/tracklist_selections_action.h"
#include "gui/backend/backend/settings/plugin_settings.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/zrythm.h"

using namespace zrythm;

constexpr const char * PLUGIN_SETTINGS_JSON_FILENAME = "plugin-settings.json";

void
PluginSetting::copy_fields_from (const PluginSetting &other)
{
  descr_ = other.descr_->clone_unique ();
  hosting_type_ = other.hosting_type_;
  open_with_carla_ = other.open_with_carla_;
  force_generic_ui_ = other.force_generic_ui_;
  bridge_mode_ = other.bridge_mode_;
  last_instantiated_time_ = other.last_instantiated_time_;
  num_instantiations_ = other.num_instantiations_;
}

void
PluginSetting::init_after_cloning (const PluginSetting &other)
{
  copy_fields_from (other);
}

PluginSetting::PluginSetting (const zrythm::plugins::PluginDescriptor &descr)
{
  PluginSetting * existing = nullptr;
  if (S_PLUGIN_SETTINGS && !ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      existing = S_PLUGIN_SETTINGS->find (descr);
    }

  if (existing)
    {
      copy_fields_from (*existing);
      validate (false);
    }
  else
    {
      descr_ = descr.clone_unique ();
      /* bridge all plugins by default */
      this->bridge_mode_ = zrythm::plugins::CarlaBridgeMode::Full;
      validate (false);
    }
}

void
PluginSetting::print () const
{
  z_debug (
    "[PluginSetting]\n"
    "descr.uri={}, "
    "open_with_carla={}, "
    "force_generic_ui={}, "
    "bridge_mode={}, "
    "last_instantiated_time={}, "
    "num_instantiations={}",
    this->descr_->uri_, this->open_with_carla_, this->force_generic_ui_,
    ENUM_NAME (this->bridge_mode_), this->last_instantiated_time_,
    this->num_instantiations_);
}

void
PluginSetting::validate (bool print_result)
{
  /* force carla */
  this->open_with_carla_ = true;

#if !HAVE_CARLA
  if (this->open_with_carla_)
    {
      z_error (
        "Requested to open with carla - carla "
        "functionality is disabled");
      this->open_with_carla_ = false;
      return;
    }
#endif

  const auto prot = get_descriptor ()->protocol_;
  if (
    prot == zrythm::plugins::Protocol::ProtocolType::VST
    || prot == zrythm::plugins::Protocol::ProtocolType::VST3
    || prot == zrythm::plugins::Protocol::ProtocolType::AudioUnit
    || prot == zrythm::plugins::Protocol::ProtocolType::SFZ
    || prot == zrythm::plugins::Protocol::ProtocolType::SF2
    || prot == zrythm::plugins::Protocol::ProtocolType::DSSI
    || prot == zrythm::plugins::Protocol::ProtocolType::LADSPA
    || prot == zrythm::plugins::Protocol::ProtocolType::JSFX
    || prot == zrythm::plugins::Protocol::ProtocolType::CLAP)
    {
      this->open_with_carla_ = true;
#if !HAVE_CARLA
      z_error (
        "Invalid setting requested: open non-LV2 "
        "plugin, carla not installed");
#endif
    }

#if defined(_WIN32) && defined(HAVE_CARLA)
  /* open all LV2 plugins with custom UIs using carla */
  if (descr_.has_custom_ui_ && !force_generic_ui_)
    {
      open_with_carla_ = true;
    }
#endif

#if HAVE_CARLA
  /* in wayland open all LV2 plugins with custom UIs using carla */
  if (z_gtk_is_wayland () && get_descriptor ()->has_custom_ui_)
    {
      open_with_carla_ = true;
    }
#endif

#if HAVE_CARLA
  /* if no bridge mode specified, calculate the bridge mode here */
  /*z_debug ("{}: recalculating bridge mode...", __func__);*/
  if (this->bridge_mode_ == zrythm::plugins::CarlaBridgeMode::None)
    {
      this->bridge_mode_ = get_descriptor ()->min_bridge_mode_;
      if (this->bridge_mode_ == zrythm::plugins::CarlaBridgeMode::None)
        {
#  if 0
          /* bridge if plugin is not whitelisted */
          if (!plugin_descriptor_is_whitelisted (descr))
            {
              this->open_with_carla_ = true;
              this->bridge_mode_ = zrythm::plugins::CarlaBridgeMode::Full;
              /*z_debug (*/
              /*"plugin descriptor not whitelisted - will bridge full");*/
            }
#  endif
        }
      else
        {
          this->open_with_carla_ = true;
        }
    }
  else
    {
      this->open_with_carla_ = true;
      zrythm::plugins::CarlaBridgeMode mode = descr_->min_bridge_mode_;

      if (mode == zrythm::plugins::CarlaBridgeMode::Full)
        {
          this->bridge_mode_ = mode;
        }
    }

#  if 0
  /* force bridge mode */
  if (this->bridge_mode_ == zrythm::plugins::CarlaBridgeMode::None)
    {
      this->bridge_mode_ = zrythm::plugins::CarlaBridgeMode::Full;
    }
#  endif

    /*z_debug ("done recalculating bridge mode");*/
#endif

  /* if no custom UI, force generic */
  /*z_debug ("checking if plugin has custom UI...");*/
  if (!descr_->has_custom_ui_)
    {
      /*z_debug ("plugin {} has no custom UI", descr->name);*/
      this->force_generic_ui_ = true;
    }
  /*z_debug ("done checking if plugin has custom UI");*/

  if (print_result)
    {
      z_debug ("plugin setting validated. new setting:");
      print ();
    }
}

void
PluginSetting::activate_finish (bool autoroute_multiout, bool has_stereo_outputs)
  const
{
  bool has_errors = false;

  auto type = Track::type_get_from_plugin_descriptor (*descr_);

  /* stop the engine so it doesn't restart all the time until all the actions
   * are performed */
  AudioEngine::State state{};
  AUDIO_ENGINE->wait_for_pause (state, false, true);

  try
    {
      if (autoroute_multiout)
        {
          int num_pairs = descr_->num_audio_outs_;
          if (has_stereo_outputs)
            num_pairs = num_pairs / 2;
          int num_actions = 0;

          /* create group */
          Track * group =
            Track::create_empty_with_action (Track::Type::AudioGroup);
          num_actions++;

          /* create the plugin track */
          auto * pl_track = dynamic_cast<ChannelTrack *> (
            Track::create_for_plugin_at_idx_w_action (
              type, this, TRACKLIST->tracks_.size ()));
          num_actions++;

          auto &pl = pl_track->channel_->instrument_;

          /* move the plugin track inside the group */
          pl_track->select (true, true, false);
          UNDO_MANAGER->perform (
            new gui::actions::MoveTracksInsideFoldableTrackAction (
              *TRACKLIST_SELECTIONS->gen_tracklist_selections (), group->pos_));
          num_actions++;

          /* route to nowhere */
          pl_track->select (true, true, false);
          UNDO_MANAGER->perform (new gui::actions::RemoveTracksDirectOutAction (
            *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
            *PORT_CONNECTIONS_MGR));
          num_actions++;

          /* rename group */
          auto name = format_str (
            QObject::tr ("{} Output").toStdString (), descr_->name_);
          UNDO_MANAGER->perform (new gui::actions::RenameTrackAction (
            *group, *PORT_CONNECTIONS_MGR, name));
          num_actions++;

          std::vector<AudioPort *> pl_audio_outs;
          for (auto * cur_port : pl->out_ports_ | type_is<AudioPort> ())
            {
              pl_audio_outs.push_back (cur_port);
            }

          for (int i = 0; i < num_pairs; i++)
            {
              /* create the audio fx track */
              auto * fx_track = dynamic_cast<AudioBusTrack *> (
                Track::create_empty_with_action (Track::Type::AudioBus));
              num_actions++;

              /* rename fx track */
              name = format_str ("{} {}", descr_->name_, i + 1);
              UNDO_MANAGER->perform (new gui::actions::RenameTrackAction (
                *fx_track, *PORT_CONNECTIONS_MGR, name));
              num_actions++;

              /* move the fx track inside the group */
              fx_track->select (true, true, false);
              UNDO_MANAGER->perform (
                new gui::actions::MoveTracksInsideFoldableTrackAction (
                  *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
                  group->pos_));
              num_actions++;

              /* move the fx track to the end */
              fx_track->select (true, true, false);
              UNDO_MANAGER->perform (new gui::actions::MoveTracksAction (
                *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
                TRACKLIST->tracks_.size ()));
              num_actions++;

              /* route to group */
              fx_track->select (true, true, false);
              UNDO_MANAGER->perform (
                new gui::actions::ChangeTracksDirectOutAction (
                  *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
                  *PORT_CONNECTIONS_MGR, *group));
              num_actions++;

              int  l_index = has_stereo_outputs ? i * 2 : i;
              auto port = pl_audio_outs[l_index];

              /* route left port to audio fx */
              UNDO_MANAGER->perform (
                new gui::actions::PortConnectionConnectAction (
                  *port->id_, *fx_track->processor_->stereo_in_->get_l ().id_));
              num_actions++;

              int r_index = has_stereo_outputs ? i * 2 + 1 : i;
              port = pl_audio_outs[r_index];

              /* route right port to audio fx */
              UNDO_MANAGER->perform (
                new gui::actions::PortConnectionConnectAction (
                  *port->id_, *fx_track->processor_->stereo_in_->get_r ().id_));
              num_actions++;
            }

          auto ua_opt = UNDO_MANAGER->get_last_action ();
          z_return_if_fail (ua_opt.has_value ());
          std::visit (
            [&] (auto &&ua) { ua->num_actions_ = num_actions; },
            ua_opt.value ());
        }
      else /* else if not autoroute multiout */
        {
          Track::create_for_plugin_at_idx_w_action (
            type, this, TRACKLIST->tracks_.size ());
        }
    }
  catch (const ZrythmException &e)
    {
      e.handle (QObject::tr ("Failed to instantiate plugin"));
      has_errors = true;
    }

  AUDIO_ENGINE->resume (state);

  if (!has_errors)
    {
      auto setting_clone = clone_unique ();
      setting_clone->increment_num_instantiations ();
    }
}

#if 0
static void
on_outputs_stereo_response (
  AdwMessageDialog * dialog,
  char *             response,
  PluginSetting *    self)
{
  if (string_is_equal (response, "close"))
    return;

  bool stereo = string_is_equal (response, "yes");
  self->activate_finish (true, stereo);
}

static void
on_contains_multiple_outputs_response (
  AdwMessageDialog * dialog,
  char *             response,
  PluginSetting *    self)
{
  if (string_is_equal (response, "yes"))
    {
      GtkWidget * stereo_dialog = adw_message_dialog_new (
        GTK_WINDOW (MAIN_WINDOW), QObject::tr ("Stereo?"), QObject::tr ("Are the outputs stereo?"));
      gtk_window_set_modal (GTK_WINDOW (stereo_dialog), true);
      adw_message_dialog_add_responses (
        ADW_MESSAGE_DIALOG (stereo_dialog), "yes", QObject::tr ("_Yes"), "no", QObject::tr ("_No"),
        nullptr);
      adw_message_dialog_set_default_response (
        ADW_MESSAGE_DIALOG (stereo_dialog), "yes");
      auto * setting_clone = self->clone_raw_ptr ();
      g_signal_connect_data (
        stereo_dialog, "response", G_CALLBACK (on_outputs_stereo_response),
        setting_clone, PluginSetting::free_closure, (GConnectFlags) 0);
      gtk_window_present (GTK_WINDOW (stereo_dialog));
    }
  else if (string_is_equal (response, "no"))
    {
      self->activate_finish (false, false);
    }
  else
    {
      /* do nothing */
    }
}
#endif

void
PluginSetting::activate () const
{
  Track::Type type = Track::type_get_from_plugin_descriptor (*descr_);

  if (descr_->num_audio_outs_ > 2 && type == Track::Type::Instrument)
    {
#if 0
      AdwMessageDialog * dialog = ADW_MESSAGE_DIALOG (adw_message_dialog_new (
        GTK_WINDOW (MAIN_WINDOW), QObject::tr ("Auto-route?"),
        _ (
          "This plugin contains multiple audio outputs. Would you like to auto-route each output to a separate FX track?")));
      adw_message_dialog_add_responses (
        dialog, "cancel", QObject::tr ("_Cancel"), "no", QObject::tr ("_No"), "yes", QObject::tr ("_Yes"),
        nullptr);
      adw_message_dialog_set_close_response (dialog, "cancel");
      adw_message_dialog_set_response_appearance (
        dialog, "yes", ADW_RESPONSE_SUGGESTED);
      auto * setting_clone = clone_raw_ptr ();
      g_signal_connect_data (
        dialog, "response", G_CALLBACK (on_contains_multiple_outputs_response),
        setting_clone, PluginSetting::free_closure, (GConnectFlags) 0);
      gtk_window_present (GTK_WINDOW (dialog));
#endif
    }
  else
    {
      activate_finish (false, false);
    }
}

void
PluginSetting::increment_num_instantiations ()
{
  last_instantiated_time_ = QDateTime::currentMSecsSinceEpoch ();
  ++num_instantiations_;

  S_PLUGIN_SETTINGS->set (*this, true);
}

fs::path
PluginSettings::get_file_path ()
{
  auto * dir_mgr = DirectoryManager::getInstance ();
  auto zrythm_dir = dir_mgr->get_dir (DirectoryManager::DirectoryType::USER_TOP);
  z_return_val_if_fail (!zrythm_dir.empty (), "");

  return fs::path (zrythm_dir) / PLUGIN_SETTINGS_JSON_FILENAME;
}

void
PluginSettings::serialize_to_file ()
{
  z_info ("Serializing plugin settings...");

  auto json_str = serialize_to_json_string ();

  auto path = get_file_path ();
  z_return_if_fail (
    !path.empty () && path.is_absolute () && path.has_parent_path ());

  z_debug ("Writing plugin settings to {}...", path);
  try
    {
      utils::io::set_file_contents (path, json_str.c_str ());
    }
  catch (const ZrythmException &e)
    {
      throw ZrythmException (
        format_str ("Unable to write plugin settings: {}", e.what ()));
    }
}

std::unique_ptr<PluginSettings>
PluginSettings::read_or_new ()
{
  auto ret = std::make_unique<PluginSettings> ();
  auto path = get_file_path ();
  if (!fs::exists (path))
    {
      z_info ("Plugin settings file at {} does not exist", path);
      return ret;
    }

  std::string json;
  try
    {
      json = utils::io::read_file_contents (path).toStdString ();
    }
  catch (const ZrythmException &e)
    {
      z_warning ("Failed to create plugin settings from {}", path);
      return ret;
    }

  try
    {
      ret->deserialize_from_json_string (json.c_str ());
    }
  catch (const ZrythmException &e)
    {
      z_warning (
        "Found invalid plugin settings file (error: {}). "
        "Purging file and creating a new one.",
        e.what ());

      try
        {
          delete_file ();
        }
      catch (const ZrythmException &e2)
        {
          z_warning (e2.what ());
        }

      return {};
    }

// FIXME: do this in the deserialize override of zrythm::plugins::PluginDescriptor
#if 0
for (size_t i = 0; i < self->descriptors->len; i++)
  {
    zrythm::plugins::PluginDescriptor * descr =
      (zrythm::plugins::PluginDescriptor *) g_ptr_array_index (self->descriptors, i);
    descr->category = plugin_descriptor_string_to_category (descr->category_str);
  }
#endif

  return ret;
}

void
PluginSettings::delete_file ()
{
  auto path = get_file_path ();
  z_return_if_fail (
    !path.empty () && path.is_absolute () && path.has_parent_path ());
  try
    {
      fs::remove (path);
    }
  catch (const fs::filesystem_error &e)
    {
      throw ZrythmException (format_str (
        "Failed to remove invalid plugin settings file: {}", e.what ()));
    }
}

PluginSetting *
PluginSettings::find (const zrythm::plugins::PluginDescriptor &descr)
{
  auto it = std::find_if (
    settings_.begin (), settings_.end (),
    [&descr] (const auto &s) { return s->descr_->is_same_plugin (descr); });
  if (it == settings_.end ())
    {
      return nullptr;
    }
  return (*it).get ();
}

void
PluginSettings::set (const PluginSetting &setting, bool _serialize)
{
  z_debug ("Saving plugin setting for {}", setting.descr_->name_);

  PluginSetting * own_setting = find (*setting.descr_);

  if (own_setting)
    {
      own_setting->force_generic_ui_ = setting.force_generic_ui_;
      own_setting->open_with_carla_ = setting.open_with_carla_;
      own_setting->bridge_mode_ = setting.bridge_mode_;
      own_setting->last_instantiated_time_ = setting.last_instantiated_time_;
      own_setting->num_instantiations_ = setting.num_instantiations_;
    }
  else
    {
      auto new_setting = setting.clone_unique ();
      new_setting->validate ();
      settings_.emplace_back (std::move (new_setting));
    }

  if (_serialize)
    {
      serialize_to_file_no_throw ();
    }
}

std::unique_ptr<zrythm::plugins::Plugin>
PluginSetting::create_plugin (
  unsigned int                    track_name_hash,
  zrythm::plugins::PluginSlotType slot_type,
  int                             slot)
{
  auto plugin = std::make_unique<zrythm::plugins::CarlaNativePlugin> (
    *this->descr_, track_name_hash, slot_type, slot);
  return plugin;
}