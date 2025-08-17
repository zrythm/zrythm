// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <filesystem>

#include "dsp/port_connections_manager.h"
#include "engine/device_io/engine.h"
#include "engine/session/graph_dispatcher.h"
#include "engine/session/transport.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/project_manager.h"
#include "gui/backend/ui.h"
#include "structure/arrangement/audio_region.h"
#include "structure/tracks/tracklist.h"
#include "utils/datetime.h"
#include "utils/exceptions.h"
#include "utils/gtest_wrapper.h"
#include "utils/io.h"
#include "utils/logger.h"
#include "utils/objects.h"
#include "utils/progress_info.h"
#include "utils/views.h"

#include "juce_wrapper.h"
#include <fmt/printf.h>
#include <zstd.h>

using namespace zrythm;

static auto
juce_plugin_toplevel_window_provider (juce::AudioProcessorEditor &editor)
{
  auto window = std::make_unique<juce::DocumentWindow> (
    editor.getAudioProcessor ()->getName (), juce::Colours::cadetblue,
    juce::DocumentWindow::minimiseButton | juce::DocumentWindow::closeButton);
  window->setAlwaysOnTop (true); // Optional: keep on top
  window->setUsingNativeTitleBar (true);
  window->setContentNonOwned (&editor, true);
  window->centreWithSize (
    editor.getWidth (), editor.getHeight ()); // Center on screen
  window->setResizable (true, true);
  window->addToDesktop ();
  window->setVisible (true);
  window->toFront (true);
  return window;
}

Project::Project (
  std::shared_ptr<juce::AudioDeviceManager> device_manager,
  QObject *                                 parent)
    : QObject (parent),
      tempo_map_ (
        device_manager->getCurrentAudioDevice ()->getCurrentSampleRate ()),
      tempo_map_wrapper_ (new dsp::TempoMapWrapper (tempo_map_, this)),
      file_audio_source_registry_ (new dsp::FileAudioSourceRegistry (this)),
      port_registry_ (new dsp::PortRegistry (this)),
      param_registry_ (
        new dsp::ProcessorParameterRegistry (*port_registry_, this)),
      plugin_registry_ (new PluginRegistry (this)),
      arranger_object_registry_ (
        new structure::arrangement::ArrangerObjectRegistry (this)),
      track_registry_ (new structure::tracks::TrackRegistry (this)),
      version_ (Zrythm::get_version (false)),
      tool_ (new gui::backend::Tool (this)),
      port_connections_manager_ (new dsp::PortConnectionsManager (this)),
      audio_engine_ (
        utils::make_qobject_unique<
          engine::device_io::AudioEngine> (this, device_manager)),
      transport_ (new engine::session::Transport (this)),
      pool_ (
        std::make_unique<dsp::AudioPool> (
          *file_audio_source_registry_,
          [this] (bool backup) {
            return get_path (ProjectPath::POOL, backup);
},
          [this] () { return audio_engine_->get_sample_rate (); })),
      quantize_opts_editor_ (
        std::make_unique<QuantizeOptions> (zrythm::utils::NoteLength::Note_1_8)),
      quantize_opts_timeline_ (
        std::make_unique<QuantizeOptions> (zrythm::utils::NoteLength::Note_1_1)),
      snap_grid_editor_ (
        std::make_unique<SnapGrid> (
          SnapGrid::Type::Editor,
          utils::NoteLength::Note_1_8,
          true,
          [&] { return audio_engine_->frames_per_tick_; },
          [&] { return transport_->ticks_per_bar_; },
          [&] { return transport_->ticks_per_beat_; })),
      snap_grid_timeline_ (
        std::make_unique<SnapGrid> (
          SnapGrid::Type::Timeline,
          utils::NoteLength::Bar,
          true,
          [&] { return audio_engine_->frames_per_tick_; },
          [&] { return transport_->ticks_per_bar_; },
          [&] { return transport_->ticks_per_beat_; })),
      timeline_ (new Timeline (this)),
      clip_editor_ (new ClipEditor (
        *arranger_object_registry_,
        [&] (const auto &id) {
          return get_track_registry ().find_by_id_or_throw (id);
        },
        this)),
      midi_mappings_ (
        std::make_unique<engine::session::MidiMappings> (*param_registry_)),
      tracklist_ (new structure::tracks::Tracklist (
        *this,
        *port_registry_,
        *param_registry_,
        *track_registry_,
        *port_connections_manager_,
        get_tempo_map ())),
      undo_manager_ (new gui::actions::UndoManager (this)),
      arranger_object_factory_ (new structure::arrangement::ArrangerObjectFactory (
        get_tempo_map (),
        *arranger_object_registry_,
        *file_audio_source_registry_,
        *gui::SettingsManager::get_instance (),
        *snap_grid_timeline_,
        *snap_grid_editor_,
        [&] () { return audio_engine_->get_sample_rate (); },
        [&] () { return get_tempo_map ().tempo_at_tick (0); },
        structure::arrangement::ArrangerObjectSelectionManager{
          clip_editor_->getAudioClipEditor ()->get_selected_object_ids (),
          *arranger_object_registry_ },
        structure::arrangement::ArrangerObjectSelectionManager{
          timeline_->get_selected_object_ids (), *arranger_object_registry_ },
        structure::arrangement::ArrangerObjectSelectionManager{
          clip_editor_->getPianoRoll ()->get_selected_object_ids (),
          *arranger_object_registry_ },
        structure::arrangement::ArrangerObjectSelectionManager{
          clip_editor_->getChordEditor ()->get_selected_object_ids (),
          *arranger_object_registry_ },
        structure::arrangement::ArrangerObjectSelectionManager{
          clip_editor_->getAutomationEditor ()->get_selected_object_ids (),
          *arranger_object_registry_ },
        *clip_editor_,
        this)),
      plugin_factory_ (new PluginFactory (
        PluginFactory::CommonFactoryDependencies{
          .plugin_registry_ = *plugin_registry_,
          .processor_base_dependencies_ =
            dsp::ProcessorBase::ProcessorBaseDependencies{
              .port_registry_ = *port_registry_,
              .param_registry_ = *param_registry_ },
          .state_dir_path_provider_ =
            [this] () { return get_path (ProjectPath::PluginStates, false); },
          .create_plugin_instance_async_func_ =
            [] (
              const juce::PluginDescription                  &description,
              double                                          initialSampleRate,
              int                                             initialBufferSize,
              juce::AudioPluginFormat::PluginCreationCallback callback) {
              Zrythm::getInstance ()
                ->getPluginManager ()
                ->get_format_manager ()
                ->createPluginInstanceAsync (
                  description, initialSampleRate, initialBufferSize, callback);
            },
          .sample_rate_provider_ =
            [this] () { return audio_engine_->get_sample_rate (); },
          .buffer_size_provider_ =
            [this] () { return audio_engine_->get_block_length (); },
          .top_level_window_provider_ = juce_plugin_toplevel_window_provider },
        *gui::SettingsManager::get_instance (),
        this)),
      track_factory_ (new structure::tracks::TrackFactory (
        get_final_track_dependencies (),
        *gui::SettingsManager::get_instance (),
        this)),
      device_manager_ (device_manager)
{
  // audio_engine_ = std::make_unique<AudioEngine> (this);
}

