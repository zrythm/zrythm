// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "actions/arranger_object_creator.h"
#include "actions/track_creator.h"
#include "dsp/audio_pool.h"
#include "dsp/snap_grid.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "engine/device_io/engine.h"
#include "engine/session/midi_mapping.h"
#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/clip_editor.h"
#include "gui/backend/file_importer.h"
#include "gui/backend/plugin_selection_manager.h"
#include "gui/backend/tool.h"
#include "gui/backend/track_selection_manager.h"
#include "gui/dsp/plugin_factory.h"
#include "gui/dsp/quantize_options.h"
#include "plugins/plugin.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/arrangement/timeline.h"
#include "structure/scenes/clip_launcher.h"
#include "structure/scenes/clip_playback_service.h"
#include "structure/tracks/track_factory.h"
#include "undo/undo_stack.h"
#include "utils/progress_info.h"

/**
 * @addtogroup project Project
 *
 * @{
 */

using namespace zrythm;

#define PROJECT (Project::get_active_instance ())
#define PORT_CONNECTIONS_MGR (PROJECT->port_connections_manager_.get ())
#define AUDIO_POOL (PROJECT->audio_pool_.get ())

#define TRACKLIST (PROJECT->tracklist_)
#define P_CHORD_TRACK (TRACKLIST->singletonTracks ()->chordTrack ())
#define P_MARKER_TRACK (TRACKLIST->singletonTracks ()->markerTrack ())
#define P_MASTER_TRACK (TRACKLIST->singletonTracks ()->masterTrack ())
#define P_MODULATOR_TRACK (TRACKLIST->singletonTracks ()->modulatorTrack ())
#define SNAP_GRID_TIMELINE (PROJECT->snapGridTimeline ())
#define SNAP_GRID_EDITOR (PROJECT->snapGridEditor ())

enum class ProjectPath
{
  ProjectFile,
  BACKUPS,

  /** Plugins path. */
  PLUGINS,

  /** Path for state .ttl files. */
  PluginStates,

  /** External files for plugin states, under the
   * STATES dir. */
  PLUGIN_EXT_COPIES,

  /** External files for plugin states, under the
   * STATES dir. */
  PLUGIN_EXT_LINKS,

  EXPORTS,

  /* EXPORTS / "stems". */
  EXPORTS_STEMS,

  POOL,

  FINISHED_FILE,
};

/**
 * @brief Contains all of the info that will be serialized into a project file.
 *
 * @todo Create a UserInterface struct and move things that are only relevant to
 * the UI there.
 *
 * A project (or song), contains all the project data as opposed to zrythm_app.h
 * which manages global things like plugin descriptors and global settings.
 **/
class Project final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    QString title READ getTitle WRITE setTitle NOTIFY titleChanged FINAL)
  Q_PROPERTY (
    QString directory READ getDirectory WRITE setDirectory NOTIFY
      directoryChanged FINAL)
  Q_PROPERTY (
    zrythm::structure::tracks::Tracklist * tracklist READ tracklist CONSTANT
      FINAL)
  Q_PROPERTY (
    zrythm::structure::scenes::ClipLauncher * clipLauncher READ clipLauncher
      CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::structure::scenes::ClipPlaybackService * clipPlaybackService READ
      clipPlaybackService CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::gui::backend::TrackSelectionManager * trackSelectionManager READ
      trackSelectionManager CONSTANT)
  Q_PROPERTY (
    zrythm::structure::arrangement::Timeline * timeline READ getTimeline
      CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::engine::device_io::AudioEngine * engine READ engine CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::engine::session::Transport * transport READ getTransport CONSTANT
      FINAL)
  Q_PROPERTY (zrythm::gui::backend::Tool * tool READ getTool CONSTANT FINAL)
  Q_PROPERTY (ClipEditor * clipEditor READ getClipEditor CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::gui::actions::UndoManager * undoManager READ getUndoManager CONSTANT
      FINAL)
  Q_PROPERTY (zrythm::undo::UndoStack * undoStack READ undoStack CONSTANT FINAL)
  Q_PROPERTY (PluginFactory * pluginFactory READ getPluginFactory CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::actions::ArrangerObjectCreator * arrangerObjectCreator READ
      arrangerObjectCreator CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::actions::TrackCreator * trackCreator READ trackCreator CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::gui::backend::FileImporter * fileImporter READ fileImporter CONSTANT
      FINAL)
  Q_PROPERTY (
    zrythm::dsp::TempoMapWrapper * tempoMap READ getTempoMap CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::dsp::SnapGrid * snapGridTimeline READ snapGridTimeline CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::dsp::SnapGrid * snapGridEditor READ snapGridEditor CONSTANT FINAL)
  QML_UNCREATABLE ("")

