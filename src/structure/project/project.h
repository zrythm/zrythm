// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_pool.h"
#include "dsp/engine.h"
#include "dsp/hardware_audio_interface.h"
#include "dsp/metronome.h"
#include "dsp/port_connections_manager.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "dsp/transport.h"
#include "plugins/plugin.h"
#include "plugins/plugin_factory.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/arrangement/tempo_object_manager.h"
#include "structure/scenes/clip_launcher.h"
#include "structure/scenes/clip_playback_service.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/tracklist.h"
#include "utils/app_settings.h"

namespace zrythm::dsp
{
class Fader;
}

namespace zrythm::structure::project
{

#define PORT_CONNECTIONS_MGR (PROJECT->port_connections_manager_.get ())
#define AUDIO_POOL (PROJECT->audio_pool_.get ())
#define TRANSPORT (PROJECT->transport_)

#define TRACKLIST (PROJECT->tracklist_)
#define P_CHORD_TRACK (TRACKLIST->singletonTracks ()->chordTrack ())
#define P_MARKER_TRACK (TRACKLIST->singletonTracks ()->markerTrack ())
#define P_MASTER_TRACK (TRACKLIST->singletonTracks ()->masterTrack ())
#define P_MODULATOR_TRACK (TRACKLIST->singletonTracks ()->modulatorTrack ())
#define MONITOR_FADER (PROJECT->controlRoom ()->monitor_fader_)
#define ROUTER (&PROJECT->engine ()->graph_dispatcher ())
#define AUDIO_ENGINE (PROJECT->engine ())

/**
 * @brief Core functionality of a Zrythm project.
 */
class Project final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    zrythm::structure::tracks::Tracklist * tracklist READ tracklist CONSTANT
      FINAL)
  Q_PROPERTY (
    zrythm::structure::scenes::ClipLauncher * clipLauncher READ clipLauncher
      CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::structure::scenes::ClipPlaybackService * clipPlaybackService READ
      clipPlaybackService CONSTANT FINAL)
  Q_PROPERTY (zrythm::dsp::AudioEngine * engine READ engine CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::dsp::Transport * transport READ getTransport CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::dsp::TempoMapWrapper * tempoMap READ getTempoMap CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::structure::arrangement::TempoObjectManager * tempoObjectManager READ
      tempoObjectManager CONSTANT FINAL)
  QML_UNCREATABLE ("")

  friend struct PluginBuilderForDeserialization;

public:
  using TrackUuid = structure::tracks::TrackUuid;
  using PluginPtrVariant = plugins::PluginPtrVariant;
  using PluginRegistry = plugins::PluginRegistry;
  using ProjectDirectoryPathProvider = std::function<fs::path (bool for_backup)>;

public:
  Project (
    utils::AppSettings           &app_settings,
    ProjectDirectoryPathProvider  project_directory_path_provider,
    dsp::IHardwareAudioInterface &hw_interface,
    std::shared_ptr<juce::AudioPluginFormatManager> plugin_format_manager,
    plugins::PluginHostWindowFactory                plugin_host_window_provider,
    dsp::Metronome                                 &metronome,
    dsp::Fader                                     &monitor_fader,
    QObject *                                       parent = nullptr);
  ~Project () override;
  Z_DISABLE_COPY_MOVE (Project)

  // =========================================================
  // QML interface
  // =========================================================

  structure::tracks::Tracklist *               tracklist () const;
  structure::scenes::ClipLauncher *            clipLauncher () const;
  structure::scenes::ClipPlaybackService *     clipPlaybackService () const;
  dsp::Transport *                             getTransport () const;
  dsp::AudioEngine *                           engine () const;
  dsp::TempoMapWrapper *                       getTempoMap () const;
  structure::arrangement::TempoObjectManager * tempoObjectManager () const;

  Q_SIGNAL void aboutToBeDeleted ();

  // =========================================================
  // TODO: Remove these accessors - QML should access via ControlRoom instead

  dsp::Fader     &monitor_fader () const;
  dsp::Metronome &metronome () const;

  // TODO: delete this getter, no one else should use factory directly
  auto * arrangerObjectFactory () const
  {
    return arranger_object_factory_.get ();
  }

  friend void init_from (
    Project               &obj,
    const Project         &other,
    utils::ObjectCloneType clone_type);

  /**
   * @brief Adds the default undeletable tracks to the project.
   *
   * To be called when creating new projects.
   */
  void add_default_tracks ();

  structure::tracks::FinalTrackDependencies
  get_final_track_dependencies () const;

  auto &get_file_audio_source_registry () const
  {
    return *file_audio_source_registry_;
  }
  auto &get_track_registry () const { return *track_registry_; }
  auto &get_plugin_registry () const { return *plugin_registry_; }
  auto &get_port_registry () const { return *port_registry_; }
  auto &get_param_registry () const { return *param_registry_; }
  auto &get_arranger_object_registry () const
  {
    return *arranger_object_registry_;
  }

  /**
   * Finds the Port corresponding to the identifier.
   *
   * @param id The PortIdentifier to use for searching.
   *
   * @note Ported from Port::find_from_identifier() in older code.
   */
  std::optional<dsp::PortPtrVariant>
  find_port_by_id (const dsp::Port::Uuid &id) const
  {
    return get_port_registry ().find_by_id (id);
  }

  dsp::ProcessorParameter *
  find_param_by_id (const dsp::ProcessorParameter::Uuid &id) const
  {
    const auto opt_var = get_param_registry ().find_by_id (id);
    if (opt_var.has_value ())
      {
        return std::get<dsp::ProcessorParameter *> (opt_var.value ());
      }
    return nullptr;
  }

  std::optional<plugins::PluginPtrVariant>
  find_plugin_by_id (plugins::Plugin::Uuid id) const
  {
    return get_plugin_registry ().find_by_id (id);
  }

  std::optional<zrythm::structure::tracks::TrackPtrVariant>
  find_track_by_id (structure::tracks::Track::Uuid id) const
  {
    return get_track_registry ().find_by_id (id);
  }

  std::optional<zrythm::structure::arrangement::ArrangerObjectPtrVariant>
  find_arranger_object_by_id (
    structure::arrangement::ArrangerObject::Uuid id) const
  {
    return get_arranger_object_registry ().find_by_id (id);
  }

  const auto &tempo_map () const { return tempo_map_; }