Project::~Project ()
{
  loaded_ = false;
}

structure::tracks::FinalTrackDependencies
Project::get_final_track_dependencies () const
{
  return structure::tracks::FinalTrackDependencies{
    tempo_map_,
    *file_audio_source_registry_,
    *plugin_registry_,
    *port_registry_,
    *param_registry_,
    *arranger_object_registry_,
    *track_registry_,
    *transport_,
    [this] () {
      return tracklist_->get_track_span ().get_num_soloed_tracks () > 0;
    }
  };
}

std::optional<fs::path>
Project::get_newer_backup ()
{
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
}

void
Project::make_project_dirs (bool is_backup)
{
  for (
    auto type :
    { ProjectPath::BACKUPS, ProjectPath::EXPORTS, ProjectPath::EXPORTS_STEMS,
      ProjectPath::POOL, ProjectPath::PluginStates,
      ProjectPath::PLUGIN_EXT_COPIES, ProjectPath::PLUGIN_EXT_LINKS })
    {
      const auto dir = get_path (type, is_backup);
      assert (!dir.empty ());
      try
        {
          utils::io::mkdir (dir);
        }
      catch (ZrythmException &e)
        {
          throw ZrythmException (
            fmt::format ("Failed to create directory {}", dir));
        }
    }
}

void
Project::compress_or_decompress (
  bool              compress,
  char **           _dest,
  size_t *          _dest_size,
  CompressionFlag   dest_type,
  const QByteArray &src)
{
  z_info (
    "using zstd v{}.{}.{}", ZSTD_VERSION_MAJOR, ZSTD_VERSION_MINOR,
    ZSTD_VERSION_RELEASE);

  char * dest = nullptr;
  size_t dest_size = 0;
  if (compress)
    {
      z_info ("compressing project...");
      size_t compress_bound = ZSTD_compressBound (src.size ());
      dest = (char *) malloc (compress_bound);
      dest_size =
        ZSTD_compress (dest, compress_bound, src.constData (), src.size (), 1);
      if (ZSTD_isError (dest_size))
        {
          free (dest);
          throw ZrythmException (format_str (
            "Failed to compress project file: {}",
            ZSTD_getErrorName (dest_size)));
        }
    }
  else /* decompress */
    {
#if (ZSTD_VERSION_MAJOR == 1 && ZSTD_VERSION_MINOR < 3)
      unsigned long long const frame_content_size =
        ZSTD_getDecompressedSize (src.constData (), src.size ());
      if (frame_content_size == 0)
#else
      unsigned long long const frame_content_size =
        ZSTD_getFrameContentSize (src.constData (), src.size ());
      if (frame_content_size == ZSTD_CONTENTSIZE_ERROR)
#endif
        {
          throw ZrythmException ("Project not compressed by zstd");
        }
      dest = (char *) malloc ((size_t) frame_content_size);
      dest_size = ZSTD_decompress (
        dest, frame_content_size, src.constData (), src.size ());
      if (ZSTD_isError (dest_size))
        {
          free (dest);
          throw ZrythmException (format_str (
            "Failed to decompress project file: {}",
            ZSTD_getErrorName (dest_size)));
        }
      if (dest_size != frame_content_size)
        {
          free (dest);
          /* impossible because zstd will check this condition */
          throw ZrythmException ("uncompressed_size != frame_content_size");
        }
    }

  z_debug (
    "{} : {} bytes -> {} bytes", compress ? "Compression" : "Decompression",
    src.size (), dest_size);

  switch (dest_type)
    {
    case CompressionFlag::PROJECT_COMPRESS_DATA:
      *_dest = dest;
      *_dest_size = dest_size;
      break;
    case CompressionFlag::PROJECT_COMPRESS_FILE:
      {
        // setting the resulting data to the file at path `_dest`
        try
          {
            utils::io::set_file_contents (fs::path (*_dest), dest, dest_size);
          }
        catch (const ZrythmException &e)
          {
            throw ZrythmException (
              fmt::format ("Failed to write project file: {}", e.what ()));
          }
      }
      break;
    }
}

