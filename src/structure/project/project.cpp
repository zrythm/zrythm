// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <filesystem>
#include <utility>

#include "dsp/engine.h"
#include "dsp/port_connections_manager.h"
#include "dsp/transport.h"
#include "structure/project/project.h"
#include "structure/project/project_graph_builder.h"
#include "structure/project/project_path_provider.h"
#include "structure/tracks/tracklist.h"
#include "utils/logger.h"

#include <juce_wrapper.h>

namespace zrythm::structure::project
{

Project::Project (
  utils::AppSettings                             &app_settings,
  std::shared_ptr<juce::AudioDeviceManager>       device_manager,
  std::shared_ptr<juce::AudioPluginFormatManager> plugin_format_manager,
  plugins::PluginHostWindowFactory                plugin_host_window_provider,
  dsp::Fader                                     &monitor_fader,
  QObject *                                       parent)
    : QObject (parent), app_settings_ (app_settings),
      tempo_map_ (
        units::sample_rate (
          device_manager->getCurrentAudioDevice ()->getCurrentSampleRate ())),
      tempo_map_wrapper_ (new dsp::TempoMapWrapper (tempo_map_, this)),
      plugin_host_window_provider_ (std::move (plugin_host_window_provider)),
      file_audio_source_registry_ (new dsp::FileAudioSourceRegistry (this)),
      port_registry_ (new dsp::PortRegistry (this)),
      param_registry_ (
        new dsp::ProcessorParameterRegistry (*port_registry_, this)),
      plugin_registry_ (new PluginRegistry (this)),
      arranger_object_registry_ (
        new structure::arrangement::ArrangerObjectRegistry (this)),
      track_registry_ (new structure::tracks::TrackRegistry (this)),
      device_manager_ (device_manager),
      plugin_format_manager_ (plugin_format_manager),
      port_connections_manager_ (new dsp::PortConnectionsManager (this)),
      snap_grid_editor_ (
        utils::make_qobject_unique<dsp::SnapGrid> (
          tempo_map_,
          utils::NoteLength::Note_1_8,
          [this] {
            return app_settings_.editorLastCreatedObjectLengthInTicks ();
},
          this)),
      snap_grid_timeline_ (
        utils::make_qobject_unique<dsp::SnapGrid> (
          tempo_map_,
          utils::NoteLength::Bar,
          [this] {
            return app_settings_.timelineLastCreatedObjectLengthInTicks ();
          },
          this)),
      transport_ (
        utils::make_qobject_unique<dsp::Transport> (
          tempo_map_,
          *snap_grid_timeline_,
          dsp::Transport::ConfigProvider{
            .return_to_cue_on_pause_ =
              [this] () { return app_settings_.transportReturnToCue (); },
            .metronome_countin_bars_ =
              [this] () { return app_settings_.metronomeCountIn (); },
            .recording_preroll_bars_ =
              [this] () { return app_settings_.recordingPreroll (); },
          },
          this)),
      audio_engine_ (
        utils::make_qobject_unique<
          dsp::AudioEngine> (*transport_, device_manager, graph_dispatcher_, this)),
      pool_ (
        std::make_unique<dsp::AudioPool> (
          *file_audio_source_registry_,
          [this] (bool backup) {
            return get_directory (backup)
                   / gui::ProjectPathProvider::get_path (
                     zrythm::gui::ProjectPathProvider::ProjectPath::
                       AudioFilePoolDir);
          },
          [this] () { return audio_engine_->get_sample_rate (); })),
      tracklist_ (
        utils::make_qobject_unique<
          structure::tracks::Tracklist> (*track_registry_, this)),
      clip_launcher_ (
        utils::make_qobject_unique<structure::scenes::ClipLauncher> (
          *arranger_object_registry_,
          *tracklist_->collection (),
          this)),
      clip_playback_service_ (
        utils::make_qobject_unique<structure::scenes::ClipPlaybackService> (
          *arranger_object_registry_,
          *tracklist_->collection (),
          this)),
      arranger_object_factory_ (
        std::make_unique<structure::arrangement::ArrangerObjectFactory> (
          structure::arrangement::ArrangerObjectFactory::Dependencies{
            .tempo_map_ = get_tempo_map (),
            .object_registry_ = *arranger_object_registry_,
            .file_audio_source_registry_ = *file_audio_source_registry_,
            .musical_mode_getter_ =
              [this] () { return app_settings_.musicalMode (); },
            .last_timeline_obj_len_provider_ =
              [this] () {
                return app_settings_.timelineLastCreatedObjectLengthInTicks ();
              },
            .last_editor_obj_len_provider_ =
              [this] () {
                return app_settings_.editorLastCreatedObjectLengthInTicks ();
              },
            .automation_curve_algorithm_provider_ =
              [this] () {
                return ENUM_INT_TO_VALUE (
                  dsp::CurveOptions::Algorithm,
                  app_settings_.automationCurveAlgorithm ());
              } },
          [&] () { return audio_engine_->get_sample_rate (); },
          [&] () { return get_tempo_map ().tempo_at_tick (units::ticks (0)); })),
      plugin_factory_ (
        utils::make_qobject_unique<plugins::PluginFactory> (
          plugins::PluginFactory::CommonFactoryDependencies{
            .plugin_registry_ = *plugin_registry_,
            .processor_base_dependencies_ =
              dsp::ProcessorBase::ProcessorBaseDependencies{
                .port_registry_ = *port_registry_,
                .param_registry_ = *param_registry_ },
            .state_dir_path_provider_ =
              [this] () {
                return dir_
                       / gui::ProjectPathProvider::get_path (
                         zrythm::gui::ProjectPathProvider::ProjectPath::
                           PluginStates);
              },
            .create_plugin_instance_async_func_ =
              [plugin_format_manager] (
                const juce::PluginDescription &description,
                double                         initialSampleRate,
                int                            initialBufferSize,
                juce::AudioPluginFormat::PluginCreationCallback callback) {
                plugin_format_manager->createPluginInstanceAsync (
                  description, initialSampleRate, initialBufferSize,
                  std::move (callback));
              },
            .sample_rate_provider_ =
              [this] () { return audio_engine_->get_sample_rate (); },
            .buffer_size_provider_ =
              [this] () { return audio_engine_->get_block_length (); },
            .top_level_window_provider_ = plugin_host_window_provider_,
            .audio_thread_checker_ =
              [this] () {
                return audio_engine_->graph_dispatcher ().is_processing_thread ();
              } },
          this)),
      track_factory_ (
        std::make_unique<structure::tracks::TrackFactory> (
          get_final_track_dependencies ())),
      tempo_object_manager_ (
        utils::make_qobject_unique<structure::arrangement::TempoObjectManager> (
          *arranger_object_registry_,
          *file_audio_source_registry_,
          this)),
      monitor_fader_ (monitor_fader),
      graph_dispatcher_ (
        std::unique_ptr<dsp::graph::IGraphBuilder> (
          new ProjectGraphBuilder (*this)),
        std::views::single (&audio_engine_->get_monitor_out_port ())
          | std::ranges::to<std::vector<dsp::graph::IProcessable *>> (),
        *device_manager_,
        [this] (const std::function<void ()> &callable) {
          bool engine_running = audio_engine_->running ();
          audio_engine_->set_running (false);
          auto lock = audio_engine_->get_processing_lock ();
          callable ();
          audio_engine_->set_running (engine_running);
        },
        [context = this] (std::function<void ()> func) {
          QMetaObject::invokeMethod (context, func);
        })
{
  const auto setup_metronome = [&] () {
    const auto load_metronome_sample = [] (QFile f) {
      if (!f.open (QFile::ReadOnly | QFile::Text))
        {
          throw std::runtime_error (
            fmt::format ("Failed to open file at {}", f.fileName ()));
        }
      const QByteArray                         wavBytes = f.readAll ();
      std::unique_ptr<juce::AudioFormatReader> reader;
      {
        auto stream = std::make_unique<juce::MemoryInputStream> (
          wavBytes.constData (), static_cast<size_t> (wavBytes.size ()), false);
        juce::WavAudioFormat wavFormat;
        reader.reset (wavFormat.createReaderFor (stream.release (), false));
      }
      if (!reader)
        throw std::runtime_error ("Not a valid WAV");

      const int numChannels = static_cast<int> (reader->numChannels);
      const int numSamples = static_cast<int> (reader->lengthInSamples);

      juce::AudioBuffer<float> buffer;
      buffer.setSize (numChannels, numSamples);

      reader->read (&buffer, 0, numSamples, 0, true, numChannels > 1);
      return buffer;
    };
    metronome_ = utils::make_qobject_unique<dsp::Metronome> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
      get_tempo_map (),
      load_metronome_sample (
        QFile (u":/qt/qml/Zrythm/wav/square_emphasis.wav"_s)),
      load_metronome_sample (QFile (u":/qt/qml/Zrythm/wav/square_normal.wav"_s)),
      app_settings_.metronomeEnabled (), app_settings_.metronomeVolume (), this);
    QObject::connect (
      metronome_.get (), &dsp::Metronome::volumeChanged, &app_settings_,
      [this] (float val) { app_settings_.set_metronomeVolume (val); });
    QObject::connect (
      &app_settings_, &utils::AppSettings::metronomeVolumeChanged,
      metronome_.get (), [this] (float val) { metronome_->setVolume (val); });
    QObject::connect (
      metronome_.get (), &dsp::Metronome::enabledChanged, &app_settings_,
      [this] (bool val) { app_settings_.set_metronomeEnabled (val); });
    QObject::connect (
      &app_settings_, &utils::AppSettings::metronomeEnabledChanged,
      metronome_.get (), [this] (bool val) { metronome_->setEnabled (val); });
  };
  setup_metronome ();

