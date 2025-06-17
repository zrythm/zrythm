// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "dsp/ditherer.h"
#include "dsp/midi_event.h"
#include "dsp/position.h"
#include "engine/device_io/engine.h"
#include "engine/session/exporter.h"
#include "engine/session/router.h"
#include "engine/session/transport.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/ui.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/marker_track.h"
#include "structure/tracks/master_track.h"
#include "structure/tracks/piano_roll_track.h"
#include "structure/tracks/tracklist.h"
#include "utils/gtest_wrapper.h"
#include "utils/io.h"
#include "utils/logger.h"
#include "utils/math.h"
#include "utils/progress_info.h"
#include "utils/views.h"

#include "juce_wrapper.h"
#include <midilib/src/midifile.h>

#define AMPLITUDE (1.0 * 0x7F000000)

namespace zrythm::engine::session
{
Exporter::ExportThread::ExportThread (Exporter &exporter)
    : juce::Thread ("ExportThread"), exporter_ (exporter)
{
}

Exporter::ExportThread::~ExportThread ()
{
  stopThread (-1);
}

void
Exporter::ExportThread::run ()
{
  exporter_.export_to_file ();
}

Exporter::Exporter (
  Settings                      settings,
  std::shared_ptr<ProgressInfo> progress_info,
  QObject *                     parent)
    : QObject (parent), settings_ (std::move (settings)),
      progress_info_ (
        progress_info ? progress_info : std::make_shared<ProgressInfo> ())
{
}

void
Exporter::begin_generic_thread ()
{
  thread_ = std::make_unique<ExportThread> (*this);
  thread_->startThread ();
}

void
Exporter::join_generic_thread ()
{
  if (thread_)
    {
      thread_->stopThread (-1);
    }
}

const char *
Exporter::format_get_ext (Format format)
{

  static constexpr const char * format_exts[] = {
    "aiff", "au",  "caf", "flac", "mp3", "ogg",
    "ogg",  "raw", "wav", "w64",  "mid", "mid",
  };
  return format_exts[static_cast<int> (format)];
}

std::pair<Exporter::Position, Exporter::Position>
Exporter::Settings::get_export_time_range () const
{
  switch (time_range_)
    {
    case Exporter::TimeRange::Song:
      {
        auto start = P_MARKER_TRACK->get_start_marker ();
        auto end = P_MARKER_TRACK->get_end_marker ();
        return { start->get_position (), end->get_position () };
      }
    case Exporter::TimeRange::Loop:
      return {
        TRANSPORT->loop_start_pos_->get_position (),
        TRANSPORT->loop_end_pos_->get_position ()
      };
    case Exporter::TimeRange::Custom:
      return { custom_start_, custom_end_ };
    default:
      z_return_val_if_reached (std::make_pair (Position (), Position ()));
    }
}

fs::path
Exporter::get_exported_path () const
{
  return settings_.file_uri_;
}

void
Exporter::export_audio (Settings &info)
{
  juce::AudioFormatManager formatManager;
  formatManager.registerBasicFormats ();

  static constexpr int EXPORT_CHANNELS = 2;

  std::unique_ptr<juce::AudioFormat> format;
  switch (info.format_)
    {
    case Exporter::Format::WAV:
      format = std::make_unique<juce::WavAudioFormat> ();
      break;
    case Exporter::Format::AIFF:
      format = std::make_unique<juce::AiffAudioFormat> ();
      break;
    case Exporter::Format::FLAC:
      format = std::make_unique<juce::FlacAudioFormat> ();
      break;
    case Exporter::Format::Vorbis:
      format = std::make_unique<juce::OggVorbisAudioFormat> ();
      break;
    default:
      throw ZrythmException ("Unsupported export format");
    }

  auto outputFile =
    utils::Utf8String::from_path (info.file_uri_).to_juce_file ();
  if (!outputFile.getParentDirectory ().createDirectory ())
    {
      throw ZrythmException ("Failed to create parent directories");
    }

  auto file_output_stream = new juce::FileOutputStream (outputFile);
  if (!file_output_stream->openedOk ())
    {
      throw ZrythmException ("Failed to open output file");
    }

  juce::StringPairArray metadata;
  metadata.set (
    "title",
    (info.title_.empty () ? PROJECT->title_ : info.title_).to_juce_string ());
  if (!info.artist_.empty ())
    metadata.set ("artist", info.artist_.to_juce_string ());
  if (!info.genre_.empty ())
    metadata.set ("genre", info.genre_.to_juce_string ());
  metadata.set ("software", PROGRAM_NAME);

  juce::AudioFormatWriter * writer = format->createWriterFor (
    file_output_stream, AUDIO_ENGINE->get_sample_rate (), EXPORT_CHANNELS,
    utils::audio::bit_depth_enum_to_int (info.depth_), metadata, 0);
  if (writer == nullptr)
    {
      throw ZrythmException ("Failed to create audio writer");
    }

  std::unique_ptr<juce::AudioFormatWriter> writer_ptr (writer);

  auto [start_pos, end_pos] = info.get_export_time_range ();

  bool  clipped = false;
  float clip_amp = 0.f;

  const auto prev_playhead_ticks = TRANSPORT->playhead_.position_ticks ();
  TRANSPORT->playhead_.set_position_ticks (start_pos.ticks_);
  {
    dsp::PlayheadProcessingGuard guard (TRANSPORT->playhead_);

    AUDIO_ENGINE->bounce_mode_ =
      info.mode_ == Mode::Full
        ? engine::device_io::BounceMode::Off
        : engine::device_io::BounceMode::On;
    AUDIO_ENGINE->bounce_step_ = info.bounce_step_;
    AUDIO_ENGINE->bounce_with_parents_ = info.bounce_with_parents_;

    /* init ditherer */
    zrythm::dsp::Ditherer ditherer;
    if (info.dither_)
      {
        z_debug (
          "dither {} bits", utils::audio::bit_depth_enum_to_int (info.depth_));
        ditherer.reset (utils::audio::bit_depth_enum_to_int (info.depth_));
      }

    z_return_if_fail (end_pos.frames_ >= 1 || start_pos.frames_ >= 0);
    const auto total_frames = (end_pos.frames_ - start_pos.frames_);
    /* frames written so far */
    signed_frame_t covered_frames = 0;

    zrythm::utils::audio::AudioBuffer buffer (
      EXPORT_CHANNELS, AUDIO_ENGINE->get_block_length ());

    do
      {
        /* calculate number of frames to process this time */
        const nframes_t nframes =
          end_pos.frames_ - TRANSPORT->get_playhead_position_in_audio_thread ();
        assert (nframes > 0);

        /* run process code */
        AUDIO_ENGINE->process_prepare (nframes);
        EngineProcessTimeInfo time_nfo = {
          .g_start_frame_ =
            (unsigned_frame_t) TRANSPORT->get_playhead_position_in_audio_thread (),
          .g_start_frame_w_offset_ =
            (unsigned_frame_t) TRANSPORT->get_playhead_position_in_audio_thread (),
          .local_offset_ = 0,
          .nframes_ = nframes,
        };
        ROUTER->start_cycle (time_nfo);
        AUDIO_ENGINE->post_process (nframes, nframes);

        /* by this time, the Master channel should have its Stereo Out ports
         * filled - pass its buffers to the output */
        for (int i = 0; i < EXPORT_CHANNELS; ++i)
          {
            auto &ch_data =
              i == 0
                ? P_MASTER_TRACK->channel_->get_stereo_out_ports ().first.buf_
                : P_MASTER_TRACK->channel_->get_stereo_out_ports ().second.buf_;
            buffer.copyFrom (i, 0, ch_data.data (), (int) nframes);
          }

        /* clipping detection */
        float max_amp = buffer.getMagnitude (0, (int) nframes);
        if (max_amp > 1.f && max_amp > clip_amp)
          {
            clip_amp = max_amp;
            clipped = true;
          }

        /* apply dither */
        if (info.dither_)
          {
            ditherer.process (
              buffer.getWritePointer (0), static_cast<int> (nframes));
            ditherer.process (
              buffer.getWritePointer (1), static_cast<int> (nframes));
          }

        /* write the frames for the current cycle */
        if (!writer->writeFromAudioSampleBuffer (
              buffer, 0, static_cast<int> (nframes)))
          {
            throw ZrythmException ("Failed to write audio data");
          }

        covered_frames += nframes;

        progress_info_->update_progress (
          (TRANSPORT->get_playhead_position_in_audio_thread ()
           - start_pos.frames_)
            / total_frames,
          {});
      }
    while (
      TRANSPORT->get_playhead_position_in_audio_thread () < end_pos.frames_
      && !progress_info_->pending_cancellation ());

    writer_ptr.reset ();

    if (!progress_info_->pending_cancellation ())
      {
        z_warn_if_fail (covered_frames == total_frames);
      }

    /* TODO silence output */

    progress_info_->update_progress (1.0, {});

    AUDIO_ENGINE->bounce_mode_ = engine::device_io::BounceMode::Off;
    AUDIO_ENGINE->bounce_with_parents_ = false;
  }
  TRANSPORT->move_playhead (prev_playhead_ticks, true, false, false);

  /* if cancelled, delete */
  if (progress_info_->pending_cancellation ())
    {
      outputFile.deleteFile ();
      progress_info_->mark_completed (
        ProgressInfo::CompletionType::CANCELLED, {});
    }
  else
    {
      z_debug ("successfully exported to {}", info.file_uri_);

      if (clipped)
        {
          float      max_db = utils::math::amp_to_dbfs (clip_amp);
          const auto warn_str = format_qstr (
            QObject::tr (
              "The exported audio contains segments louder than 0 dB (max detected %.1f dB)."),
            max_db);
          progress_info_->mark_completed (
            ProgressInfo::CompletionType::HAS_WARNING,
            utils::Utf8String::from_qstring (warn_str));
        }
      else
        {
          /* return normally */
          progress_info_->mark_completed (
            ProgressInfo::CompletionType::SUCCESS, {});
        }
    }
}

void
Exporter::export_midi (Settings &info)
{
  MIDI_FILE * mf;

  auto [start_pos, end_pos] = info.get_export_time_range ();

  if (
    (mf = midiFileCreate (
       utils::Utf8String::from_path (info.file_uri_).c_str (), TRUE))
    != nullptr)
    {
      /* Write tempo information out to track 1 */
      const auto &tempo_map = PROJECT->get_tempo_map ();
      midiSongAddTempo (mf, 1, (int) tempo_map.get_tempo_events ().front ().bpm);

      midiFileSetPPQN (mf, Position::TICKS_PER_QUARTER_NOTE);

      int midi_version = info.format_ == Exporter::Format::Midi0 ? 0 : 1;
      z_debug ("setting MIDI version to {}", midi_version);
      midiFileSetVersion (mf, midi_version);

      /* common time: 4 crochet beats, per bar */
      int beats_per_bar =
        tempo_map.get_time_signature_events ().front ().numerator;
      midiSongAddSimpleTimeSig (
        mf, 1, beats_per_bar, TRANSPORT->ticks_per_beat_);

      /* add generic export name if version 0 */
      if (midi_version == 0)
        {
          midiTrackAddText (mf, 1, textTrackName, info.title_.c_str ());
        }

      auto track_span = TRACKLIST->get_track_span ();
      for (const auto &[index, track_var] : utils::views::enumerate (track_span))
        {
          std::visit (
            [&] (auto &&track) {
              using TrackT = base_type<decltype (track)>;
              if constexpr (
                std::derived_from<TrackT, structure::tracks::PianoRollTrack>)
                {
                  std::unique_ptr<dsp::MidiEventVector> events;
                  if (midi_version == 0)
                    {
                      events = std::make_unique<dsp::MidiEventVector> ();
                    }

                  /* write track to midi file */
                  track->write_to_midi_file (
                    mf, midi_version == 0 ? events.get () : nullptr, &start_pos,
                    &end_pos, midi_version == 0 ? false : info.lanes_as_tracks_,
                    midi_version == 0 ? false : true);

                  if (events)
                    {
                      events->write_to_midi_file (mf, 1);
                    }
                }

              progress_info_->update_progress (
                (double) index / (double) track_span.size (), {});
            },
            track_var);
        }

      midiFileClose (mf);
    }

  progress_info_->mark_completed (ProgressInfo::CompletionType::SUCCESS, {});
}

void
Exporter::Settings::set_bounce_defaults (
  Format                   format,
  const fs::path          &filepath,
  const utils::Utf8String &bounce_name)
{
  format_ = format;
  artist_ = {};
  title_ = {};
  genre_ = {};
  depth_ = BitDepth::BIT_DEPTH_16;
  time_range_ = TimeRange::Custom;
  switch (mode_)
    {
    case Mode::Regions:
      {
        auto tl_sel = TRACKLIST->get_timeline_objects_in_range ();
        auto [start_obj, start_pos] =
          structure::arrangement::ArrangerObjectSpan{ tl_sel }
            .get_first_object_and_pos (true);
        auto [end_obj, end_pos] =
          structure::arrangement::ArrangerObjectSpan{ tl_sel }
            .get_last_object_and_pos (true, true);
        custom_start_ = start_pos;
        custom_end_ = end_pos;
      }
      break;
    case Mode::Tracks:
      disable_after_bounce_ =
        ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
          ? false
          : gui::SettingsManager::disableAfterBounce ();
      [[fallthrough]];
    case Mode::Full:
      {
        auto start = P_MARKER_TRACK->get_start_marker ();
        auto end = P_MARKER_TRACK->get_end_marker ();
        custom_start_ = start->get_position ();
        custom_end_ = end->get_position ();
      }
      break;
    }
  custom_end_.add_ms (
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 100
      : gui::SettingsManager::bounceTailLength (),
    AUDIO_ENGINE->get_sample_rate (), AUDIO_ENGINE->ticks_per_frame_);

  bounce_step_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? BounceStep::PostFader
      : ENUM_INT_TO_VALUE (BounceStep, gui::SettingsManager::bounceStep ());
  bounce_with_parents_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? true
      : gui::SettingsManager::bounceWithParents ();

  if (!filepath.empty ())
    {
      file_uri_ = filepath;
    }
  else
    {
      auto tmp_dir = utils::io::make_tmp_dir (
        std::make_optional (QString::fromUtf8 ("zrythm_bounce_XXXXXX")));
      tmp_dir->setAutoRemove (false);
      const char * ext = format_get_ext (format_);
      auto         filename =
        bounce_name + u8"." + utils::Utf8String::from_utf8_encoded_string (ext);
      file_uri_ =
        utils::Utf8String::from_qstring (tmp_dir->path ()).to_path () / filename;
    }
}

void
Exporter::prepare_tracks_for_export (
  engine::device_io::AudioEngine &engine,
  Transport                      &transport)
{
  AUDIO_ENGINE->preparing_to_export_ = true;
  state_ = std::make_unique<engine::device_io::AudioEngine::State> ();

  AUDIO_ENGINE->wait_for_pause (*state_, false, true);
  z_info ("engine paused");

  TRANSPORT->play_state_ = Transport::PlayState::Rolling;

  AUDIO_ENGINE->exporting_ = true;
  AUDIO_ENGINE->preparing_to_export_ = false;
  TRANSPORT->loop_ = false;

  z_info ("deactivating and reactivating plugins");

  /* deactivate and activate all plugins to make them reset their states */
  /* TODO this doesn't reset the plugin state as expected, so sending note off
   * is needed */
  TRACKLIST->get_track_span ().activate_all_plugins (false);
  TRACKLIST->get_track_span ().activate_all_plugins (true);

  connections_ =
    std::make_unique<dsp::PortConnectionsManager::ConnectionsVector> ();
  if (settings_.mode_ != Exporter::Mode::Full)
    {
      /* disconnect all track faders from their channel outputs so that sends
       * and custom connections will work */
      for (
        auto cur_tr :
        TRACKLIST->get_track_span ()
          | std::views::filter (
            structure::tracks::TrackSpan::derived_from_type_projection<
              structure::tracks::ChannelTrack>)
          | std::views::transform (
            structure::tracks::TrackSpan::derived_type_transformation<
              structure::tracks::ChannelTrack>))
        {
          if (
            cur_tr->bounce_
            || cur_tr->get_output_signal_type () != dsp::PortType::Audio)
            continue;

          const auto &l_src_id =
            cur_tr->channel_->fader_->get_stereo_out_left_id ();
          const auto &l_dest_id =
            cur_tr->channel_->get_stereo_out_ports ().first.get_uuid ();
          auto l_conn =
            PORT_CONNECTIONS_MGR->get_connection (l_src_id, l_dest_id);
          z_return_if_fail (l_conn);
          connections_->push_back (utils::clone_qobject (*l_conn, this));
          PORT_CONNECTIONS_MGR->remove_connection (l_src_id, l_dest_id);

          const auto &r_src_id =
            cur_tr->channel_->fader_->get_stereo_out_right_id ();
          const auto &r_dest_id =
            cur_tr->channel_->get_stereo_out_ports ().second.get_uuid ();
          auto r_conn =
            PORT_CONNECTIONS_MGR->get_connection (r_src_id, r_dest_id);
          z_return_if_fail (r_conn);
          connections_->push_back (utils::clone_qobject (*r_conn, this));
          PORT_CONNECTIONS_MGR->remove_connection (r_src_id, r_dest_id);
        }

      /* recalculate the graph to apply the changes */
      ROUTER->recalc_graph (false);

      /* remark all tracks for bounce */
      TRACKLIST->mark_all_tracks_for_bounce (true);
    }

  z_debug ("preparing playback snapshots...");
  TRACKLIST->get_track_span ().set_caches (CacheType::PlaybackSnapshots);
}

void
Exporter::post_export ()
{
  /* this must be called after prepare_tracks_for_export() */
  z_return_if_fail (state_ != nullptr);

  /* not needed when exporting full */
  if (settings_.mode_ != Mode::Full)
    {
      /* ditto*/
      z_return_if_fail (connections_ != nullptr);

      /* re-connect disconnected connections */
      for (const auto &conn : *connections_)
        {
          PORT_CONNECTIONS_MGR->add_connection (*conn);
        }
      connections_.reset ();

      /* recalculate the graph to apply the changes */
      ROUTER->recalc_graph (false);
    }

  /* reset "bounce to master" on each track */
  std::ranges::for_each (TRACKLIST->get_track_span (), [] (const auto &track) {
    auto tr = structure::tracks::Track::from_variant (track);
    tr->bounce_to_master_ = false;
  });

  /* restart engine */
  AUDIO_ENGINE->exporting_ = false;
  AUDIO_ENGINE->resume (*state_);
  state_.reset ();
}

void
Exporter::Settings::print () const
{
  const auto time_range_type_str = Exporter_TimeRange_to_string (time_range_);
  utils::Utf8String time_range;
  if (time_range_ == Exporter::TimeRange::Custom)
    {
      const auto &tempo_map = PROJECT->get_tempo_map ();
      const auto  beats_per_bar =
        tempo_map.get_time_signature_events ().front ().numerator;
      time_range = utils::Utf8String::from_utf8_encoded_string (
        fmt::format (
          "Custom: {} ~ {}",
          custom_start_.to_string (
            beats_per_bar, TRANSPORT->sixteenths_per_beat_,
            AUDIO_ENGINE->frames_per_tick_),
          custom_end_.to_string (
            beats_per_bar, TRANSPORT->sixteenths_per_beat_,
            AUDIO_ENGINE->frames_per_tick_)));
    }
  else
    {
      time_range = time_range_type_str;
    }

  z_debug (
    "~~~ Export Settings ~~~\n"
    "format: {}\n"
    "artist: {}\n"
    "title: {}\n"
    "genre: {}\n"
    "bit depth: {}\n"
    "time range: {}\n"
    "export mode: {}\n"
    "disable after bounce: {}\n"
    "bounce with parents: {}\n"
    "bounce step: {}\n"
    "dither: {}\n"
    "file: {}\n"
    "num files: {}\n",
    Exporter_Format_to_string (format_), artist_, title_, genre_,
    utils::audio::bit_depth_enum_to_int (depth_), time_range,
    Exporter_Mode_to_string (mode_), disable_after_bounce_, bounce_with_parents_,
    BounceStep_to_string (bounce_step_), dither_, file_uri_, num_files_);
}

void
Exporter::create_audio_track_after_bounce (Position pos)
{
  /* assert exporting is finished */
  z_return_if_fail (!AUDIO_ENGINE->exporting_);

  FileDescriptor descr (settings_.file_uri_);

  /* find next track */
  std::optional<structure::tracks::TrackPtrVariant> last_track_var;
  std::optional<structure::tracks::TrackPtrVariant> track_to_disable_var;
  auto tl_sel = TRACKLIST->get_timeline_objects_in_range ();
  switch (settings_.mode_)
    {
    case Mode::Regions:
      last_track_var =
        structure::arrangement::ArrangerObjectSpan{ tl_sel }
          .get_first_and_last_track ()
          .second;
      break;
    case Mode::Tracks:
      last_track_var =
        TRACKLIST->get_track_span ().get_selected_tracks ().back ();
      if (settings_.disable_after_bounce_)
        {
          track_to_disable_var = last_track_var;
        }
      break;
    case Mode::Full:
      z_return_if_reached ();
      break;
    }
  z_return_if_fail (last_track_var.has_value ());

  Position tmp = TRANSPORT->get_playhead_position_in_gui_thread ();
  TRANSPORT->getPlayhead ()->setTicks (settings_.custom_start_.ticks_);
  try
    {
      std::visit (
        [&] (auto &&last_track, auto &&track_to_disable) {
          structure::tracks::Track::create_with_action (
            structure::tracks::Track::Type::Audio, nullptr, &descr, &pos,
            last_track->get_index () + 1, 1,
            track_to_disable ? track_to_disable->get_index () : -1, nullptr);
        },
        *last_track_var, *track_to_disable_var);
    }
  catch (const ZrythmException &ex)
    {
      ex.handle (QObject::tr ("Failed to create audio track"));
    }

  TRANSPORT->getPlayhead ()->setTicks (tmp.ticks_);
}

void
Exporter::export_to_file ()
{
  z_return_if_fail (!settings_.file_uri_.empty ());

  z_debug ("exporting to {}", settings_.file_uri_);

  settings_.print ();

  /* validate */
  if (settings_.time_range_ == Exporter::TimeRange::Custom)
    {
      Position init_pos;
      init_pos.set_to_bar (
        1, TRANSPORT->ticks_per_bar_, AUDIO_ENGINE->frames_per_tick_);
      if (
        settings_.custom_start_ >= settings_.custom_end_
        || settings_.custom_start_ < init_pos)
        {
          progress_info_->mark_completed (
            ProgressInfo::CompletionType::HAS_ERROR,
            utils::Utf8String::from_qstring (QObject::tr ("Invalid time range")));
          z_warning ("invalid time range");
          return; // FIXME: throw exception?
        }
    }

  try
    {
      if (
        settings_.format_ == Exporter::Format::Midi0
        || settings_.format_ == Exporter::Format::Midi1)
        {
          export_midi (settings_);
        }
      else
        {
          export_audio (settings_);
        }
    }
  catch (const ZrythmException &ex)
    {
      ex.handle (QObject::tr ("Failed to export"));
    }

  z_debug ("done exporting");
}
}