public:
  using QuantizeOptions = zrythm::gui::old_dsp::QuantizeOptions;
  using TrackUuid = structure::tracks::TrackUuid;
  using PluginPtrVariant = plugins::PluginPtrVariant;
  using PluginRegistry = plugins::PluginRegistry;

  static constexpr auto PROJECT_FILE = "project.zpj"sv;
  static constexpr auto PROJECT_BACKUPS_DIR = "backups"sv;
  static constexpr auto PROJECT_PLUGINS_DIR = "plugins"sv;
  static constexpr auto PROJECT_PLUGIN_STATES_DIR = "states"sv;
  static constexpr auto PROJECT_PLUGIN_EXT_COPIES_DIR = "ext_file_copies"sv;
  static constexpr auto PROJECT_PLUGIN_EXT_LINKS_DIR = "ext_file_links"sv;
  static constexpr auto PROJECT_EXPORTS_DIR = "exports"sv;
  static constexpr auto PROJECT_STEMS_DIR = "stems"sv;
  static constexpr auto PROJECT_POOL_DIR = "pool"sv;
  static constexpr auto PROJECT_FINISHED_FILE = "FINISHED"sv;

public:
  Project (
    std::shared_ptr<juce::AudioDeviceManager> device_manager,
    QObject *                                 parent = nullptr);
  ~Project () override;
  Z_DISABLE_COPY_MOVE (Project)

  /**
   * Selection type, used for controlling which part of the interface is
   * selected, for copy-paste, displaying info in the inspector, etc.
   */
  enum class SelectionType
  {
    /** Track selection in tracklist or mixer. */
    Tracklist,

    /** Timeline or pinned timeline. */
    Timeline,

    /** Insert selections in the mixer. */
    Insert,

    /** MIDI FX selections in the mixer. */
    MidiFX,

    /** Instrument slot. */
    Instrument,

    /** Modulator slot. */
    Modulator,

    /** Editor arranger. */
    Editor,
  };

  /**
   * Flag to pass to project_compress() and project_decompress().
   */
  enum class CompressionFlag
  {
    PROJECT_COMPRESS_FILE = 0,
    PROJECT_DECOMPRESS_FILE = 0,
    PROJECT_COMPRESS_DATA = 1,
    PROJECT_DECOMPRESS_DATA = 1,
  };

  // =========================================================
  // QML interface
  // =========================================================

  QString                           getTitle () const;
  void                              setTitle (const QString &title);
  QString                           getDirectory () const;
  void                              setDirectory (const QString &directory);
  structure::tracks::Tracklist *    tracklist () const;
  structure::scenes::ClipLauncher * clipLauncher () const;
  structure::scenes::ClipPlaybackService * clipPlaybackService () const;
  gui::backend::TrackSelectionManager *    trackSelectionManager () const;
  gui::backend::PluginSelectionManager *   pluginSelectionManager () const;
  structure::arrangement::Timeline *       getTimeline () const;
  engine::session::Transport *             getTransport () const;
  engine::device_io::AudioEngine *         engine () const;
  gui::backend::Tool *                     getTool () const;
  ClipEditor *                             getClipEditor () const;
  gui::actions::UndoManager *              getUndoManager () const;
  undo::UndoStack *                        undoStack () const;
  PluginFactory *                          getPluginFactory () const;
  zrythm::actions::ArrangerObjectCreator * arrangerObjectCreator () const;
  zrythm::actions::TrackCreator *          trackCreator () const;
  gui::backend::FileImporter *             fileImporter () const;
  dsp::TempoMapWrapper *                   getTempoMap () const;
  dsp::SnapGrid *                          snapGridTimeline () const;
  dsp::SnapGrid *                          snapGridEditor () const;

  Q_SIGNAL void titleChanged (const QString &title);
  Q_SIGNAL void directoryChanged (const QString &directory);
  Q_SIGNAL void aboutToBeDeleted ();

  // =========================================================

  /**
   * Returns the requested project path as a newly allocated string.
   *
   * @param backup Whether to get the path for the current backup instead of the
   * main project.
   */
  fs::path get_path (ProjectPath path, bool backup) const;

  static Project * get_active_instance ();

  // TODO: delete this getter, no one else should use factory directly
  auto * getArrangerObjectFactory () const
  {
    return arranger_object_factory_.get ();
  }

  /**
   * Saves the project to a project file in the given dir.
   *
   * @param is_backup 1 if this is a backup. Backups will be saved as <original
   * filename>.bak<num>.
   * @param show_notification Show a notification in the UI that the project
   * was saved.
   * @param async Save asynchronously in another thread.
   *
   * @throw ZrythmException If the project cannot be saved.
   */
  void
  save (const fs::path &_dir, bool is_backup, bool show_notification, bool async);

  /**
   * Autosave callback.
   *
   * This will keep getting called at regular short intervals, and if enough
   * time has passed and it's okay to save it will autosave, otherwise it will
   * wait until the next interval and check again.
   */
  static int autosave_cb (void * data);

  /**
   * @brief Creates the project directories.
   *
   * @param is_backup
   * @throw ZrythmException If the directories cannot be created.
   */
  void make_project_dirs (bool is_backup);

  /**
   * Compresses/decompress a project from a file/data to a file/data.
   *
   * @param compress True to compress, false to decompress.
   * @param[out] _dest Pointer to a location to allocate memory.
   * @param[out] _dest_size Pointer to a location to store the size of the
   * allocated memory.
   * @param src Input bytes to compress/decompress.
   *
   * @throw ZrythmException If the compression/decompression fails.
   */
  static void compress_or_decompress (
    bool              compress,
    char **           _dest,
    size_t *          _dest_size,
    CompressionFlag   dest_type,
    const QByteArray &src);

  static void compress (
    char **           _dest,
    size_t *          _dest_size,
    CompressionFlag   dest_type,
    const QByteArray &src)
  {
    compress_or_decompress (true, _dest, _dest_size, dest_type, src);
  }

  static void decompress (
    char **           _dest,
    size_t *          _dest_size,
    CompressionFlag   dest_type,
    const QByteArray &src)
  {
    compress_or_decompress (false, _dest, _dest_size, dest_type, src);
  }

  /**
   * Returns the uncompressed text representation of the saved project file.
   *
   * @param backup Whether to use the project file from the most recent
   * backup.
   *
   * @throw ZrythmException If an error occurs.
   */
  std::string get_existing_uncompressed_text (bool backup);

  bool has_unsaved_changes () const;

  friend void init_from (
    Project               &obj,
    const Project         &other,
    utils::ObjectCloneType clone_type);

  /**
   * Deep-clones the given project.
   *
   * To be used during save on the main thread.
   *
   * @param for_backup Whether the resulting project is for a backup.
   *
   * @throw ZrythmException If an error occurs.
   */
  Q_INVOKABLE Project * clone (bool for_backup) const;

  /**
   * Returns the filepath of a backup (directory), if any, if it has a newer
   * timestamp than main project file.
   *
   * Returns nullopt if there were errors or no backup was found.
   */
  std::optional<fs::path> get_newer_backup ();

  /**
   * @brief Connects things up, exposes ports to the backend, calculates the
   * graph and begins processing.
   *
   */
  Q_INVOKABLE void activate ();

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

  const auto &get_tempo_map () const { return tempo_map_; }