  // Keep up-to-date realtime cache of tracks
  // Note: this is thread-safe since tracks are only added/removed while the
  // graph is paused
  QObject::connect (
    tracklist_->collection (), &structure::tracks::TrackCollection::rowsInserted,
    this, [this] (const QModelIndex &, int first, int last) {
      for (int i = first; i <= last; ++i)
        {
          const auto &track = tracklist_->collection ()->tracks ().at (i);
          const auto  track_id = track.id ();
          tracks_rt_[track_id] = track.get_object ();
        }
    });
  QObject::connect (
    tracklist_->collection (),
    &structure::tracks::TrackCollection::rowsAboutToBeRemoved, this,
    [this] (const QModelIndex &, int first, int last) {
      for (int i = first; i <= last; ++i)
        {
          const auto track_id =
            tracklist_->collection ()->tracks ().at (i).id ();
          tracks_rt_.erase (track_id);
        }
    });

  // Sync changes from tempo-related arranger objects to tempo map
  const auto rebuild_tempo_map = [this] () {
    // This must never be called while the engine is running
    assert (!audio_engine_->running ());

    tempo_map_wrapper_->clearTimeSignatureEvents ();
    tempo_map_wrapper_->clearTempoEvents ();
    for (
      const auto * tempo_obj :
      tempo_object_manager_->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::TimeSignatureObject>::get_sorted_children_view ())
      {
        tempo_map_wrapper_->addTimeSignatureEvent (
          static_cast<int64_t> (std::round (tempo_obj->position ()->ticks ())),
          tempo_obj->numerator (), tempo_obj->denominator ());
      }
    for (
      const auto * tempo_obj :
      tempo_object_manager_->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::TempoObject>::get_sorted_children_view ())
      {
        tempo_map_wrapper_->addTempoEvent (
          static_cast<int64_t> (std::round (tempo_obj->position ()->ticks ())),
          tempo_obj->tempo (), tempo_obj->curve ());
      }
  };
  QObject::connect (
    tempo_object_manager_->tempoObjects (),
    &structure::arrangement::ArrangerObjectListModel::contentChanged,
    tempo_map_wrapper_.get (), rebuild_tempo_map);
  QObject::connect (
    tempo_object_manager_->timeSignatureObjects (),
    &structure::arrangement::ArrangerObjectListModel::contentChanged,
    tempo_map_wrapper_.get (), rebuild_tempo_map);
}

