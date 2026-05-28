// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_input_selection.h"
#include "dsp/audio_pool.h"
#include "dsp/engine.h"
#include "dsp/hardware_audio_interface.h"
#include "dsp/metronome.h"
#include "dsp/midi_input_selection.h"
#include "dsp/port_connections_manager.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "dsp/transport.h"
#include "plugins/plugin.h"
#include "plugins/plugin_factory.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/arrangement/tempo_object_manager.h"
#include "structure/project/project_registry.h"
#include "structure/scenes/clip_launcher.h"
#include "structure/scenes/clip_playback_service.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/tracklist.h"

namespace zrythm::dsp
{
class Fader;
}
namespace zrythm::utils
{
class AppSettings;
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

public:
  using TrackUuid = structure::tracks::TrackUuid;
  using PluginPtrVariant = plugins::PluginPtrVariant;
  using ProjectDirectoryPathProvider =
    std::function<std::filesystem::path (bool for_backup)>;

  /**
   * @brief Callback to look up audio input selection for a track.
   *
   * Set by ProjectSession after construction. Returns nullptr if no selection
   * exists for the given track UUID.
   */
  using AudioInputSelectionProvider = std::function<dsp::AudioInputSelection *(
    const structure::tracks::Track::Uuid &)>;

  using MidiInputSelectionProvider = std::function<dsp::MidiInputSelection *(
    const structure::tracks::Track::Uuid &)>;

public:
  Project (
    utils::AppSettings           &app_settings,
    ProjectDirectoryPathProvider  project_directory_path_provider,
    dsp::IHardwareAudioInterface &hw_interface,
    dsp::IHardwareMidiInterface  &midi_interface,
    std::shared_ptr<juce::AudioPluginFormatManager> plugin_format_manager,
    plugins::PluginHostWindowFactory                plugin_host_window_provider,
    dsp::Metronome                                 &metronome,
    dsp::Fader                                     &monitor_fader,
    QObject *                                       parent = nullptr);
  ~Project () override;
  Q_DISABLE_COPY_MOVE (Project)

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

  structure::tracks::FinalTrackDependencies get_final_track_dependencies ();

  utils::IObjectRegistry       &get_registry () { return project_registry_; }
  const utils::IObjectRegistry &get_registry () const
  {
    return project_registry_;
  }

  const auto &tempo_map () const { return tempo_map_; }

  void set_audio_input_selection_provider (AudioInputSelectionProvider provider)
  {
    audio_input_selection_provider_ = std::move (provider);
  }

  const auto &audio_input_selection_provider () const
  {
    return audio_input_selection_provider_;
  }

  void set_midi_input_selection_provider (MidiInputSelectionProvider provider)
  {
    midi_input_selection_provider_ = std::move (provider);
  }

  const auto &midi_input_selection_provider () const
  {
    return midi_input_selection_provider_;
  }

  /**
   * @brief Installs the recording callback used by all tracks.
   *
   * Must be called exactly once by ProjectSession after construction,
   * before any tracks are created.
   */
  void install_recording_callback (
    structure::tracks::TrackRecordingCallback callback);

private:
  static constexpr auto kTempoMapKey = "tempoMap"sv;
  static constexpr auto kRegistryKey = "registry"sv;
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

  ProjectRegistry project_registry_;

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
   *
   * Thread-safe because tracks are only added/removed while the graph is
   * paused (engine not running).
   */
  std::vector<structure::tracks::Track *> tracks_rt_;

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

  AudioInputSelectionProvider audio_input_selection_provider_;
  MidiInputSelectionProvider  midi_input_selection_provider_;

  structure::tracks::TrackRecordingCallback track_recording_callback_;

  /**
   * @brief Graph dispatcher.
   *
   * @note Needs to be deleted after the audio engine so placed here torwards
   * the end.
   */
  dsp::DspGraphDispatcher graph_dispatcher_;
};

}