private:
  /**
   * Sets (and creates on the disk) the next available backup dir to use for
   * saving a backup during this call.
   *
   * @throw ZrythmException If the backup dir cannot be created.
   */
  void set_and_create_next_available_backup_dir ();

  /**
   * Cleans up unnecessary plugin state dirs from the main project.
   *
   * This is called during save on the newly cloned project to be saved.
   *
   * @param main_project The main project.
   * @param is_backup Whether this is for a backup.
   */
  void cleanup_plugin_state_dirs (Project &main_project, bool is_backup);

  /**
   * Project save data.
   */
  struct SaveContext
  {
    /** Project clone (with memcpy). */
    // std::unique_ptr<Project> project_;

    /**
     * @brief Original project.
     */
    Project * main_project_;

    /** Full path to save to. */
    fs::path project_file_path_;

    bool is_backup_ = false;

    /** To be set to true when the thread finishes. */
    std::atomic_bool finished_ = false;

    bool show_notification_ = false;

    /** Whether an error occurred during saving. */
    bool has_error_ = false;

    ProgressInfo progress_info_;
  };

  /**
   * Thread that does the serialization and saving.
   */
  class SerializeProjectThread final : public juce::Thread
  {
  public:
    SerializeProjectThread (SaveContext &ctx);
    ~SerializeProjectThread () override;
    void run () override;

  private:
    SaveContext &ctx_;
  };

  /**
   * Idle func to check if the project has finished saving and show a
   * notification.
   *
   * @return Whether to continue calling this.
   */
  static bool idle_saved_callback (SaveContext * ctx);