Project::~Project () = default;

dsp::Fader &
Project::monitor_fader ()
{
  return monitor_fader_;
}

structure::tracks::FinalTrackDependencies
Project::get_final_track_dependencies () const
{
  return structure::tracks::FinalTrackDependencies{
    *tempo_map_wrapper_,
    *file_audio_source_registry_,
    *plugin_registry_,
    *port_registry_,
    *param_registry_,
    *arranger_object_registry_,
    *track_registry_,
    *transport_,
    [this] () {
      return std::ranges::any_of (tracks_rt_, [] (const auto &track_var) {
        const auto * track = structure::tracks::from_variant (track_var.second);
        const auto * ch = track->channel ();
        if (ch == nullptr)
          {
            return false;
          }
        return ch->fader ()->currently_soloed_rt ();
      });
    }
  };
}

std::optional<fs::path>
Project::get_newer_backup ()
{
  // TODO
  return std::nullopt;
#if 0
  const auto filepath = get_path (ProjectPath::ProjectFile, false);
  z_return_val_if_fail (!filepath.empty (), std::nullopt);

  std::filesystem::file_time_type original_time;
  if (std::filesystem::exists (filepath))
    {
      original_time = std::filesystem::last_write_time (filepath);
    }
  else
    {
      z_warning ("Failed to get last modified for {}", filepath);
      return std::nullopt;
    }

  fs::path   result;
  const auto backups_dir = get_path (ProjectPath::BACKUPS, false);
  try
    {
      for (const auto &entry : std::filesystem::directory_iterator (backups_dir))
        {
          auto full_path = entry.path () / PROJECT_FILE;
          z_debug ("{}", full_path);

          if (std::filesystem::exists (full_path))
            {
              auto backup_time = std::filesystem::last_write_time (full_path);
              if (backup_time > original_time)
                {
                  result = entry.path ();
                  original_time = backup_time;
                }
            }
          else
            {
              z_warning ("Failed to get last modified for {}", full_path);
              return std::nullopt;
            }
        }
    }
  catch (const std::filesystem::filesystem_error &e)
    {
      z_warning ("Error accessing backup directory: {}", e.what ());
      return std::nullopt;
    }

  return result;
#endif
}

