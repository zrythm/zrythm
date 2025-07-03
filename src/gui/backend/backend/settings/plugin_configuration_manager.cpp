// SPDX-FileCopyrightText: © 2021-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/settings/plugin_configuration_manager.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/zrythm_application.h"
#include "plugins/plugin_descriptor.h"
#include "utils/directory_manager.h"
#include "utils/gtest_wrapper.h"
#include "utils/io.h"
#include "utils/string.h"

using namespace zrythm;

constexpr auto PLUGIN_SETTINGS_JSON_FILENAME = "plugin-settings.json"sv;

auto
PluginConfigurationManager::create_configuration_for_descriptor (
  const plugins::PluginDescriptor &descr) -> std::unique_ptr<PluginConfiguration>
{
  auto * existing = find (descr);
  if (existing != nullptr)
    {
      auto ret = std::make_unique<PluginConfiguration> ();
      ret->copy_fields_from (*existing);
      ret->validate ();
      return ret;
    }

  return PluginConfiguration::create_new_for_descriptor (descr);
}

fs::path
PluginConfigurationManager::get_file_path ()
{
  auto &dir_mgr =
    dynamic_cast<gui::ZrythmApplication *> (qApp)->get_directory_manager ();
  auto zrythm_dir = dir_mgr.get_dir (DirectoryManager::DirectoryType::USER_TOP);
  z_return_val_if_fail (!zrythm_dir.empty (), "");

  return fs::path (zrythm_dir) / PLUGIN_SETTINGS_JSON_FILENAME;
}

void
PluginConfigurationManager::serialize_to_file ()
{
  z_info ("Serializing plugin settings...");

  nlohmann::json json = *this;
  auto           json_str = json.dump ();

  auto path = get_file_path ();
  z_return_if_fail (
    !path.empty () && path.is_absolute () && path.has_parent_path ());

  z_debug ("Writing plugin settings to {}...", path);
  try
    {
      utils::io::set_file_contents (
        path, utils::Utf8String::from_utf8_encoded_string (json_str));
    }
  catch (const ZrythmException &e)
    {
      throw ZrythmException (
        format_str ("Unable to write plugin settings: {}", e.what ()));
    }
}