private:
  static constexpr auto kTempoMapKey = "tempoMap"sv;
  static constexpr auto kFileAudioSourceRegistryKey =
    "fileAudioSourceRegistry"sv;
  static constexpr auto kPortRegistryKey = "portRegistry"sv;
  static constexpr auto kParameterRegistryKey = "paramRegistry"sv;
  static constexpr auto kPluginRegistryKey = "pluginRegistry"sv;
  static constexpr auto kArrangerObjectRegistryKey = "arrangerObjectRegistry"sv;
  static constexpr auto kTrackRegistryKey = "trackRegistry"sv;
  static constexpr auto kTransportKey = "transport"sv;
  static constexpr auto kAudioPoolKey = "audioPool"sv;
  static constexpr auto kTracklistKey = "tracklist"sv;
  static constexpr auto kRegionLinkGroupManagerKey = "regionLinkGroupManager"sv;
  static constexpr auto kPortConnectionsManagerKey = "portConnectionsManager"sv;
  static constexpr auto kTempoObjectManagerKey = "tempoObjectManager"sv;
  static constexpr auto kClipLauncherKey = "clipLauncher"sv;
  friend void           to_json (nlohmann::json &j, const Project &project);
  friend void           from_json (const nlohmann::json &j, Project &project);

private:
  utils::AppSettings                           &app_settings_;
  dsp::TempoMap                                 tempo_map_;
  utils::QObjectUniquePtr<dsp::TempoMapWrapper> tempo_map_wrapper_;

  plugins::PluginHostWindowFactory plugin_host_window_provider_;

  dsp::FileAudioSourceRegistry *    file_audio_source_registry_{};
  dsp::PortRegistry *               port_registry_{};
  dsp::ProcessorParameterRegistry * param_registry_{};
  PluginRegistry *                  plugin_registry_{};
  structure::arrangement::ArrangerObjectRegistry * arranger_object_registry_{};
  structure::tracks::TrackRegistry *               track_registry_{};

  ProjectDirectoryPathProvider project_directory_path_provider_;

  /* !!! IMPORTANT: order matters (for destruction) !!! */

  dsp::IHardwareAudioInterface                   &hw_interface_;
  std::shared_ptr<juce::AudioPluginFormatManager> plugin_format_manager_;

public:
  /**
   * @brief
   *
   * Must be free'd after engine.
   */
  utils::QObjectUniquePtr<dsp::PortConnectionsManager> port_connections_manager_;

public:
  /**
   * Timeline metadata like BPM, time signature, etc.
   */
  utils::QObjectUniquePtr<dsp::Transport> transport_;

  /**
   * The audio backend.
   */
  utils::QObjectUniquePtr<dsp::AudioEngine> audio_engine_;

  /** Audio file pool. */
  std::unique_ptr<dsp::AudioPool> pool_;

  /** Manager for region link groups. */
  // structure::arrangement::RegionLinkGroupManager region_link_group_manager_;

  /**
   * @brief Tracklist.
   *
   * Must be free'd before engine and port connection manager.
   */
  utils::QObjectUniquePtr<structure::tracks::Tracklist> tracklist_;

  /**
   * @brief Realtime cache of tracks
   */
  boost::unordered_flat_map<
    structure::tracks::TrackUuid,
    structure::tracks::TrackPtrVariant>
    tracks_rt_;

private:
  utils::QObjectUniquePtr<structure::scenes::ClipLauncher> clip_launcher_;
  utils::QObjectUniquePtr<structure::scenes::ClipPlaybackService>
    clip_playback_service_;

  std::unique_ptr<structure::arrangement::ArrangerObjectFactory>
    arranger_object_factory_;

public:
  utils::QObjectUniquePtr<plugins::PluginFactory>  plugin_factory_;
  std::unique_ptr<structure::tracks::TrackFactory> track_factory_;

private:
  utils::QObjectUniquePtr<structure::arrangement::TempoObjectManager>
    tempo_object_manager_;

  dsp::Fader     &monitor_fader_;
  dsp::Metronome &metronome_;

  /**
   * @brief Graph dispatcher.
   *
   * @note Needs to be deleted after the audio engine so placed here torwards
   * the end.
   */
  dsp::DspGraphDispatcher graph_dispatcher_;
};

}