void
Project::activate ()
{
  z_debug ("Activating project {} ({:p})...", title_, fmt::ptr (this));

  // TODO
  // last_saved_action_ = legacy_undo_manager_->get_last_action ();

  audio_engine_->activate ();

  // Pause and recalculate the graph
  audio_engine_->execute_function_with_paused_processing_synchronously (
    [] () { }, true);

  z_debug ("Project {} ({:p}) activated", title_, fmt::ptr (this));
}

void
Project::add_default_tracks ()
{
  using namespace zrythm::structure::tracks;
  z_return_if_fail (tracklist_);

  /* init pinned tracks */

  auto add_track =
    [&]<structure::tracks::FinalTrackSubclass TrackT> (const auto &name) {
      z_info ("Adding '{}' track", name);
      auto track_ref = track_factory_->create_empty_track<TrackT> ();
      track_ref.template get_object_as<TrackT> ()->setName (name);
      tracklist_->collection ()->add_track (track_ref);

      return track_ref.template get_object_as<TrackT> ();
    };

  /* chord */
  auto * chord_track = add_track.operator()<ChordTrack> (QObject::tr ("Chords"));
  tracklist_->singletonTracks ()->setChordTrack (chord_track);
// TODO
#if 0
  chord_track->set_note_pitch_to_descriptor_func ([this] (midi_byte_t note_pitch) {
    return getClipEditor ()->getChordEditor ()->get_chord_from_note_number (
      note_pitch);
  });
#endif

  /* add a scale */
  auto scale_ref =
    arranger_object_factory_->get_builder<structure::arrangement::ScaleObject> ()
      .with_start_ticks (0)
      .with_scale (
        utils::make_qobject_unique<dsp::MusicalScale> (
          dsp::MusicalScale::ScaleType::Aeolian, dsp::MusicalNote::A))
      .build_in_registry ();
  chord_track->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::ScaleObject>::add_object (scale_ref);

  /* modulator */
  auto * modulator_track =
    add_track.operator()<ModulatorTrack> (QObject::tr ("Modulators"));
  tracklist_->singletonTracks ()->setModulatorTrack (modulator_track);

  /* add marker track and default markers */
  auto * marker_track =
    add_track.operator()<MarkerTrack> (QObject::tr ("Markers"));
  tracklist_->singletonTracks ()->setMarkerTrack (marker_track);
  const auto add_default_markers =
    [] (auto &marker_track_inner, const auto &factory, const auto &tempo_map) {
      {
        auto marker_name = fmt::format ("[{}]", QObject::tr ("start"));
        auto marker_ref =
          factory->template get_builder<structure::arrangement::Marker> ()
            .with_start_ticks (0)
            .with_name (
              utils::Utf8String::from_utf8_encoded_string (marker_name)
                .to_qstring ())
            .with_marker_type (structure::arrangement::Marker::MarkerType::Start)
            .build_in_registry ();
        marker_track_inner->add_object (marker_ref);
      }

      {
        auto       marker_name = fmt::format ("[{}]", QObject::tr ("end"));
        const auto pos = tempo_map.musical_position_to_tick (
          { .bar = 129, .beat = 1, .sixteenth = 1, .tick = 0 });
        auto marker_ref =
          factory->template get_builder<structure::arrangement::Marker> ()
            .with_start_ticks (pos.in (units::ticks))
            .with_name (
              utils::Utf8String::from_utf8_encoded_string (marker_name)
                .to_qstring ())
            .with_marker_type (structure::arrangement::Marker::MarkerType::End)
            .build_in_registry ();
        marker_track_inner->add_object (marker_ref);
      }
    };
  add_default_markers (marker_track, arranger_object_factory_, tempo_map_);

  tracklist_->setPinnedTracksCutoff (
    static_cast<int> (tracklist_->collection ()->track_count ()));

  /* add master track */
  auto * master_track =
    add_track.operator()<MasterTrack> (QObject::tr ("Master"));
  tracklist_->singletonTracks ()->setMasterTrack (master_track);
}