void
Project::set_and_create_next_available_backup_dir ()
{
  auto backups_dir = get_path (ProjectPath::BACKUPS, false);

  int i = 0;
  do
    {
      if (i > 0)
        {
          std::string bak_title = fmt::format ("{}.bak{}", title_, i);
          backup_dir_ = backups_dir / bak_title;
        }
      else
        {
          std::string bak_title = fmt::format ("{}.bak", title_);
          backup_dir_ = backups_dir / bak_title;
        }
      i++;
    }
  while (
    backup_dir_.has_value () && utils::io::path_exists (backup_dir_.value ()));

  try
    {
      utils::io::mkdir (*backup_dir_);
    }
  catch (ZrythmException &e)
    {
      throw ZrythmException (format_qstr (
        QObject::tr ("Failed to create backup directory {}"), backup_dir_));
    }
}

void
Project::activate ()
{
  z_debug ("Activating project {} ({:p})...", title_, fmt::ptr (this));

  last_saved_action_ = undo_manager_->get_last_action ();

  audio_engine_->activate (true);

  /* pause engine */
  engine::device_io::AudioEngine::State state{};
  audio_engine_->wait_for_pause (state, true, false);

  /* connect channel inputs to hardware and re-expose ports to
   * backend. has to be done after engine activation */
  // auto track_span = tracklist_->get_track_span ();
  // track_span.reconnect_ext_input_ports (*audio_engine_);

  /* reconnect graph */
  audio_engine_->router_->recalc_graph (false);

  /* fix audio regions in case running under a new sample rate */
  // fix_audio_regions ();

  /* resume engine */
  audio_engine_->resume (state);

  z_debug ("Project {} ({:p}) activated", title_, fmt::ptr (this));
}

void
Project::add_default_tracks ()
{
  using namespace zrythm::structure::tracks;
  z_return_if_fail (tracklist_);

  /* init pinned tracks */

  auto add_track = [&]<typename TrackT> (const auto &name) {
    static_assert (
      std::derived_from<TrackT, structure::tracks::Track>,
      "T must be derived from Track");

    z_debug ("adding {} track...", typeid (TrackT).name ());
    auto * track = new TrackT (get_final_track_dependencies ());
    get_track_registry ().register_object (track);
    track->setName (name);
    tracklist_->append_track (
      structure::tracks::TrackUuidReference{
        track->get_uuid (), get_track_registry () },
      *audio_engine_, false, false);

    return track;
  };

  /* chord */
  add_track.operator()<ChordTrack> (QObject::tr ("Chords"));
  tracklist_->get_chord_track ()->set_note_pitch_to_descriptor_func (
    [this] (midi_byte_t note_pitch) {
      return getClipEditor ()->getChordEditor ()->get_chord_from_note_number (
        note_pitch);
    });

  /* tempo */
  transport_->update_caches (
    get_tempo_map ().time_signature_at_tick (0).numerator,
    get_tempo_map ().time_signature_at_tick (0).denominator);
  audio_engine_->update_frames_per_tick (
    get_tempo_map ().time_signature_at_tick (0).numerator,
    static_cast<bpm_t> (get_tempo_map ().tempo_at_tick (0)),
    audio_engine_->get_sample_rate (), true, true, false);

  /* add a scale */
  arranger_object_factory_->add_scale_object (
    *tracklist_->chord_track_,
    utils::make_qobject_unique<dsp::MusicalScale> (
      dsp::MusicalScale::ScaleType::Aeolian, dsp::MusicalNote::A),
    0);

  /* modulator */
  add_track.operator()<ModulatorTrack> (QObject::tr ("Modulators"));

  /* add marker track and default markers */
  auto * marker_track =
    add_track.operator()<MarkerTrack> (QObject::tr ("Markers"));
  const auto add_default_markers =
    [] (
      auto &marker_track_inner, const auto &factory, const int ticks_per_bar,
      const auto frames_per_tick) {
      {
        auto          marker_name = fmt::format ("[{}]", QObject::tr ("start"));
        dsp::Position pos;
        pos.set_to_bar (1, ticks_per_bar, frames_per_tick);
        factory->addMarker (
          structure::arrangement::Marker::MarkerType::Start, marker_track_inner,
          utils::Utf8String::from_utf8_encoded_string (marker_name).to_qstring (),
          pos.ticks_);
      }

      {
        auto          marker_name = fmt::format ("[{}]", QObject::tr ("end"));
        dsp::Position pos;
        pos.set_to_bar (129, ticks_per_bar, frames_per_tick);
        factory->addMarker (
          structure::arrangement::Marker::MarkerType::End, marker_track_inner,
          utils::Utf8String::from_utf8_encoded_string (marker_name).to_qstring (),
          pos.ticks_);
      }
    };
  add_default_markers (
    marker_track, arranger_object_factory_, transport_->ticks_per_bar_,
    audio_engine_->frames_per_tick_);

  tracklist_->set_pinned_tracks_cutoff_index (tracklist_->track_count ());

  /* add master track */
  auto * master_track =
    add_track.operator()<MasterTrack> (QObject::tr ("Master"));
  tracklist_->get_selection_manager ().select_unique (master_track->get_uuid ());

  last_selection_ = SelectionType::Tracklist;
}

