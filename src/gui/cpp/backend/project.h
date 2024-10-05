// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __PROJECT_H__
#define __PROJECT_H__

#include <string>

#include "common/dsp/engine.h"
#include "common/dsp/midi_mapping.h"
#include "common/dsp/port.h"
#include "common/dsp/port_connections_manager.h"
#include "common/dsp/quantize_options.h"
#include "common/dsp/region_link_group_manager.h"
#include "common/plugins/plugin.h"
#include "common/utils/progress_info.h"
#include "gui/cpp/backend/actions/undo_manager.h"
#include "gui/cpp/backend/audio_selections.h"
#include "gui/cpp/backend/automation_selections.h"
#include "gui/cpp/backend/chord_selections.h"
#include "gui/cpp/backend/clip_editor.h"
#include "gui/cpp/backend/midi_selections.h"
#include "gui/cpp/backend/mixer_selections.h"
#include "gui/cpp/backend/timeline.h"
#include "gui/cpp/backend/timeline_selections.h"
#include "gui/cpp/backend/tool.h"
#include "gui/cpp/backend/tracklist_selections.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

/**
 * @addtogroup project Project
 *
 * @{
 */

#define PROJECT (gZrythm->project_)
constexpr const char * PROJECT_FILE = "project.zpj";
#define PROJECT_BACKUPS_DIR "backups"
#define PROJECT_PLUGINS_DIR "plugins"
#define PROJECT_PLUGIN_STATES_DIR "states"
#define PROJECT_PLUGIN_EXT_COPIES_DIR "ext_file_copies"
#define PROJECT_PLUGIN_EXT_LINKS_DIR "ext_file_links"
#define PROJECT_EXPORTS_DIR "exports"
#define PROJECT_STEMS_DIR "stems"
#define PROJECT_POOL_DIR "pool"
#define PROJECT_FINISHED_FILE "FINISHED"

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
 * Flag to pass to project_compress() and project_decompress().
 */
enum class ProjectCompressionFlag
{
  PROJECT_COMPRESS_FILE,
  PROJECT_COMPRESS_DATA,
};

#define PROJECT_DECOMPRESS_FILE ProjectCompressionFlag::PROJECT_COMPRESS_FILE
#define PROJECT_DECOMPRESS_DATA ProjectCompressionFlag::PROJECT_COMPRESS_DATA

/**
 * @brief Contains all of the info that will be serialized into a project file.
 *
 * @todo Create a UserInterface struct and move things that are only relevant to
 * the UI there.
 *
 * A project (or song), contains all the project data as opposed to zrythm_app.h
 * which manages global things like plugin descriptors and global settings.
 **/
class Project final : public ISerializable<Project>, public ICloneable<Project>
{
public:
  Project ();
  Project (std::string &title);
  ~Project () override;

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
   * Returns the requested project path as a newly allocated string.
   *
   * @param backup Whether to get the path for the current backup instead of the
   * main project.
   */
  std::string get_path (ProjectPath path, bool backup);

  /**
   * Checks that everything is okay with the project.
   */
  bool validate () const;

  DECLARE_DEFINE_FIELDS_METHOD ();

  std::string get_document_type () const override { return "ZrythmProject"; }

  /**
   * @brief These are used when serializing.
   *
   * @return int
   */
  int get_format_major_version () const override { return 1; }
  int get_format_minor_version () const override { return 11; }

  /**
   * @return Whether positions were adjusted.
   */
  bool fix_audio_regions ();

  ArrangerSelections * get_arranger_selections_for_last_selection ();

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
  void save (
    const std::string &_dir,
    const bool         is_backup,
    const bool         show_notification,
    const bool         async);

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
   * Initializes the selections in the project.
   *
   * @param including_arranger_selections Whether to also initialize the
   * arranger selections to zero (sometimes we want to keep them, eg, when this
   * is called after deserialization of a project to load).
   */
  ATTR_COLD void init_selections (bool including_arranger_selections = true);

  /**
   * Compresses/decompress a project from a file/data to a file/data.
   *
   * @param compress True to compress, false to decompress.
   * @param[out] _dest Pointer to a location to allocate memory.
   * @param[out] _dest_size Pointer to a location to store the size of the
   * allocated memory.
   * @param _src Input buffer or filepath.
   * @param _src_size Input buffer size, if not filepath.
   *
   * @throw ZrythmException If the compression/decompression fails.
   */
  static void compress_or_decompress (
    bool                   compress,
    char **                _dest,
    size_t *               _dest_size,
    ProjectCompressionFlag dest_type,
    const char *           _src,
    const size_t           _src_size,
    ProjectCompressionFlag src_type);

  static void compress (
    char **                _dest,
    size_t *               _dest_size,
    ProjectCompressionFlag dest_type,
    const char *           _src,
    const size_t           _src_size,
    ProjectCompressionFlag src_type)
  {
    compress_or_decompress (
      true, _dest, _dest_size, dest_type, _src, _src_size, src_type);
  }

  static void decompress (
    char **                _dest,
    size_t *               _dest_size,
    ProjectCompressionFlag dest_type,
    const char *           _src,
    const size_t           _src_size,
    ProjectCompressionFlag src_type)
  {
    compress_or_decompress (
      false, _dest, _dest_size, dest_type, _src, _src_size, src_type);
  }