fs::path
Project::get_directory (bool for_backup) const
{
  return for_backup ? *backup_dir_ : dir_;
}

void
init_from (Project &obj, const Project &other, utils::ObjectCloneType clone_type)
{
  z_return_if_fail (ZRYTHM_IS_QT_THREAD);
  z_debug ("cloning project...");

  obj.title_ = other.title_;
  // obj.transport_ = utils::clone_qobject (*other.transport_, &obj);
  // obj.audio_engine_ = utils::clone_qobject (
  //   *other.audio_engine_, &obj, clone_type, &obj, obj.device_manager_);
  obj.pool_ = utils::clone_unique (
    *other.pool_, clone_type, *obj.file_audio_source_registry_,
    [&obj] (bool backup) {
      return obj.get_directory (backup)
             / gui::ProjectPathProvider::get_path (
               gui::ProjectPathProvider::ProjectPath::AudioFilePoolDir);
    },
    [&obj] () { return obj.audio_engine_->get_sample_rate (); });
  obj.tracklist_ = utils::clone_qobject (
    *other.tracklist_, &obj, clone_type, *obj.track_registry_, &obj);
  obj.port_connections_manager_ =
    utils::clone_qobject (*other.port_connections_manager_, &obj);

  z_debug ("finished cloning project");
}

QString
Project::getTitle () const
{
  return title_;
}

void
Project::setTitle (const QString &title)
{
  const auto std_str = utils::Utf8String::from_qstring (title);
  if (title_ == std_str)
    return;

  title_ = std_str;
  Q_EMIT titleChanged (title);
}

QString
Project::directory () const
{
  return utils::Utf8String::from_path (dir_);
}

void
Project::setDirectory (const QString &directory)
{
  const auto dir_path = utils::Utf8String::from_qstring (directory).to_path ();
  if (dir_ == dir_path)
    return;

  dir_ = dir_path;
  Q_EMIT directoryChanged (directory);
}

structure::tracks::Tracklist *
Project::tracklist () const
{
  return tracklist_.get ();
}

structure::scenes::ClipLauncher *
Project::clipLauncher () const
{
  return clip_launcher_.get ();
}