private:
  static constexpr auto kTempoMapKey = "tempoMap"sv;
  static constexpr auto kFileAudioSourceRegistryKey =
    "fileAudioSourceRegistry"sv;
  static constexpr auto kPortRegistryKey = "portRegistry"sv;
  static constexpr auto kParameterRegistryKey = "paramRegistry"sv;
  static constexpr auto kPluginRegistryKey = "pluginRegistry"sv;
  static constexpr auto kArrangerObjectRegistryKey = "arrangerObjectRegistry"sv;
  static constexpr auto kTrackRegistryKey = "trackRegistry"sv;
  static constexpr auto kTitleKey = "title"sv;
  static constexpr auto kDatetimeKey = "datetime"sv;
  static constexpr auto kVersionKey = "version"sv;
  static constexpr auto kClipEditorKey = "clipEditor"sv;
  static constexpr auto kTimelineKey = "timeline"sv;
  static constexpr auto kSnapGridTimelineKey = "snapGridTimeline"sv;
  static constexpr auto kSnapGridEditorKey = "snapGridEditor"sv;
  static constexpr auto kQuantizeOptsTimelineKey = "quantizeOptsTimeline"sv;
  static constexpr auto kQuantizeOptsEditorKey = "quantizeOptsEditor"sv;
  static constexpr auto kTransportKey = "transport"sv;
  static constexpr auto kAudioEngineKey = "audioEngine"sv;
  static constexpr auto kAudioPoolKey = "audioPool"sv;
  static constexpr auto kTracklistKey = "tracklist"sv;
  static constexpr auto kRegionLinkGroupManagerKey = "regionLinkGroupManager"sv;
  static constexpr auto kPortConnectionsManagerKey = "portConnectionsManager"sv;
  static constexpr auto kMidiMappingsKey = "midiMappings"sv;
  static constexpr auto kUndoManagerKey = "undoManager"sv;
  static constexpr auto kUndoStackKey = "undoStack"sv;
  static constexpr auto kLastSelectionKey = "lastSelection"sv;
  static constexpr auto DOCUMENT_TYPE = "ZrythmProject"sv;
  static constexpr auto FORMAT_MAJOR_VER = 2;
  static constexpr auto FORMAT_MINOR_VER = 1;
  friend void           to_json (nlohmann::json &j, const Project &project);
  friend void           from_json (const nlohmann::json &j, Project &project);

private:
  dsp::TempoMap                                 tempo_map_;
  utils::QObjectUniquePtr<dsp::TempoMapWrapper> tempo_map_wrapper_;

  dsp::FileAudioSourceRegistry *    file_audio_source_registry_{};
  dsp::PortRegistry *               port_registry_{};
  dsp::ProcessorParameterRegistry * param_registry_{};
  PluginRegistry *                  plugin_registry_{};
  structure::arrangement::ArrangerObjectRegistry * arranger_object_registry_{};
  structure::tracks::TrackRegistry *               track_registry_{};

