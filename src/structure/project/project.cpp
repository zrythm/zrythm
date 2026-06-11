// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "utils/format_qt.h"

#include "dsp/engine.h"
#include "dsp/hardware_midi_interface.h"
#include "dsp/juce_hardware_audio_interface.h"
#include "dsp/port_connections_manager.h"
#include "dsp/port_observer.h"
#include "dsp/transport.h"
#include "structure/project/project.h"
#include "structure/project/project_graph_builder.h"
#include "structure/project/project_path_provider.h"
#include "structure/tracks/tracklist.h"
#include "utils/app_settings.h"
#include "utils/logger.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace zrythm::structure::project
{

Project::Project (
  utils::AppSettings           &app_settings,
  ProjectDirectoryPathProvider  project_directory_path_provider,
  dsp::IHardwareAudioInterface &hw_interface,
  dsp::IHardwareMidiInterface  &midi_interface,
  std::shared_ptr<juce::AudioPluginFormatManager> plugin_format_manager,
  plugins::PluginHostWindowFactory                plugin_host_window_provider,
  dsp::Metronome                                 &metronome,
  dsp::Fader                                     &monitor_fader,
  QObject *                                       parent)
    : QObject (parent), app_settings_ (app_settings),
      tempo_map_ (hw_interface.get_device_info ().sample_rate),
      tempo_map_wrapper_ (new dsp::TempoMapWrapper (tempo_map_, this)),
      plugin_host_window_provider_ (std::move (plugin_host_window_provider)),
      project_registry_ (this),
      project_directory_path_provider_ (
        std::move (project_directory_path_provider)),
      hw_interface_ (hw_interface),
      plugin_format_manager_ (plugin_format_manager),
      port_connections_manager_ (new dsp::PortConnectionsManager (this)),
      transport_ (
        utils::make_qobject_unique<dsp::Transport> (
          tempo_map_,
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
        utils::make_qobject_unique<dsp::AudioEngine> (
          *transport_,
          hw_interface,
          midi_interface,
          graph_dispatcher_,
          tempo_map_,
          this)),
      pool_ (
        std::make_unique<dsp::AudioPool> (
          project_registry_,
          [this] (bool backup) {
            return project_directory_path_provider_ (backup)
                   / structure::project::ProjectPathProvider::get_path (
                     zrythm::structure::project::ProjectPathProvider::
                       ProjectPath::AudioFilePoolDir);
          },
          [this] () { return audio_engine_->sample_rate (); })),
      tracklist_ (
        utils::make_qobject_unique<
          structure::tracks::Tracklist> (project_registry_, this)),
      clip_launcher_ (
        utils::make_qobject_unique<structure::scenes::ClipLauncher> (
          project_registry_,
          *tracklist_->collection (),
          this)),
      clip_playback_service_ (
        utils::make_qobject_unique<structure::scenes::ClipPlaybackService> (
          project_registry_,
          *tracklist_->collection (),
          this)),
      arranger_object_factory_ (
        std::make_unique<structure::arrangement::ArrangerObjectFactory> (
          structure::arrangement::ArrangerObjectFactory::Dependencies{
            .tempo_map_ = tempo_map (),
            .registry_ = project_registry_,
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
          [&] () { return audio_engine_->sample_rate (); },
          [&] () { return tempo_map ().tempo_at_tick (units::ticks (0)); })),
      plugin_factory_ (
        utils::make_qobject_unique<plugins::PluginFactory> (
          plugins::PluginFactory::CommonFactoryDependencies{
            .registry = project_registry_,
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
              [this] () { return audio_engine_->sample_rate (); },
            .buffer_size_provider_ =
              [this] () { return audio_engine_->block_length (); },
            .top_level_window_provider_ = plugin_host_window_provider_,
          },
          this)),
      track_factory_ (std::make_unique<structure::tracks::TrackFactory> ([this] () {
        return get_final_track_dependencies ();
      })),
      tempo_object_manager_ (
        utils::make_qobject_unique<
          structure::arrangement::TempoObjectManager> (project_registry_, this)),
      monitor_fader_ (monitor_fader), metronome_ (metronome),
      port_observation_manager_ (
        utils::make_qobject_unique<
          dsp::PortObservationManager> (project_registry_, this)),
      fixed_graph_endpoints_ (
        std::views::single (
          static_cast<dsp::graph::IProcessable *> (
            &monitor_fader.get_stereo_out_port ()))
        | std::ranges::to<std::vector> ()),
      playback_graph_endpoints_ (fixed_graph_endpoints_),
      graph_dispatcher_ (
        std::unique_ptr<dsp::graph::IGraphBuilder> (
          new ProjectGraphBuilder (*this, metronome, monitor_fader)),
        [this] () { return std::span (playback_graph_endpoints_); },
        hw_interface_,
        [this] (const std::function<void ()> &callable) {
          bool engine_running = audio_engine_->running ();
          audio_engine_->set_running (false);
          auto lock = audio_engine_->get_processing_lock ();
          callable ();
          audio_engine_->set_running (engine_running);
        },
        [context = this] (std::function<void ()> func) {
          QMetaObject::invokeMethod (context, func);
        },
        [&] () -> std::optional<juce::AudioWorkgroup> {
          auto * juce_hw =
            dynamic_cast<dsp::JuceHardwareAudioInterface *> (&hw_interface_);
          return juce_hw ? juce_hw->get_device_audio_workgroup () : std::nullopt;
        }())
{
  audio_engine_->set_monitor_out_source (monitor_fader_.get_stereo_out_port ());

  QObject::connect (
    audio_engine_.get (), &dsp::AudioEngine::sampleRateChanged,
    tempo_map_wrapper_.get (), [this] (int new_rate) {
      tempo_map_wrapper_->setSampleRate (static_cast<double> (new_rate));
    });

  QObject::connect (
    port_observation_manager_.get (),
    &dsp::PortObservationManager::observationChanged, this, [this] () {
      playback_graph_endpoints_ = fixed_graph_endpoints_;
      for (auto * observer : port_observation_manager_->observers ())
        {
          playback_graph_endpoints_.push_back (observer);
        }
      graph_dispatcher_.recalc_graph (false);
    });

  // Keep up-to-date realtime cache of tracks
  // Note: this is thread-safe since tracks are only added/removed while the
  // graph is paused
  QObject::connect (
    tracklist_->collection (), &structure::tracks::TrackCollection::rowsInserted,
    this, [this] (const QModelIndex &, int first, int last) {
      for (int i = first; i <= last; ++i)
        {
          const auto &track = tracklist_->collection ()->tracks ().at (i);
          tracks_rt_.push_back (track.get ());
        }
    });
  QObject::connect (
    tracklist_->collection (),
    &structure::tracks::TrackCollection::rowsAboutToBeRemoved, this,
    [this] (const QModelIndex &, int first, int last) {
      for (int i = first; i <= last; ++i)
        {
          const auto &track = tracklist_->collection ()->tracks ().at (i);
          auto *      ptr = track.get ();
          std::erase (tracks_rt_, ptr);
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

Project::~Project ()
{
  if (audio_engine_->activated ())
    {
      audio_engine_->deactivate ();
    }
}

dsp::Fader &
Project::monitor_fader () const
{
  return monitor_fader_;
}

dsp::Metronome &
Project::metronome () const
{
  return metronome_;
}

void
Project::install_recording_callback (
  structure::tracks::TrackRecordingCallback callback)
{
  if (track_recording_callback_)
    {
      throw std::runtime_error (
        "install_recording_callback called more than once");
    }
  track_recording_callback_ = std::move (callback);
}

structure::tracks::FinalTrackDependencies
Project::get_final_track_dependencies ()
{
  if (!track_recording_callback_)
    {
      throw std::runtime_error (
        "Recording callback not installed — call install_recording_callback() first");
    }
  return structure::tracks::FinalTrackDependencies{
    *tempo_map_wrapper_, project_registry_, *transport_,
    [this] () {
      return std::ranges::any_of (tracks_rt_, [] (const auto * track) {
        const auto * ch = track->channel ();
        if (ch == nullptr)
          {
            return false;
          }
        return ch->fader ()->currently_soloed_rt ();
      });
    },
    track_recording_callback_
  };
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

  /* chord track MIDI expansion wiring is done in ProjectSession after
   * ProjectUiState is constructed. */

  /* add a scale */
  auto scale_ref =
    arranger_object_factory_->get_builder<structure::arrangement::ScaleObject> ()
      .with_start_ticks (units::ticks (0))
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
            .with_start_ticks (units::ticks (0))
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
            .with_start_ticks (pos)
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

void
init_from (Project &obj, const Project &other, utils::ObjectCloneType clone_type)
{
  z_debug ("cloning project...");

  // obj.transport_ = utils::clone_qobject (*other.transport_, &obj);
  // obj.audio_engine_ = utils::clone_qobject (
  //   *other.audio_engine_, &obj, clone_type, &obj, obj.device_manager_);
  obj.pool_ = utils::clone_unique (
    *other.pool_, clone_type, obj.project_registry_,
    [&obj] (bool backup) {
      return obj.project_directory_path_provider_ (backup)
             / structure::project::ProjectPathProvider::get_path (
               structure::project::ProjectPathProvider::ProjectPath::
                 AudioFilePoolDir);
    },
    [&obj] () { return obj.audio_engine_->sample_rate (); });
  obj.tracklist_ = utils::clone_qobject (
    *other.tracklist_, &obj, clone_type, obj.project_registry_);
  obj.port_connections_manager_ =
    utils::clone_qobject (*other.port_connections_manager_, &obj);

  z_debug ("finished cloning project");
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

dsp::PortObservationManager *
Project::portObservationManager () const
{
  return port_observation_manager_.get ();
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

void
to_json (nlohmann::json &j, const Project &project)
{
  j[Project::kTempoMapKey] = project.tempo_map_;
  j[Project::kTransportKey] = project.transport_;
  j[Project::kAudioPoolKey] = project.pool_;
  j[Project::kTracklistKey] = project.tracklist_;
  // j[Project::kRegionLinkGroupManagerKey] =
  // project.region_link_group_manager_;
  j[Project::kPortConnectionsManagerKey] = project.port_connections_manager_;
  j[Project::kTempoObjectManagerKey] = project.tempo_object_manager_;
  j[Project::kClipLauncherKey] = project.clip_launcher_;

  j[Project::kRegistryKey] = project.project_registry_;
}

void
from_json (const nlohmann::json &j, Project &project)
{
  j.at (Project::kTempoMapKey).get_to (project.tempo_map_);

  project.project_registry_.set_deserialization_dependencies (
    {
      *project.track_factory_,
      *project.arranger_object_factory_,
      *project.plugin_factory_,
    });
  j.at (Project::kRegistryKey).get_to (project.project_registry_);

  // Transport is required
  j.at (Project::kTransportKey).get_to (*project.transport_);

  // AudioPool - optional
  if (j.contains (Project::kAudioPoolKey))
    {
      j.at (Project::kAudioPoolKey).get_to (*project.pool_);
    }

  // Tracklist is required
  j.at (Project::kTracklistKey).get_to (*project.tracklist_);

  // Optional fields
  // j.at (Project::kRegionLinkGroupManagerKey)
  //   .get_to (project.region_link_group_manager_);
  if (j.contains (Project::kPortConnectionsManagerKey))
    {
      j.at (Project::kPortConnectionsManagerKey)
        .get_to (*project.port_connections_manager_);
    }
  if (j.contains (Project::kTempoObjectManagerKey))
    {
      j.at (Project::kTempoObjectManagerKey)
        .get_to (*project.tempo_object_manager_);
    }
  if (j.contains (Project::kClipLauncherKey))
    {
      j.at (Project::kClipLauncherKey).get_to (*project.clip_launcher_);
    }

  /* chord track MIDI expansion wiring is done in ProjectSession after
   * ProjectUiState is deserialized. */
}
}