#if 0
ArrangerWidget *
project_get_arranger_for_last_selection (
  Project * self)
{
  Region * r =
    CLIP_EDITOR->get_region ();
  switch (self->last_selection)
    {
    case Project::SelectionType::Timeline:
      return TL_SELECTIONS;
      break;
    case Project::SelectionType::Editor:
      if (r)
        {
          switch (r->id.type)
            {
            case RegionType::REGION_TYPE_AUDIO:
              return AUDIO_SELECTIONS;
            case RegionType::REGION_TYPE_AUTOMATION:
              return AUTOMATION_SELECTIONS;
            case RegionType::REGION_TYPE_MIDI:
              return MIDI_SELECTIONS;
            case RegionType::REGION_TYPE_CHORD:
              return CHORD_SELECTIONS;
            }
        }
      break;
    default:
      return NULL;
    }

  return NULL;
}
#endif

#if 0
std::optional<ArrangerSelectionsPtrVariant>
Project::get_arranger_selections_for_last_selection ()
{
  auto r = CLIP_EDITOR->get_region ();
  switch (last_selection_)
    {
    case Project::SelectionType::Timeline:
      return timeline_selections_;
      break;
    case Project::SelectionType::Editor:
      if (r)
        {
          return std::visit (
            [&] (auto &&region) -> std::optional<ArrangerSelectionsPtrVariant> {
              if (region->get_arranger_selections ().has_value ())
                {
                  return std::visit (
                    [&] (auto &&sel)
                      -> std::optional<ArrangerSelectionsPtrVariant> {
                      return sel;
                    },
                    region->get_arranger_selections ().value ());
                }

              return std::nullopt;
            },
            *r);
        }
      break;
    default:
      return std::nullopt;
    }

  return std::nullopt;
}
#endif

std::string
Project::get_existing_uncompressed_text (bool backup)
{
  /* get file contents */
  const auto project_file_path = get_path (ProjectPath::ProjectFile, backup);
  z_debug ("getting text for project file {}", project_file_path);

  QByteArray compressed_pj{};
  try
    {
      compressed_pj = utils::io::read_file_contents (project_file_path);
    }
  catch (const ZrythmException &e)
    {
      throw ZrythmException (format_qstr (
        QObject::tr ("Unable to read file at {}: {}"), project_file_path,
        e.what ()));
    }

  /* decompress */
  z_info ("decompressing project...");
  char * text = nullptr;
  size_t text_size = 0;
  try
    {
      decompress (
        &text, &text_size, CompressionFlag::PROJECT_DECOMPRESS_DATA,
        compressed_pj);
    }
  catch (ZrythmException &e)
    {
      throw ZrythmException (format_qstr (
        QObject::tr ("Unable to decompress project file at {}"),
        project_file_path));
    }

  /* make string null-terminated */
  text = (char *) realloc (text, text_size + sizeof (char));
  text[text_size] = '\0';
  std::string ret (text);
  free (text);
  return ret;
}

int
Project::autosave_cb (void * data)
{
#if 0
  if (
    !PROJECT || !PROJECT->loaded_ || PROJECT->dir_.empty ()
    || PROJECT->datetime_str_.empty () || !MAIN_WINDOW || !MAIN_WINDOW->setup)
    return G_SOURCE_CONTINUE;

  unsigned int autosave_interval_mins =
    g_settings_get_uint (S_P_PROJECTS_GENERAL, "autosave-interval");

  /* return if autosave disabled */
  if (autosave_interval_mins <= 0)
    return G_SOURCE_CONTINUE;

  auto &out_ports =
    PROJECT->tracklist_->master_track_->channel ()->stereo_out_;
  auto cur_time = SteadyClock::now ();
  /* subtract 4 seconds because the time this gets called is not exact (this is
   * an old comment and I don't remember why this is done) */
  auto autosave_interval =
    std::chrono::minutes (autosave_interval_mins) - std::chrono::seconds (4);

  /* skip if semaphore busy */
  SemaphoreRAII sem (PROJECT->save_sem_);
  if (!sem.is_acquired ())
    {
      z_debug ("can't acquire project lock - skipping autosave");
      return G_SOURCE_CONTINUE;
    }

  auto * last_action = PROJECT->undo_manager_->get_last_action ();

  /* skip if bad time to save or rolling */
  if (cur_time - PROJECT->last_successful_autosave_time_ < autosave_interval || TRANSPORT->is_rolling() || (TRANSPORT->play_state_ == Transport::PlayState::RollRequested && (TRANSPORT->preroll_frames_remaining_ > 0 || TRANSPORT->countin_frames_remaining_ > 0)))
    {
      return G_SOURCE_CONTINUE;
    }

  /* skip if sound is playing */
  if (
    out_ports->get_l ().get_peak () >= 0.0001f
    || out_ports->get_r ().get_peak () >= 0.0001f)
    {
      z_debug ("sound is playing, skipping autosave");
      return G_SOURCE_CONTINUE;
    }

  /* skip if currently performing action */
  if (arranger_widget_any_doing_action ())
    {
      z_debug ("in the middle of an action, skipping autosave");
      return G_SOURCE_CONTINUE;
    }

  if (PROJECT->last_action_in_last_successful_autosave_ == last_action)
    {
      z_debug ("last action is same as previous backup - skipping autosave");
      return G_SOURCE_CONTINUE;
    }

  /* skip if any modal window is open */
  if (ZRYTHM_HAVE_UI)
    {
      auto toplevels = gtk_window_get_toplevels ();
      auto num_toplevels = g_list_model_get_n_items (toplevels);
      for (guint i = 0; i < num_toplevels; i++)
        {
          auto window = GTK_WINDOW (g_list_model_get_item (toplevels, i));
          if (
            gtk_widget_get_visible (GTK_WIDGET (window))
            && (gtk_window_get_modal (window) || gtk_window_get_transient_for (window) == GTK_WINDOW (MAIN_WINDOW)))
            {
              z_debug ("modal/transient windows exist - skipping autosave");
              z_gtk_widget_print_hierarchy (GTK_WIDGET (window));
              return G_SOURCE_CONTINUE;
            }
        }
    }

  /* ok to save */
  try
    {
      PROJECT->save (PROJECT->dir_, true, true, true);
      PROJECT->last_successful_autosave_time_ = cur_time;
    }
  catch (const ZrythmException &e)
    {
      if (ZRYTHM_HAVE_UI)
        {
          e.handle (QObject::tr ("Failed to save the project"));
        }
      else
        {
          z_warning ("{}", e.what ());
        }
    }

  return G_SOURCE_CONTINUE;
#endif
  return 0;
}