structure::scenes::ClipPlaybackService *
Project::clipPlaybackService () const
{
  return clip_playback_service_.get ();
}

dsp::Metronome *
Project::metronome () const
{
  return metronome_.get ();
}

dsp::Transport *
Project::getTransport () const
{
  return transport_.get ();
}

dsp::AudioEngine *
Project::engine () const
{
  return audio_engine_.get ();
}

dsp::TempoMapWrapper *
Project::getTempoMap () const
{
  return tempo_map_wrapper_.get ();
}

structure::arrangement::TempoObjectManager *
Project::tempoObjectManager () const
{
  return tempo_object_manager_.get ();
}

dsp::SnapGrid *
Project::snapGridTimeline () const
{
  return snap_grid_timeline_.get ();
}

dsp::SnapGrid *
Project::snapGridEditor () const
{
  return snap_grid_editor_.get ();
}

#if 0
Project *
Project::get_active_instance ()
{
  auto * ui_state =
    zrythm::gui::ProjectManager::get_instance ()->activeProject ();
  return ui_state != nullptr ? ui_state->project () : nullptr;
}
#endif

Project *
Project::clone (bool for_backup) const
{
// TODO
#if 0
  auto * ret = utils::clone_raw_ptr (
    *this, utils::ObjectCloneType::Snapshot, app_settings_, device_manager_,
    plugin_format_manager_, plugin_host_window_provider_, monitor_fader_,
    version_);
  if (for_backup)
    {
      /* no undo history in backups */
      if (ret->legacy_undo_manager_ != nullptr)
        {
          delete ret->legacy_undo_manager_;
          ret->legacy_undo_manager_ = nullptr;
        }
    }
  return ret;
#endif
  return nullptr;
}

void
to_json (nlohmann::json &j, const Project &project)
{
  j[Project::kTempoMapKey] = project.tempo_map_;
  j[Project::kFileAudioSourceRegistryKey] = project.file_audio_source_registry_;
  j[Project::kPortRegistryKey] = project.port_registry_;
  j[Project::kParameterRegistryKey] = project.param_registry_;
  j[Project::kPluginRegistryKey] = project.plugin_registry_;
  j[Project::kArrangerObjectRegistryKey] = project.arranger_object_registry_;
  j[Project::kTrackRegistryKey] = project.track_registry_;
  j[Project::kTitleKey] = project.title_;
  j[Project::kSnapGridTimelineKey] = project.snap_grid_timeline_;
  j[Project::kSnapGridEditorKey] = project.snap_grid_editor_;
  j[Project::kTransportKey] = project.transport_;
  j[Project::kAudioPoolKey] = project.pool_;
  j[Project::kTracklistKey] = project.tracklist_;
  // j[Project::kRegionLinkGroupManagerKey] =
  // project.region_link_group_manager_;
  j[Project::kPortConnectionsManagerKey] = project.port_connections_manager_;
  j[Project::kTempoObjectManagerKey] = project.tempo_object_manager_;
}

struct ArrangerObjectBuilderForDeserialization
{
  ArrangerObjectBuilderForDeserialization (const Project &project)
      : project_ (project)
  {
  }

  template <typename T> std::unique_ptr<T> build () const
  {
    return project_.arrangerObjectFactory ()->get_builder<T> ().build_empty ();
  }

  const Project &project_;
};

struct TrackBuilderForDeserialization
{
  TrackBuilderForDeserialization (const Project &project) : project_ (project)
  {
  }

  template <typename T> std::unique_ptr<T> build () const
  {
    return project_.track_factory_->get_builder<T> ()
      .build_for_deserialization ();
  }

  const Project &project_;
};