  /**
   * Returns the uncompressed text representation of the saved project file.
   *
   * To be free'd with free().
   *
   * @param backup Whether to use the project file from the most recent
   * backup.
   *
   * @throw ZrythmException If an error occurs.
   */
  char * get_existing_uncompressed_text (bool backup);

  bool has_unsaved_changes () const;

  void init_after_cloning (const Project &other) override;

  /**
   * Deep-clones the given project.
   *
   * To be used during save on the main thread.
   *
   * @param for_backup Whether the resulting project is for a backup.
   *
   * @throw ZrythmException If an error occurs.
   */
  std::unique_ptr<Project> clone (bool for_backup) const
  {
    auto ret = clone_unique ();
    if (for_backup)
      {
        /* no undo history in backups */
        ret->undo_manager_.reset ();
      }
    return ret;
  }

  /**
   * Returns the filepath of a backup (directory), if any, if it has a newer
   * timestamp than main project file.
   *
   * Returns an empty string if there were errors or no backup was found.
   */
  std::string get_newer_backup ();

  /**
   * @brief Connects things up, exposes ports to the backend, calculates the
   * graph and begins processing.
   *
   */
  void activate ();

  /**
   * @brief Gets all the ports in the project.
   *
   * @param ports Array to append to.
   */
  void get_all_ports (std::vector<Port *> &ports) const;

  /**
   * @brief Adds the default undeletable tracks to the project.
   *
   * To be called when creating new projects.
   */
  void add_default_tracks ();

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
    std::unique_ptr<Project> project_;

    /** Full path to save to. */
    std::string project_file_path_;

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
   */
  static int idle_saved_callback (SaveContext * ctx);

public:
  /** Project title. */
  std::string title_;

  /** Datetime string to add to the project file. */
  std::string datetime_str_;

  /** Path to save the project in. */
  std::string dir_;

  /**
   * Backup dir to save the project during the current save call.
   *
   * For example, @ref Project.dir /backups/myproject.bak3.
   */
  std::string backup_dir_;

  /* !!! IMPORTANT: order matters (for destruction) !!! */

  UndoableAction * last_action_in_last_successful_autosave_ = nullptr;

  /** Last successful autosave timestamp. */
  SteadyTimePoint last_successful_autosave_time_;

  /** Used to check if the project has unsaved changes. */
  UndoableAction * last_saved_action_ = nullptr;

  /** Semaphore used to block saving. */
  std::binary_semaphore save_sem_{ 1 };

  /** Zrythm version, for serialization */
  std::string version_;

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
  Tool tool_ = Tool::Select;

  /**
   * Plugin selections in the Mixer.
   *
   * Must be free'd after port connections manager.
   */
  std::unique_ptr<ProjectMixerSelections> mixer_selections_;

  /**
   * @brief
   *
   * Must be free'd after engine.
   */
  std::unique_ptr<PortConnectionsManager> port_connections_manager_;

  /**
   * The audio backend.
   */
  std::unique_ptr<AudioEngine> audio_engine_;

  /** Zoom levels. TODO & move to clip_editor */
  double piano_roll_zoom_ = 0;
  double timeline_zoom_ = 0;

  /**
   * Selected MidiNote's in the MidiArrangerWidget.
   */
  std::unique_ptr<MidiSelections> midi_selections_;

  /**
   * Selected objects in the TimelineArrangerWidget.
   */
  std::unique_ptr<TimelineSelections> timeline_selections_;

  /**
   * Selected objects in the
   * ChordObjectArrangerWidget.
   */
  std::unique_ptr<ChordSelections> chord_selections_;

  /**
   * Selected objects in the audio editor.
   */
  std::unique_ptr<AudioSelections> audio_selections_;

  /**
   * Selected objects in the
   * AutomationArrangerWidget.
   */
  std::unique_ptr<AutomationSelections> automation_selections_;

  /**
   * Selected Track's.
   */
  std::unique_ptr<SimpleTracklistSelections> tracklist_selections_;

  /** Manager for region link groups. */
  RegionLinkGroupManager region_link_group_manager_;

  /** Quantize info for the piano roll. */
  std::unique_ptr<QuantizeOptions> quantize_opts_editor_;

  /** Quantize info for the timeline. */
  std::unique_ptr<QuantizeOptions> quantize_opts_timeline_;

  /** Snap/Grid info for the editor. */
  std::shared_ptr<SnapGrid> snap_grid_editor_;

  /** Snap/Grid info for the timeline. */
  std::shared_ptr<SnapGrid> snap_grid_timeline_;

  /** Timeline widget backend. */
  std::unique_ptr<Timeline> timeline_;

  /** Backend for the widget. */
  ClipEditor clip_editor_;

  /** MIDI bindings. */
  std::unique_ptr<MidiMappings> midi_mappings_;

  /**
   * @brief Tracklist.
   *
   * Shared with @ref tracklist_selections_.
   *
   * must be free'd before tracklist selections, mixer selections, engine,
   * and port connection manager
   */
  std::shared_ptr<Tracklist> tracklist_;

  std::unique_ptr<UndoManager> undo_manager_;

  /** Used when deserializing projects. */
  int format_major_ = 0;
  int format_minor_ = 0;
};

/**
 * @}
 */

#endif