fs::path
Project::get_path (ProjectPath path, bool backup) const
{
  const auto dir = backup ? *backup_dir_ : dir_;
  switch (path)
    {
    case ProjectPath::BACKUPS:
      return dir / PROJECT_BACKUPS_DIR;
    case ProjectPath::EXPORTS:
      return dir / PROJECT_EXPORTS_DIR;
    case ProjectPath::EXPORTS_STEMS:
      return dir / PROJECT_EXPORTS_DIR / PROJECT_STEMS_DIR;
    case ProjectPath::PLUGINS:
      return dir / PROJECT_PLUGINS_DIR;
    case ProjectPath::PluginStates:
      {
        auto plugins_dir = get_path (ProjectPath::PLUGINS, backup);
        return plugins_dir / PROJECT_PLUGIN_STATES_DIR;
      }
      break;
    case ProjectPath::PLUGIN_EXT_COPIES:
      {
        auto plugins_dir = get_path (ProjectPath::PLUGINS, backup);
        return plugins_dir / PROJECT_PLUGIN_EXT_COPIES_DIR;
      }
      break;
    case ProjectPath::PLUGIN_EXT_LINKS:
      {
        auto plugins_dir = get_path (ProjectPath::PLUGINS, backup);
        return plugins_dir / PROJECT_PLUGIN_EXT_LINKS_DIR;
      }
      break;
    case ProjectPath::POOL:
      return dir / PROJECT_POOL_DIR;
    case ProjectPath::ProjectFile:
      return dir / PROJECT_FILE;
    case ProjectPath::FINISHED_FILE:
      return dir / PROJECT_FINISHED_FILE;
    default:
      z_return_val_if_reached ({});
    }
  z_return_val_if_reached ({});
}

Project::SerializeProjectThread::SerializeProjectThread (SaveContext &ctx)
    : juce::Thread ("SerializeProject"), ctx_ (ctx)
{
  startThread ();
}

Project::SerializeProjectThread::~SerializeProjectThread ()
{
  stopThread (-1);
}

void
Project::SerializeProjectThread::run ()
{
  char * compressed_json{};
  size_t compressed_size{};

  /* generate json */
  z_debug ("serializing project to json...");
  auto   time_before = Zrythm::getInstance ()->get_monotonic_time_usecs ();
  qint64 time_after{};
  std::optional<nlohmann::json> json;
  try
    {
      json = *ctx_.main_project_;
    }
  catch (const ZrythmException &e)
    {
      e.handle ("Failed to serialize project");
      ctx_.has_error_ = true;
      goto serialize_end;
    }
  time_after = Zrythm::getInstance ()->get_monotonic_time_usecs ();
  z_debug ("time to serialize: {}ms", (time_after - time_before) / 1000);

  /* compress */
  try
    {
      const auto json_str = json->dump ();
      // warning: this byte array depends on json_str being alive while it's used
      QByteArray src_data =
        QByteArray::fromRawData (json_str.c_str (), json_str.length ());
      compress (
        &compressed_json, &compressed_size,
        CompressionFlag::PROJECT_COMPRESS_DATA, src_data);
    }
  catch (const ZrythmException &ex)
    {
      ex.handle (QObject::tr ("Failed to compress project file"));
      ctx_.has_error_ = true;
      goto serialize_end;
    }

  /* set file contents */
  z_debug ("saving project file at {}...", ctx_.project_file_path_);
  try
    {
      utils::io::set_file_contents (
        ctx_.project_file_path_, compressed_json, compressed_size);
    }
  catch (const ZrythmException &e)
    {
      ctx_.has_error_ = true;
      z_error ("Unable to write project file: {}", e.what ());
    }
  free (compressed_json);

  z_debug ("successfully saved project");

serialize_end:
  ctx_.main_project_->undo_manager_->action_sem_.release ();
  ctx_.finished_.store (true);
}