struct PluginBuilderForDeserialization
{
  PluginBuilderForDeserialization (const Project &project) : project_ (project)
  {
  }
  template <typename T> std::unique_ptr<T> build () const
  {
    if constexpr (std::derived_from<T, plugins::InternalPluginBase>)
      {
        return std::make_unique<T> (
          plugins::Plugin::ProcessorBaseDependencies{
            .port_registry_ = project_.get_port_registry (),
            .param_registry_ = project_.get_param_registry () },
          [this] () {
            return project_.dir_
                   / gui::ProjectPathProvider::get_path (
                     gui::ProjectPathProvider::ProjectPath::PluginStates);
          });
      }
    else if constexpr (std::is_same_v<T, plugins::ClapPlugin>)
      {
        return std::make_unique<T> (
          plugins::Plugin::ProcessorBaseDependencies{
            .port_registry_ = project_.get_port_registry (),
            .param_registry_ = project_.get_param_registry () },
          [this] () {
            return project_.dir_
                   / gui::ProjectPathProvider::get_path (
                     gui::ProjectPathProvider::ProjectPath::PluginStates);
          },
          [this] () {
            return project_.audio_engine_->graph_dispatcher ()
              .is_processing_thread ();
          },
          project_.plugin_host_window_provider_);
      }
    else
      {
        return std::make_unique<T> (
          plugins::Plugin::ProcessorBaseDependencies{
            .port_registry_ = project_.get_port_registry (),
            .param_registry_ = project_.get_param_registry () },
          [this] () {
            return project_.dir_
                   / gui::ProjectPathProvider::get_path (
                     gui::ProjectPathProvider::ProjectPath::PluginStates);
          },
          [this] (
            const juce::PluginDescription &description,
            double initialSampleRate, int initialBufferSize,
            juce::AudioPluginFormat::PluginCreationCallback callback) {
            project_.plugin_format_manager_->createPluginInstanceAsync (
              description, initialSampleRate, initialBufferSize, callback);
          },
          [this] () { return project_.audio_engine_->get_sample_rate (); },
          [this] () { return project_.audio_engine_->get_block_length (); },
          project_.plugin_host_window_provider_);
      }
  }

  const Project &project_;
};

void
from_json (const nlohmann::json &j, Project &project)
{
  j.at (utils::serialization::kFormatMajorKey).get_to (project.format_major_);
  j.at (utils::serialization::kFormatMinorKey).get_to (project.format_minor_);
  j.at (Project::kTempoMapKey).get_to (project.tempo_map_);
  j.at (Project::kPortRegistryKey).get_to (*project.port_registry_);
  j.at (Project::kParameterRegistryKey).get_to (*project.param_registry_);
  from_json_with_builder (
    j.at (Project::kPluginRegistryKey), *project.plugin_registry_,
    PluginBuilderForDeserialization{ project });
  from_json_with_builder (
    j.at (Project::kArrangerObjectRegistryKey),
    *project.arranger_object_registry_,
    ArrangerObjectBuilderForDeserialization{ project });
  from_json_with_builder (
    j.at (Project::kTrackRegistryKey), *project.track_registry_,
    TrackBuilderForDeserialization{ project });
  j.at (Project::kTitleKey).get_to (project.title_);
  j.at (Project::kSnapGridTimelineKey).get_to (*project.snap_grid_timeline_);
  j.at (Project::kSnapGridEditorKey).get_to (*project.snap_grid_editor_);
  j.at (Project::kTransportKey).get_to (*project.transport_);
  project.pool_ = std::make_unique<dsp::AudioPool> (
    *project.file_audio_source_registry_,
    [&project] (bool backup) {
      return project.get_directory (backup)
             / gui::ProjectPathProvider::get_path (
               gui::ProjectPathProvider::ProjectPath::AudioFilePoolDir);
    },
    [&project] () { return project.audio_engine_->get_sample_rate (); });
  j.at (Project::kAudioPoolKey).get_to (*project.pool_);
  j.at (Project::kTracklistKey).get_to (*project.tracklist_);
  // j.at (Project::kRegionLinkGroupManagerKey)
  //   .get_to (project.region_link_group_manager_);
  j.at (Project::kPortConnectionsManagerKey)
    .get_to (*project.port_connections_manager_);
  j.at (Project::kTempoObjectManagerKey).get_to (*project.tempo_object_manager_);

// TODO
#if 0
  project.tracklist_->singletonTracks ()
    ->chordTrack ()
    ->set_note_pitch_to_descriptor_func ([&project] (midi_byte_t note_pitch) {
      return project.getClipEditor ()
        ->getChordEditor ()
        ->get_chord_from_note_number (note_pitch);
    });
#endif
}
}