public:
  /** Project title. */
  utils::Utf8String title_;

  /** Datetime string to add to the project file. */
  utils::Utf8String datetime_str_;

  /** Path to save the project in. */
  fs::path dir_;

  /**
   * Backup dir to save the project during the current save call.
   *
   * For example, @ref Project.dir /backups/myproject.bak3.
   */
  std::optional<fs::path> backup_dir_;

  /* !!! IMPORTANT: order matters (for destruction) !!! */

  std::optional<gui::actions::UndoableActionPtrVariant>
    last_action_in_last_successful_autosave_;

  /** Last successful autosave timestamp. */
  SteadyTimePoint last_successful_autosave_time_;

  /** Used to check if the project has unsaved changes. */
  std::optional<gui::actions::UndoableActionPtrVariant> last_saved_action_;

  /** Semaphore used to block saving. */
  std::binary_semaphore save_sem_{ 1 };

  /** Zrythm version, for serialization */
  utils::Utf8String version_;

  /**
   * The last thing selected in the GUI.
   *
   * Used in inspector_widget_refresh.
   */
  Project::SelectionType last_selection_ = (SelectionType) 0;

  /**
   * If a project is currently loaded or not.
   *
   * This is useful so that we know if we need to tear down when loading a
   * new project while another one is loaded.
   */
  bool loaded_ = false;

  /**
   * Whether the current is currently being loaded from a backup file.
   *
   * This is useful when instantiating plugins from state and should be set
   * to false after the project is loaded.
   */
  bool loading_from_backup_ = false;

  /**
   * Currently selected tool (select - normal,
   * select - stretch, edit, delete, ramp, audition)
   */
  gui::backend::Tool * tool_{};

  /**
   * @brief
   *
   * Must be free'd after engine.
   */
  utils::QObjectUniquePtr<dsp::PortConnectionsManager> port_connections_manager_;

  /**
   * The audio backend.
   */
  utils::QObjectUniquePtr<engine::device_io::AudioEngine> audio_engine_;

  /**
   * Timeline metadata like BPM, time signature, etc.
   */
  engine::session::Transport * transport_;

  /** Audio file pool. */
  std::unique_ptr<dsp::AudioPool> pool_;

  /** Manager for region link groups. */
  // structure::arrangement::RegionLinkGroupManager region_link_group_manager_;

  /** Quantize info for the piano roll. */
  std::unique_ptr<QuantizeOptions> quantize_opts_editor_;

  /** Quantize info for the timeline. */
  std::unique_ptr<QuantizeOptions> quantize_opts_timeline_;

  /** Snap/Grid info for the editor. */
  utils::QObjectUniquePtr<dsp::SnapGrid> snap_grid_editor_;

  /** Snap/Grid info for the timeline. */
  utils::QObjectUniquePtr<dsp::SnapGrid> snap_grid_timeline_;

  /** Timeline widget backend. */
  utils::QObjectUniquePtr<structure::arrangement::Timeline> timeline_;

  /** Backend for the widget. */
  utils::QObjectUniquePtr<ClipEditor> clip_editor_;

  /** MIDI bindings. */
  std::unique_ptr<engine::session::MidiMappings> midi_mappings_;

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

  utils::QObjectUniquePtr<structure::scenes::ClipLauncher> clip_launcher_;
  utils::QObjectUniquePtr<structure::scenes::ClipPlaybackService>
    clip_playback_service_;

  gui::actions::UndoManager * legacy_undo_manager_{};

  utils::QObjectUniquePtr<undo::UndoStack> undo_stack_;

  std::unique_ptr<structure::arrangement::ArrangerObjectFactory>
                                                   arranger_object_factory_;
  PluginFactory *                                  plugin_factory_{};
  std::unique_ptr<structure::tracks::TrackFactory> track_factory_;

  utils::QObjectUniquePtr<actions::ArrangerObjectCreator>
                                                 arranger_object_creator_;
  utils::QObjectUniquePtr<actions::TrackCreator> track_creator_;

  utils::QObjectUniquePtr<gui::backend::FileImporter> file_importer_;

  utils::QObjectUniquePtr<gui::backend::TrackSelectionManager>
    track_selection_manager_;
  utils::QObjectUniquePtr<gui::backend::PluginSelectionManager>
    plugin_selection_manager_;

  /** Used when deserializing projects. */
  int format_major_ = 0;
  int format_minor_ = 0;

  std::shared_ptr<juce::AudioDeviceManager> device_manager_;
};

/**
 * @}
 */