std::unique_ptr<PluginConfigurationManager>
PluginConfigurationManager::read_or_new ()
{
  auto ret = std::make_unique<PluginConfigurationManager> ();
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
      nlohmann::json j = nlohmann::json::parse (json);
      from_json (j, *ret);
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

// FIXME: do this in the deserialize override of
// zrythm::plugins::PluginDescriptor
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
PluginConfigurationManager::delete_file ()
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

void
PluginConfigurationManager::activate_plugin_configuration (
  const PluginConfiguration &config,
  bool                       autoroute_multiout,
  bool                       has_stereo_outputs)
{
  using namespace structure::tracks;
  bool has_errors = false;

  const auto &descr = *config.descr_;
  auto type = structure::tracks::Track::type_get_from_plugin_descriptor (descr);

  /* stop the engine so it doesn't restart all the time until all the actions
   * are performed */
  zrythm::engine::device_io::AudioEngine::State state{};
  AUDIO_ENGINE->wait_for_pause (state, false, true);

  try
    {
      if (autoroute_multiout)
        {
          int num_pairs = descr.num_audio_outs_;
          if (has_stereo_outputs)
            num_pairs = num_pairs / 2;
          int num_actions = 0;

          /* create group */
          auto * group = structure::tracks::Track::create_empty_with_action (
            structure::tracks::Track::Type::AudioGroup);
          num_actions++;

          /* create the plugin track */
          auto * pl_track = dynamic_cast<structure::tracks::ChannelTrack *> (
            structure::tracks::Track::create_for_plugin_at_idx_w_action (
              type, &config, TRACKLIST->track_count ()));
          num_actions++;

          auto pl_id = pl_track->channel_->instrument_;
          auto pl_var = pl_track->channel_->get_instrument ();

          /* move the plugin track inside the group */
          TRACKLIST->get_selection_manager ().select_unique (
            pl_track->get_uuid ());
          UNDO_MANAGER->perform (
            new gui::actions::MoveTracksInsideFoldableTrackAction (
              structure::tracks::TrackSpan{ std::ranges::to<std::vector> (
                TRACKLIST->get_track_span ().get_selected_tracks ()) },
              group->get_index ()));
          num_actions++;

          /* route to nowhere */
          TRACKLIST->get_selection_manager ().select_unique (
            pl_track->get_uuid ());
          UNDO_MANAGER->perform (new gui::actions::RemoveTracksDirectOutAction (
            structure::tracks::TrackSpan{ std::ranges::to<std::vector> (
              TRACKLIST->get_track_span ().get_selected_tracks ()) },
            *PORT_CONNECTIONS_MGR));
          num_actions++;

          /* rename group */
          auto name = utils::Utf8String::from_qstring (
            format_qstr (QObject::tr ("{} Output"), descr.getName ()));
          UNDO_MANAGER->perform (new gui::actions::RenameTrackAction (
            convert_to_variant<TrackPtrVariant> (group), *PORT_CONNECTIONS_MGR,
            name));
          num_actions++;

          auto pl_audio_outs = std::visit (
            [&] (auto &&pl) {
              return std::ranges::to<std::vector> (
                pl->get_output_port_span ()
                  .template get_elements_by_type<AudioPort> ());
            },
            pl_var.value ());

          for (int i = 0; i < num_pairs; i++)
            {
              /* create the audio fx track */
              auto * fx_track = dynamic_cast<structure::tracks::AudioBusTrack *> (
                structure::tracks::Track::create_empty_with_action (
                  structure::tracks::Track::Type::AudioBus));
              num_actions++;

              /* rename fx track */
              name = utils::Utf8String::from_utf8_encoded_string (
                format_str ("{} {}", descr.getName (), i + 1));
              UNDO_MANAGER->perform (new gui::actions::RenameTrackAction (
                convert_to_variant<TrackPtrVariant> (fx_track),
                *PORT_CONNECTIONS_MGR, name));
              num_actions++;

              /* move the fx track inside the group */
              TRACKLIST->get_selection_manager ().select_unique (
                fx_track->get_uuid ());
              UNDO_MANAGER->perform (
                new gui::actions::MoveTracksInsideFoldableTrackAction (
                  TrackSpan{ std::ranges::to<std::vector> (
                    TRACKLIST->get_track_span ().get_selected_tracks ()) },
                  group->get_index ()));
              num_actions++;

              /* move the fx track to the end */
              TRACKLIST->get_selection_manager ().select_unique (
                fx_track->get_uuid ());
              UNDO_MANAGER->perform (new gui::actions::MoveTracksAction (
                TrackSpan{ std::ranges::to<std::vector> (
                  TRACKLIST->get_track_span ().get_selected_tracks ()) },
                TRACKLIST->track_count ()));
              num_actions++;

              /* route to group */
              TRACKLIST->get_selection_manager ().select_unique (
                fx_track->get_uuid ());
              UNDO_MANAGER->perform (new gui::actions::ChangeTracksDirectOutAction (
                TrackSpan{ std::ranges::to<std::vector> (
                  TRACKLIST->get_track_span ().get_selected_tracks ()) },
                *PORT_CONNECTIONS_MGR,
                convert_to_variant<TrackPtrVariant> (group)));
              num_actions++;

              int  l_index = has_stereo_outputs ? i * 2 : i;
              auto port = pl_audio_outs[l_index];

              /* route left port to audio fx */
              UNDO_MANAGER->perform (new gui::actions::PortConnectionConnectAction (
                port->get_uuid (),
                fx_track->processor_->get_stereo_in_ports ().first.get_uuid ()));
              num_actions++;

              int r_index = has_stereo_outputs ? i * 2 + 1 : i;
              port = pl_audio_outs[r_index];

              /* route right port to audio fx */
              UNDO_MANAGER->perform (new gui::actions::PortConnectionConnectAction (
                port->get_uuid (),
                fx_track->processor_->get_stereo_in_ports ().second.get_uuid ()));
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
          structure::tracks::Track::create_for_plugin_at_idx_w_action (
            type, &config, TRACKLIST->track_count ());
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
      increment_num_instantiations_for_plugin (*config.get_descriptor ());
    }
}

#if 0
static void
on_outputs_stereo_response (
  AdwMessageDialog * dialog,
  char *             response,
  PluginConfiguration *    self)
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
  PluginConfiguration *    self)
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
        setting_clone, PluginConfiguration::free_closure, (GConnectFlags) 0);
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
PluginConfigurationManager::activate_plugin_configuration_async (
  const PluginConfiguration &config)
{
  structure::tracks::Track::Type type = structure::tracks::Track::
    type_get_from_plugin_descriptor (*config.get_descriptor ());

  if (
    config.descr_->num_audio_outs_ > 2
    && type == structure::tracks::Track::Type::Instrument)
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
        setting_clone, PluginConfiguration::free_closure, (GConnectFlags) 0);
      gtk_window_present (GTK_WINDOW (dialog));
#endif
    }
  else
    {
      activate_plugin_configuration (config, false, false);
    }
}

void
PluginConfigurationManager::set (
  const PluginConfiguration &setting,
  bool                       _serialize)
{
  z_debug ("Saving plugin setting for {}", setting.descr_->name_);

  PluginConfiguration * own_setting = find (*setting.descr_);

  if (own_setting)
    {
      own_setting->force_generic_ui_ = setting.force_generic_ui_;
      own_setting->bridge_mode_ = setting.bridge_mode_;
    }
  else
    {
      auto new_setting = utils::clone_unique (setting);
      new_setting->validate ();
      settings_.emplace_back (std::move (new_setting));
    }

  if (_serialize)
    {
      serialize_to_file_no_throw ();
    }
}

auto
PluginConfigurationManager::find (
  const zrythm::plugins::PluginDescriptor &descr) -> PluginConfiguration *
{
  auto it = std::ranges::find_if (settings_, [&descr] (const auto &s) {
    return s->descr_->is_same_plugin (descr);
  });
  if (it == settings_.end ())
    {
      return nullptr;
    }
  return (*it).get ();
}
