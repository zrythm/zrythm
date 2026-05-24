// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <fmt/std.h>

#include "controllers/project_saver.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/project_session.h"
#include "gui/dsp/quantize_options.h"
#include "structure/project/project_path_provider.h"
#include "utils/app_settings.h"
#include "utils/io_utils.h"

#include <QPointer>
#include <QQmlEngine>

namespace zrythm::gui
{

ProjectSession::ProjectSession (
  utils::AppSettings                                    &app_settings,
  utils::QObjectUniquePtr<structure::project::Project> &&project)
    : app_settings_ (app_settings), project_ (std::move (project)),
      ui_state_ (
        utils::make_qobject_unique<
          structure::project::ProjectUiState> (*project_, app_settings_)),
      undo_stack_ (
        utils::make_qobject_unique<undo::UndoStack> (
          [this] (const auto &action, bool recalculate_graph) {
            project_->engine ()
              ->execute_function_with_paused_processing_synchronously (
                action, recalculate_graph);
          },
          this)),
      quantize_opts_editor_ (
        std::make_unique<old_dsp::QuantizeOptions> (
          dsp::notes::NoteLength::Note_1_8)),
      quantize_opts_timeline_ (
        std::make_unique<old_dsp::QuantizeOptions> (
          dsp::notes::NoteLength::Note_1_1)),
      arranger_object_creator_ (
        utils::make_qobject_unique<actions::ArrangerObjectCreator> (
          *undo_stack_,
          *project_->arrangerObjectFactory (),
          *ui_state_->snapGridTimeline (),
          *ui_state_->snapGridEditor (),
          this)),
      track_creator_ (
        utils::make_qobject_unique<actions::TrackCreator> (
          *undo_stack_,
          *project_->track_factory_,
          *project_->tracklist ()->collection (),
          *project_->tracklist ()->trackRouting (),
          *project_->tracklist ()->singletonTracks (),
          this)),
      plugin_importer_ (
        utils::make_qobject_unique<actions::PluginImporter> (
          *undo_stack_,
          *project_->plugin_factory_,
          *track_creator_,
          [] (plugins::PluginUuidReference plugin_ref) {
            z_debug ("Plugin instantiation completed");
            plugin_ref.get ()->setUiVisible (true);
          },
          this)),
      plugin_operator_ (
        utils::make_qobject_unique<
          actions::PluginOperator> (*undo_stack_, project_->get_registry (), this)),
      file_importer_ (
        utils::make_qobject_unique<actions::FileImporter> (
          *undo_stack_,
          *arranger_object_creator_,
          *track_creator_,
          this)),
      transport_controller_ (
        utils::make_qobject_unique<controllers::TransportController> (
          *project_->transport_,
          *ui_state_->snapGridTimeline (),
          this))
{
  project_->setParent (this);

  project_->set_audio_input_selection_provider (
    [this] (const structure::tracks::Track::Uuid &uuid)
      -> dsp::AudioInputSelection * {
      return ui_state_->find_audio_input_selection (uuid);
    });

  QObject::connect (
    ui_state_.get (),
    &structure::project::ProjectUiState::audioInputSelectionChanged, this,
    [this] { project_->engine ()->graph_dispatcher ().recalc_graph (false); });

  recording_coordinator_ =
    utils::make_qobject_unique<controllers::RecordingCoordinator> (this);

  // The recording callback captures a raw pointer to the coordinator.
  // This is safe because ~ProjectSession() deactivates the engine (stopping
  // all audio callbacks) before member destruction begins, so the coordinator
  // outlives all callback invocations.
  auto * coordinator = recording_coordinator_.get ();
  project_->install_recording_callback (
    [coordinator] (
      const structure::tracks::Track::Uuid &track_id,
      units::sample_t timeline_position, const dsp::ITransport &transport,
      const dsp::MidiEventVector * midi_events,
      std::optional<structure::tracks::TrackProcessor::ConstStereoPortPair>
        stereo_ports) {
      auto * session = coordinator->session_for_track (track_id);
      if (session == nullptr)
        return;

      if (stereo_ports.has_value ())
        {
          session->write_samples (
            timeline_position, transport.recording_enabled (),
            stereo_ports->first, stereo_ports->second);
        }
    });

  auto * collection = project_->tracklist ()->collection ();
  QObject::connect (
    collection, &structure::tracks::TrackCollection::trackRecordingArmedChanged,
    this,
    [this,
     coordinator_ptr = QPointer<controllers::RecordingCoordinator> (
       coordinator)] (structure::tracks::Track * track, bool armed) {
      if (coordinator_ptr.isNull ())
        return;
      if (armed)
        {
          static constexpr auto kDefaultMaxBlockLength = units::samples (8192u);
          auto block_length = project_->engine ()->block_length ();
          coordinator_ptr->arm_track (
            track->get_uuid (),
            block_length > units::samples (0u)
              ? block_length
              : kDefaultMaxBlockLength);
        }
      else
        {
          coordinator_ptr->disarm_track (track->get_uuid ());
        }
    });

  auto * transport = project_->getTransport ();
  QObject::connect (
    transport, &dsp::Transport::playStateChanged, this,
    [coordinator_ptr = QPointer<controllers::RecordingCoordinator> (
       coordinator)] (dsp::ITransport::PlayState new_state) {
      if (coordinator_ptr.isNull ())
        return;
      if (new_state == dsp::ITransport::PlayState::Paused)
        {
          coordinator_ptr->finalizeAllSessions ();
        }
    });

  QObject::connect (
    project_->engine (), &dsp::AudioEngine::blockLengthChanged, coordinator,
    [coordinator] (int block_length) {
      coordinator->prepare_for_processing (units::samples (block_length));
    });

  recording_materializer_ = utils::make_qobject_unique<
    controllers::RecordingMaterializer> (
    *recording_coordinator_, *undo_stack_,
    [creator_ptr = QPointer<actions::ArrangerObjectCreator> (
       arranger_object_creator_.get ()),
     project_ptr = QPointer<structure::project::Project> (project_.get ())] (
      structure::tracks::TrackUuid track_id, units::sample_t start_position,
      const utils::audio::AudioBuffer &initial_frames, size_t lane_index)
      -> controllers::RecordingMaterializer::RegionCreationResult {
      if (creator_ptr.isNull () || project_ptr.isNull ())
        return std::nullopt;

      auto * track = project_ptr->tracklist ()->get_track (track_id);
      if (track == nullptr)
        return std::nullopt;

      assert (!track->lanes ()->lanes ().empty ());

      const auto actual_lane_idx =
        std::max (lane_index, track->lanes ()->lanes ().size () - 1);
      track->lanes ()->create_missing_lanes (actual_lane_idx);
      auto * lane = track->lanes ()->lanes ().at (actual_lane_idx).get ();

      const auto start_ticks = project_ptr->tempo_map ().samples_to_tick (
        units::precise_sample_t (start_position));

      const auto clip_name = utils::Utf8String::from_qstring (
        QObject::tr ("Recording %1").arg (track->name ()));

      auto region_ref = creator_ptr->add_audio_region_for_recording (
        *track, *lane, initial_frames, clip_name, start_ticks.in (units::ticks));

      return controllers::RecordingMaterializer::CreatedRegion{
        std::move (region_ref), actual_lane_idx
      };
    },
    [&settings = app_settings_] () {
      using controllers::recording::RecordingMode;
      return static_cast<RecordingMode> (std::clamp (
        settings.recordingMode (), 0,
        static_cast<int> (RecordingMode::TakesMuted)));
    },
    this);
}

ProjectSession::~ProjectSession ()
{
  project_->engine ()->deactivate ();
  recording_materializer_.reset ();
}

QString
ProjectSession::title () const
{
  return title_.to_qstring ();
}

void
ProjectSession::setTitle (const QString &title)
{
  const auto std_str = utils::Utf8String::from_qstring (title);
  if (title_ == std_str)
    return;

  title_ = std_str;
  Q_EMIT titleChanged (title);
}

QString
ProjectSession::projectDirectory () const
{
  return utils::Utf8String::from_path (project_directory_).to_qstring ();
}

void
ProjectSession::setProjectDirectory (const QString &directory)
{
  auto path = utils::Utf8String::from_qstring (directory).to_path ();
  if (project_directory_ == path)
    return;

  project_directory_ = path;
  Q_EMIT projectDirectoryChanged (directory);
}

structure::project::Project *
ProjectSession::project () const
{
  return project_.get ();
}

structure::project::ProjectUiState *
ProjectSession::uiState () const
{
  return ui_state_.get ();
}

undo::UndoStack *
ProjectSession::undoStack () const
{
  return undo_stack_.get ();
}

zrythm::actions::ArrangerObjectCreator *
ProjectSession::arrangerObjectCreator () const
{
  return arranger_object_creator_.get ();
}

zrythm::actions::TrackCreator *
ProjectSession::trackCreator () const
{
  return track_creator_.get ();
}

actions::PluginImporter *
ProjectSession::pluginImporter () const
{
  return plugin_importer_.get ();
}

actions::PluginOperator *
ProjectSession::pluginOperator () const
{
  return plugin_operator_.get ();
}

actions::FileImporter *
ProjectSession::fileImporter () const
{
  return file_importer_.get ();
}

controllers::TransportController *
ProjectSession::transportController () const
{
  return transport_controller_.get ();
}

controllers::RecordingCoordinator *
ProjectSession::recordingCoordinator () const
{
  return recording_coordinator_.get ();
}

actions::ArrangerObjectSelectionOperator *
ProjectSession::createArrangerObjectSelectionOperator (
  QItemSelectionModel * selectionModel) const
{
  auto * sel_operator = new actions::ArrangerObjectSelectionOperator (
    *undo_stack_, *selectionModel,
    [this] (structure::arrangement::ArrangerObjectPtrVariant obj_var) {
      return std::visit (
        [&] (const auto &obj)
          -> actions::ArrangerObjectSelectionOperator::ArrangerObjectOwnerPtrVariant {
          using ObjT = utils::base_type<decltype (obj)>;
          if constexpr (structure::arrangement::LaneOwnedObject<ObjT>)
            {
              return static_cast<structure::arrangement::ArrangerObjectOwner<
                ObjT> *> (project_->tracklist ()->getTrackLaneForObject (obj));
            }
          else if constexpr (structure::arrangement::TimelineObject<ObjT>)
            {
              return dynamic_cast<
                structure::arrangement::ArrangerObjectOwner<ObjT> *> (
                project_->tracklist ()->getTrackForTimelineObject (obj));
            }
          else
            {
              return dynamic_cast<structure::arrangement::ArrangerObjectOwner<
                ObjT> *> (obj->parentObject ());
            }
        },
        obj_var);
    },
    *project_->arrangerObjectFactory ());

  QQmlEngine::setObjectOwnership (sel_operator, QQmlEngine::JavaScriptOwnership);

  return sel_operator;
}

std::optional<std::filesystem::path>
ProjectSession::get_newer_backup ()
{
  // TODO
  return std::nullopt;
  const auto filepath =
    project_directory_
    / structure::project::ProjectPathProvider::get_path (
      structure::project::ProjectPathProvider::ProjectPath::ProjectFile);

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

  std::filesystem::path result;
  const auto            backups_dir =
    project_directory_
    / structure::project::ProjectPathProvider::get_path (
      structure::project::ProjectPathProvider::ProjectPath::BackupsDir);
  try
    {
      for (const auto &entry : std::filesystem::directory_iterator (backups_dir))
        {
          auto full_path =
            entry.path ()
            / structure::project::ProjectPathProvider::get_path (
              structure::project::ProjectPathProvider::ProjectPath::ProjectFile);
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
}

gui::qquick::QFutureQmlWrapper *
ProjectSession::save ()
{
  assert (!project_directory_.empty ());

  auto future = controllers::ProjectSaver::save (
    *project_, *ui_state_, *undo_stack_, zrythm::Zrythm::get_app_version (),
    project_directory_, false);

  auto * wrapper = new gui::qquick::QFutureQmlWrapperT<QString> (future);
  QQmlEngine::setObjectOwnership (wrapper, QQmlEngine::JavaScriptOwnership);

  return wrapper;
}

gui::qquick::QFutureQmlWrapper *
ProjectSession::saveAs (const QString &path)
{
  auto new_path = utils::Utf8String::from_qstring (path).to_path ();

  auto future = controllers::ProjectSaver::save (
    *project_, *ui_state_, *undo_stack_, zrythm::Zrythm::get_app_version (),
    new_path, false);

  auto * wrapper = new gui::qquick::QFutureQmlWrapperT<QString> (future);

  // Update project directory and title when save completes
  QObject::connect (
    wrapper, &gui::qquick::QFutureQmlWrapperT<QString>::finished, this,
    [this, new_path, future] () {
      if (future.resultCount () > 0)
        {
          auto saved_path = future.result ();
          if (!saved_path.isEmpty ())
            {
              setProjectDirectory (saved_path);
              setTitle (
                utils::Utf8String::from_path (
                  utils::io::path_get_basename (new_path))
                  .to_qstring ());
            }
        }
    });

  QQmlEngine::setObjectOwnership (wrapper, QQmlEngine::JavaScriptOwnership);

  return wrapper;
}

} // namespace zrythm::gui