bool
Project::idle_saved_callback (SaveContext * ctx)
{
  if (!ctx->finished_)
    {
      return true;
    }

  if (ctx->is_backup_)
    {

      z_debug ("Backup saved.");
      if (ZRYTHM_HAVE_UI)
        {

          // ui_show_notification (QObject::tr ("Backup saved."));
        }
    }
  else
    {
      if (ZRYTHM_HAVE_UI && !ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
        {
          zrythm::gui::ProjectManager::get_instance ()->add_to_recent_projects (
            utils::Utf8String::from_path (ctx->project_file_path_).to_qstring ());
        }
      if (ctx->show_notification_)
        {
          // ui_show_notification (QObject::tr ("Project saved."));
        }
    }

#if 0
  if (ZRYTHM_HAVE_UI && PROJECT->loaded_ && MAIN_WINDOW)
    {
      /* EVENTS_PUSH (EventType::ET_PROJECT_SAVED, PROJECT.get ()); */
    }
#endif

  ctx->progress_info_.mark_completed (ProgressInfo::CompletionType::SUCCESS, {});

  return false;
}

void
Project::cleanup_plugin_state_dirs (Project &main_project, bool is_backup)
{
  z_debug ("cleaning plugin state dirs{}...", is_backup ? " for backup" : "");

  std::vector<PluginPtrVariant> plugins;
  for (
    const auto &pl_var :
    main_project.get_plugin_registry ().get_hash_map () | std::views::values)
    {
      plugins.push_back (pl_var);
    }
  for (
    const auto &pl_var :
    get_plugin_registry ().get_hash_map () | std::views::values)
    {
      plugins.push_back (pl_var);
    }

  for (const auto &[i, pl_var] : utils::views::enumerate (plugins))
    {
      z_debug ("plugin {}: {}", i, PluginSpan::state_dir_projection (pl_var));
    }

  auto plugin_states_path =
    main_project.get_path (ProjectPath::PluginStates, false);

  try
    {
      QDir srcdir (
        utils::Utf8String::from_path (plugin_states_path).to_qstring ());
      const auto entries =
        srcdir.entryInfoList (QDir::Files | QDir::NoDotAndDotDot);

      for (const auto &filename : entries)
        {
          const auto filename_str =
            utils::Utf8String::from_qstring (filename.fileName ()).to_path ();
          const auto full_path = plugin_states_path / filename_str;

          bool found = std::ranges::contains (
            plugins, filename_str, PluginSpan::state_dir_projection);
          if (!found)
            {
              z_debug ("removing unused plugin state in {}", full_path);
              utils::io::rmdir (full_path, true);
            }
        }
    }
  catch (const ZrythmException &e)
    {
      z_critical ("Failed to open directory: {}", e.what ());
      return;
    }

  z_debug ("cleaned plugin state directories");
}

void
Project::save (
  const fs::path &_dir,
  const bool      is_backup,
  const bool      show_notification,
  const bool      async)
{
  z_info (
    "Saving project at {}, is backup: {}, show notification: {}, async: {}",
    _dir, is_backup, show_notification, async);

  /* pause engine */
  engine::device_io::AudioEngine::State state{};
  bool                                  engine_paused = false;
  z_return_if_fail (audio_engine_);
  if (audio_engine_->activated_)
    {
      audio_engine_->wait_for_pause (state, false, true);
      engine_paused = true;
    }

  /* if async, lock the undo manager */
  if (async)
    {
      undo_manager_->action_sem_.acquire ();
    }

  /* set the dir and create it if it doesn't exist */
  dir_ = _dir;
  try
    {
      utils::io::mkdir (dir_);
    }

  catch (const ZrythmException &e)
    {
      throw ZrythmException (

        fmt::format ("Failed to create project directory {}", dir_));
    }

  /* set the title */
  title_ = utils::Utf8String::from_path (dir_.filename ());

  /* save current datetime */
  datetime_str_ = utils::datetime::get_current_as_string ();

  /* set the project version */
  version_ = Zrythm::get_version (false);

  /* if backup, get next available backup dir */
  if (is_backup)
    {
      try
        {
          set_and_create_next_available_backup_dir ();
        }

      catch (const ZrythmException &e)
        {
          throw ZrythmException (
            QObject::tr ("Failed to create backup directory"));
        }
    }

  try
    {
      make_project_dirs (is_backup);
    }

  catch (const ZrythmException &e)
    {
      throw ZrythmException ("Failed to create project directories");
    }

  if (this == get_active_instance ())
    {
      /* write the pool */
      pool_->remove_unused (is_backup);
    }

  try
    {
      pool_->write_to_disk (is_backup);
    }
  catch (const ZrythmException &e)
    {
      throw ZrythmException ("Failed to write audio pool to disk");
    }

  auto ctx = std::make_unique<SaveContext> ();
  ctx->main_project_ = this;
  ctx->project_file_path_ = get_path (ProjectPath::ProjectFile, is_backup);
  ctx->show_notification_ = show_notification;
  ctx->is_backup_ = is_backup;
#if 0
  ctx->project_ = [&] () {
    if (ZRYTHM_IS_QT_THREAD)
      {
        return std::unique_ptr<Project> (clone (is_backup));
      }
    else
      {
        Project *  cloned_prj = nullptr;
        QThread *  currentThread = QThread::currentThread ();
        QEventLoop loop;
        QMetaObject::invokeMethod (
          QCoreApplication::instance (),
          [this, currentThread, is_backup, &cloned_prj, &loop] () {
            cloned_prj = clone (is_backup);

            // need to move the temporary cloned project to the outer scope's
            // thread, because it will be free'd on that thread too
            cloned_prj->moveToThread (currentThread);
            loop.quit ();
          },
          Qt::QueuedConnection);
        loop.exec ();
        return std::unique_ptr<Project> (cloned_prj);
      }
  }();
#endif

  if (is_backup)
    {
      /* copy plugin states */
      auto prj_pl_states_dir = get_path (ProjectPath::PLUGINS, false);
      auto prj_backup_pl_states_dir = get_path (ProjectPath::PLUGINS, true);
      try
        {
          utils::io::copy_dir (
            prj_backup_pl_states_dir, prj_pl_states_dir, false, true);
        }
      catch (const ZrythmException &e)
        {
          throw ZrythmException (QObject::tr ("Failed to copy plugin states"));
        }
    }

  if (!is_backup)
    {
      /* cleanup unused plugin states */
      cleanup_plugin_state_dirs (*this, is_backup);
    }

  /* TODO verify all plugin states exist */

  if (async)
    {
      SerializeProjectThread save_thread (*ctx);

      /* TODO: show progress dialog */
      if (ZRYTHM_HAVE_UI && false)
        {
          auto timer = new QTimer (this);
          QObject::connect (timer, &QTimer::timeout, this, [timer, &ctx] () {
            bool keep_calling = idle_saved_callback (ctx.get ());
            if (!keep_calling)
              {
                timer->stop ();
                timer->deleteLater ();
              }
          });
          timer->start (100);

          /* show progress while saving (TODO) */
        }
      else
        {
          while (!ctx->finished_.load ())
            {
              std::this_thread::sleep_for (std::chrono::milliseconds (1));
            }
          idle_saved_callback (ctx.get ());
        }
    }
  else /* else if no async */
    {
      /* call synchronously */
      SerializeProjectThread save_thread (*ctx);
      while (save_thread.isThreadRunning ())
        {
          std::this_thread::sleep_for (std::chrono::milliseconds (1));
        }
      idle_saved_callback (ctx.get ());
    }

  /* write FINISHED file */
  {
    const auto finished_file_path =
      get_path (ProjectPath::FINISHED_FILE, is_backup);
    utils::io::touch_file (finished_file_path);
  }

  auto last_action = undo_manager_->get_last_action ();
  if (is_backup)
    {
      last_action_in_last_successful_autosave_ = last_action;
    }
  else
    {
      last_saved_action_ = last_action;
    }

  if (engine_paused)
    {
      audio_engine_->resume (state);
    }

  z_info (
    "Saved project at {}, is backup: {}, show notification: {}, async: {}",
    _dir, is_backup, show_notification, async);
}

bool
Project::has_unsaved_changes () const
{
  /* simply check if the last performed action matches the last action when the
   * project was last saved/loaded */
  auto last_performed_action = undo_manager_->get_last_action ();
  return last_performed_action != last_saved_action_;
}

void
init_from (Project &obj, const Project &other, utils::ObjectCloneType clone_type)
{
  z_return_if_fail (ZRYTHM_IS_QT_THREAD);
  z_debug ("cloning project...");

  obj.title_ = other.title_;
  obj.datetime_str_ = other.datetime_str_;
  obj.version_ = other.version_;
  obj.transport_ = utils::clone_qobject (*other.transport_, &obj);
  obj.audio_engine_ = utils::clone_qobject (
    *other.audio_engine_, &obj, clone_type, &obj, obj.device_manager_);
  obj.pool_ = utils::clone_unique (
    *other.pool_, clone_type, *obj.file_audio_source_registry_,
    [&obj] (bool backup) { return obj.get_path (ProjectPath::POOL, backup); },
    [&obj] () { return obj.audio_engine_->get_sample_rate (); });
  obj.tracklist_ = utils::clone_qobject (
    *other.tracklist_, &obj, clone_type, obj, *obj.port_registry_,
    *obj.param_registry_, *obj.track_registry_, *obj.port_connections_manager_,
    obj.get_tempo_map ());
  obj.clip_editor_ = utils::clone_qobject (
    *other.clip_editor_, &obj, clone_type, *obj.arranger_object_registry_,
    [&] (const Project::TrackUuid &id) {
      return obj.get_track_registry ().find_by_id_or_throw (id);
    });
  obj.timeline_ = utils::clone_qobject (*other.timeline_, &obj);
  obj.snap_grid_timeline_ =
    std::make_unique<Project::SnapGrid> (*other.snap_grid_timeline_);
  obj.snap_grid_editor_ =
    std::make_unique<zrythm::gui::SnapGrid> (*other.snap_grid_editor_);
  obj.quantize_opts_timeline_ =
    std::make_unique<Project::QuantizeOptions> (*other.quantize_opts_timeline_);
  obj.quantize_opts_editor_ =
    std::make_unique<Project::QuantizeOptions> (*other.quantize_opts_editor_);
  // obj.region_link_group_manager_ = other.region_link_group_manager_;
  obj.port_connections_manager_ =
    utils::clone_qobject (*other.port_connections_manager_, &obj);
  obj.midi_mappings_ = utils::clone_unique (
    *other.midi_mappings_, clone_type, *obj.param_registry_);
  obj.undo_manager_ = utils::clone_qobject (*other.undo_manager_, &obj);
  obj.tool_ = utils::clone_qobject (*other.tool_, &obj);

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
Project::getDirectory () const
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
Project::getTracklist () const
{
  return tracklist_;
}

Timeline *
Project::getTimeline () const
{
  return timeline_;
}

engine::session::Transport *
Project::getTransport () const
{
  return transport_;
}

engine::device_io::AudioEngine *
Project::engine () const
{
  return audio_engine_.get ();
}

gui::backend::Tool *
Project::getTool () const
{
  return tool_;
}

ClipEditor *
Project::getClipEditor () const
{
  return clip_editor_;
}

gui::actions::UndoManager *
Project::getUndoManager () const
{
  return undo_manager_;
}

structure::arrangement::ArrangerObjectFactory *
Project::getArrangerObjectFactory () const
{
  return arranger_object_factory_;
}

PluginFactory *
Project::getPluginFactory () const
{
  return plugin_factory_;
}

structure::tracks::TrackFactory *
Project::getTrackFactory () const
{
  return track_factory_;
}

dsp::TempoMapWrapper *
Project::getTempoMap () const
{
  return tempo_map_wrapper_.get ();
}

Project *
Project::get_active_instance ()
{
  return zrythm::gui::ProjectManager::get_instance ()->getActiveProject ();
}

Project *
Project::clone (bool for_backup) const
{
  auto ret = utils::clone_raw_ptr (
    *this, utils::ObjectCloneType::Snapshot, device_manager_);
  if (for_backup)
    {
      /* no undo history in backups */
      if (ret->undo_manager_ != nullptr)
        {
          delete ret->undo_manager_;
          ret->undo_manager_ = nullptr;
        }
    }
  return ret;
}

void
to_json (nlohmann::json &j, const Project &project)
{
  j[utils::serialization::kDocumentTypeKey] = Project::DOCUMENT_TYPE;
  j[utils::serialization::kFormatMajorKey] = Project::FORMAT_MAJOR_VER;
  j[utils::serialization::kFormatMinorKey] = Project::FORMAT_MINOR_VER;
  j[Project::kTempoMapKey] = project.tempo_map_;
  j[Project::kFileAudioSourceRegistryKey] = project.file_audio_source_registry_;
  j[Project::kPortRegistryKey] = project.port_registry_;
  j[Project::kParameterRegistryKey] = project.param_registry_;
  j[Project::kPluginRegistryKey] = project.plugin_registry_;
  j[Project::kArrangerObjectRegistryKey] = project.arranger_object_registry_;
  j[Project::kTrackRegistryKey] = project.track_registry_;
  j[Project::kTitleKey] = project.title_;
  j[Project::kDatetimeKey] = project.datetime_str_;
  j[Project::kVersionKey] = project.version_;
  j[Project::kClipEditorKey] = project.clip_editor_;
  j[Project::kTimelineKey] = project.timeline_;
  j[Project::kSnapGridTimelineKey] = project.snap_grid_timeline_;
  j[Project::kSnapGridEditorKey] = project.snap_grid_editor_;
  j[Project::kQuantizeOptsTimelineKey] = project.quantize_opts_timeline_;
  j[Project::kQuantizeOptsEditorKey] = project.quantize_opts_editor_;
  j[Project::kTransportKey] = project.transport_;
  j[Project::kAudioEngineKey] = project.audio_engine_;
  j[Project::kAudioPoolKey] = project.pool_;
  j[Project::kTracklistKey] = project.tracklist_;
  // j[Project::kRegionLinkGroupManagerKey] =
  // project.region_link_group_manager_;
  j[Project::kPortConnectionsManagerKey] = project.port_connections_manager_;
  j[Project::kMidiMappingsKey] = project.midi_mappings_;
  j[Project::kUndoManagerKey] = project.undo_manager_;
  j[Project::kLastSelectionKey] = project.last_selection_;
}

struct ArrangerObjectBuilderForDeserialization
{
  ArrangerObjectBuilderForDeserialization (const Project &project)
      : project_ (project)
  {
  }

  template <typename T> std::unique_ptr<T> build () const
  {
    return project_.getArrangerObjectFactory ()->get_builder<T> ().build_empty ();
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
    return project_.getTrackFactory ()
      ->get_builder<T> ()
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
            return project_.get_path (ProjectPath::PluginStates, false);
          });
      }
    else
      {
        return std::make_unique<T> (
          plugins::Plugin::ProcessorBaseDependencies{
            .port_registry_ = project_.get_port_registry (),
            .param_registry_ = project_.get_param_registry () },
          [this] () {
            return project_.get_path (ProjectPath::PluginStates, false);
          },
          [] (
            const juce::PluginDescription &description,
            double initialSampleRate, int initialBufferSize,
            juce::AudioPluginFormat::PluginCreationCallback callback) {
            Zrythm::getInstance ()
              ->getPluginManager ()
              ->get_format_manager ()
              ->createPluginInstanceAsync (
                description, initialSampleRate, initialBufferSize, callback);
          },
          [this] () { return project_.audio_engine_->get_sample_rate (); },
          [this] () { return project_.audio_engine_->get_block_length (); },
          juce_plugin_toplevel_window_provider);
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
  j.at (Project::kDatetimeKey).get_to (project.datetime_str_);
  j.at (Project::kVersionKey).get_to (project.version_);
  j.at (Project::kClipEditorKey).get_to (*project.clip_editor_);
  j.at (Project::kTimelineKey).get_to (*project.timeline_);
  j.at (Project::kSnapGridTimelineKey).get_to (project.snap_grid_timeline_);
  j.at (Project::kSnapGridEditorKey).get_to (project.snap_grid_editor_);
  j.at (Project::kQuantizeOptsTimelineKey)
    .get_to (project.quantize_opts_timeline_);
  j.at (Project::kQuantizeOptsEditorKey).get_to (project.quantize_opts_editor_);
  j.at (Project::kTransportKey).get_to (*project.transport_);
  j.at (Project::kAudioEngineKey).get_to (project.audio_engine_);
  project.pool_ = std::make_unique<dsp::AudioPool> (
    *project.file_audio_source_registry_,
    [&project] (bool backup) {
      return project.get_path (ProjectPath::POOL, backup);
    },
    [&project] () { return project.audio_engine_->get_sample_rate (); });
  j.at (Project::kAudioPoolKey).get_to (*project.pool_);
  j.at (Project::kTracklistKey).get_to (*project.tracklist_);
  // j.at (Project::kRegionLinkGroupManagerKey)
  //   .get_to (project.region_link_group_manager_);
  j.at (Project::kPortConnectionsManagerKey)
    .get_to (*project.port_connections_manager_);
  j.at (Project::kMidiMappingsKey).get_to (*project.midi_mappings_);
  j.at (Project::kUndoManagerKey).get_to (*project.undo_manager_);
  j.at (Project::kLastSelectionKey).get_to (project.last_selection_);

  project.tracklist_->get_chord_track ()->set_note_pitch_to_descriptor_func (
    [&project] (midi_byte_t note_pitch) {
      return project.getClipEditor ()
        ->getChordEditor ()
        ->get_chord_from_note_number (note_pitch);
    });
}
