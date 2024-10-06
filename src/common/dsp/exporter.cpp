// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <cstdio>

#include "common/dsp/channel.h"
#include "common/dsp/ditherer.h"
#include "common/dsp/engine.h"
#include "common/dsp/piano_roll_track.h"
#include "common/dsp/tracklist.h"
#include "common/utils/gtest_wrapper.h"
#include "common/utils/logger.h"
#include "gui/backend/backend/zrythm.h"
#if HAVE_JACK
#  include "common/dsp/engine_jack.h"
#endif
#include "common/dsp/exporter.h"
#include "common/dsp/marker_track.h"
#include "common/dsp/master_track.h"
#include "common/dsp/midi_event.h"
#include "common/dsp/position.h"
#include "common/dsp/router.h"
#include "common/dsp/tempo_track.h"
#include "common/dsp/transport.h"
#include "common/utils/flags.h"
#include "common/utils/io.h"
#include "common/utils/math.h"
#include "common/utils/progress_info.h"
#include "common/utils/ui.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/g_settings_manager.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#include "juce/juce.h"
#include "midilib/src/midifile.h"
#include <glibmm.h>
#include <sndfile.h>

#define AMPLITUDE (1.0 * 0x7F000000)

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
  GtkWidget *                   parent_owner,
  std::shared_ptr<ProgressInfo> progress_info)
    : settings_ (std::move (settings)),
      progress_info_ (
        progress_info ? progress_info : std::make_shared<ProgressInfo> ()),
      parent_owner_ (parent_owner)
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

std::pair<Position, Position>
Exporter::Settings::get_export_time_range () const
{
  switch (time_range_)
    {
    case Exporter::TimeRange::Song:
      {
        auto start = P_MARKER_TRACK->get_start_marker ();
        auto end = P_MARKER_TRACK->get_end_marker ();
        return { start->pos_, end->pos_ };
      }
    case Exporter::TimeRange::Loop:
      return { TRANSPORT->loop_start_pos_, TRANSPORT->loop_end_pos_ };
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

  juce::File outputFile (info.file_uri_);
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
  metadata.set ("title", info.title_.empty () ? PROJECT->title_ : info.title_);
  if (!info.artist_.empty ())
    metadata.set ("artist", info.artist_);
  if (!info.genre_.empty ())
    metadata.set ("genre", info.genre_);
  metadata.set ("software", PROGRAM_NAME);

  juce::AudioFormatWriter * writer = format->createWriterFor (
    file_output_stream, AUDIO_ENGINE->sample_rate_, EXPORT_CHANNELS,
    audio_bit_depth_enum_to_int (info.depth_), metadata, 0);
  if (writer == nullptr)
    {
      throw ZrythmException ("Failed to create audio writer");
    }

  std::unique_ptr<juce::AudioFormatWriter> writer_ptr (writer);

  auto [start_pos, end_pos] = info.get_export_time_range ();

  Position prev_playhead_pos = TRANSPORT->playhead_pos_;
  TRANSPORT->set_playhead_pos (start_pos);

  AUDIO_ENGINE->bounce_mode_ =
    info.mode_ == Mode::Full ? BounceMode::BOUNCE_OFF : BounceMode::BOUNCE_ON;
  AUDIO_ENGINE->bounce_step_ = info.bounce_step_;
  AUDIO_ENGINE->bounce_with_parents_ = info.bounce_with_parents_;

  /* set jack freewheeling mode and temporarily disable transport link */
#if HAVE_JACK
  AudioEngine::JackTransportType transport_type = AUDIO_ENGINE->transport_type_;
  if (AUDIO_ENGINE->audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK)
    {
      engine_jack_set_transport_type (
        AUDIO_ENGINE.get (), AudioEngine::JackTransportType::NoJackTransport);

      /* FIXME this is not how freewheeling should
       * work. see https://todo.sr.ht/~alextee/zrythm-feature/371 */
#  if 0
      z_info ("setting freewheel on");
      jack_set_freewheel (
        AUDIO_ENGINE->client, 1);
#  endif
    }
#endif

  /* init ditherer */
  Ditherer ditherer;
  memset (&ditherer, 0, sizeof (Ditherer));
  if (info.dither_)
    {
      z_debug ("dither {} bits", audio_bit_depth_enum_to_int (info.depth_));
      ditherer_reset (&ditherer, audio_bit_depth_enum_to_int (info.depth_));
    }

  z_return_if_fail (end_pos.frames_ >= 1 || start_pos.frames_ >= 0);
  const double total_ticks = (end_pos.ticks_ - start_pos.ticks_);
  /* frames written so far */
  double covered_ticks = 0;
  bool   clipped = false;
  float  clip_amp = 0.f;

  juce::AudioBuffer<float> buffer (EXPORT_CHANNELS, AUDIO_ENGINE->block_length_);

  do
    {
      /* calculate number of frames to process this time */
      const double    nticks = end_pos.ticks_ - TRANSPORT->playhead_pos_.ticks_;
      const nframes_t nframes = (nframes_t) MIN (
        (long) ceil (AUDIO_ENGINE->frames_per_tick_ * nticks),
        (long) AUDIO_ENGINE->block_length_);
      z_return_if_fail (nframes > 0);

      /* run process code */
      AUDIO_ENGINE->process_prepare (nframes);
      EngineProcessTimeInfo time_nfo = {
        .g_start_frame_ = (unsigned_frame_t) PLAYHEAD.frames_,
        .g_start_frame_w_offset_ = (unsigned_frame_t) PLAYHEAD.frames_,
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
              ? P_MASTER_TRACK->channel_->stereo_out_->get_l ().buf_
              : P_MASTER_TRACK->channel_->stereo_out_->get_r ().buf_;
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
          ditherer_process (&ditherer, buffer.getWritePointer (0), nframes);
          ditherer_process (&ditherer, buffer.getWritePointer (1), nframes);
        }

      /* write the frames for the current cycle */
      if (!writer->writeFromAudioSampleBuffer (buffer, 0, nframes))
        {
          throw ZrythmException ("Failed to write audio data");
        }

      covered_ticks += AUDIO_ENGINE->ticks_per_frame_ * nframes;

      progress_info_->update_progress (
        (TRANSPORT->playhead_pos_.ticks_ - start_pos.ticks_) / total_ticks,
        nullptr);
    }
  while (
    TRANSPORT->playhead_pos_.ticks_ < end_pos.ticks_
    && !progress_info_->pending_cancellation ());

  writer_ptr.reset ();

  if (!progress_info_->pending_cancellation ())
    {
      z_warn_if_fail (
        math_floats_equal_epsilon (covered_ticks, total_ticks, 1.0));
    }

  /* TODO silence output */

  progress_info_->update_progress (1.0, nullptr);

  /* set jack freewheeling mode and transport type */
#if HAVE_JACK
  if (AUDIO_ENGINE->audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK)
    {
      /* FIXME this is not how freewheeling should
       * work. see https://todo.sr.ht/~alextee/zrythm-feature/371 */
#  if 0
      z_info ("setting freewheel off");
      jack_set_freewheel (
        AUDIO_ENGINE->client, 0);
#  endif
      engine_jack_set_transport_type (AUDIO_ENGINE.get (), transport_type);
    }
#endif

  AUDIO_ENGINE->bounce_mode_ = BounceMode::BOUNCE_OFF;
  AUDIO_ENGINE->bounce_with_parents_ = false;
  TRANSPORT->move_playhead (&prev_playhead_pos, true, false, false);

  /* if cancelled, delete */
  if (progress_info_->pending_cancellation ())
    {
      outputFile.deleteFile ();
      progress_info_->mark_completed (
        ProgressInfo::CompletionType::CANCELLED, nullptr);
    }
  else
    {
      z_debug ("successfully exported to {}", info.file_uri_);

      if (clipped)
        {
          float       max_db = math_amp_to_dbfs (clip_amp);
          std::string warn_str = format_str (
            _ ("The exported audio contains segments louder than 0 dB (max detected %.1f dB)."),
            max_db);
          progress_info_->mark_completed (
            ProgressInfo::CompletionType::HAS_WARNING, warn_str.c_str ());
        }
      else
        {
          /* return normally */
          progress_info_->mark_completed (
            ProgressInfo::CompletionType::SUCCESS, nullptr);
        }
    }
}

void
Exporter::export_midi (Settings &info)
{
  MIDI_FILE * mf;

  auto [start_pos, end_pos] = info.get_export_time_range ();

  if ((mf = midiFileCreate (info.file_uri_.c_str (), TRUE)))
    {
      /* Write tempo information out to track 1 */
      midiSongAddTempo (mf, 1, (int) P_TEMPO_TRACK->get_current_bpm ());

      midiFileSetPPQN (mf, TICKS_PER_QUARTER_NOTE);

      int midi_version = info.format_ == Exporter::Format::Midi0 ? 0 : 1;
      z_debug ("setting MIDI version to {}", midi_version);
      midiFileSetVersion (mf, midi_version);

      /* common time: 4 crochet beats, per bar */
      int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
      midiSongAddSimpleTimeSig (
        mf, 1, beats_per_bar,
        math_round_double_to_signed_32 (TRANSPORT->ticks_per_beat_));

      /* add generic export name if version 0 */
      if (midi_version == 0)
        {
          midiTrackAddText (mf, 1, textTrackName, info.title_.c_str ());
        }

      for (size_t i = 0; i < TRACKLIST->tracks_.size (); ++i)
        {
          auto &track = TRACKLIST->tracks_[i];

          if (track->has_piano_roll ())
            {
              auto &piano_roll_track = dynamic_cast<PianoRollTrack &> (*track);
              std::unique_ptr<MidiEventVector> events;
              if (midi_version == 0)
                {
                  events = std::make_unique<MidiEventVector> ();
                }

              /* write track to midi file */
              piano_roll_track.write_to_midi_file (
                mf, midi_version == 0 ? events.get () : nullptr, &start_pos,
                &end_pos, midi_version == 0 ? false : info.lanes_as_tracks_,
                midi_version == 0 ? false : true);

              if (events)
                {
                  events->write_to_midi_file (mf, 1);
                }
            }

          progress_info_->update_progress (
            (double) i / (double) TRACKLIST->tracks_.size (), nullptr);
        }

      midiFileClose (mf);
    }

  progress_info_->mark_completed (
    ProgressInfo::CompletionType::SUCCESS, nullptr);
}

void
Exporter::Settings::set_bounce_defaults (
  Format             format,
  const std::string &filepath,
  const std::string &bounce_name)
{
  format_ = format;
  artist_ = "";
  title_ = "";
  genre_ = "";
  depth_ = BitDepth::BIT_DEPTH_16;
  time_range_ = TimeRange::Custom;
  switch (mode_)
    {
    case Mode::Regions:
      {
        auto [start_obj, start_pos] =
          TL_SELECTIONS->get_first_object_and_pos (true);
        auto [end_obj, end_pos] =
          TL_SELECTIONS->get_last_object_and_pos (true, true);
        custom_start_ = start_pos;
        custom_end_ = end_pos;
      }
      break;
    case Mode::Tracks:
      disable_after_bounce_ =
        ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
          ? false
          : g_settings_get_boolean (S_UI, "disable-after-bounce");
      [[fallthrough]];
    case Mode::Full:
      {
        auto start = P_MARKER_TRACK->get_start_marker ();
        auto end = P_MARKER_TRACK->get_end_marker ();
        custom_start_ = start->pos_;
        custom_end_ = end->pos_;
      }
      break;
    }
  custom_end_.add_ms (
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 100
      : g_settings_get_int (S_UI, "bounce-tail"));

  bounce_step_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? BounceStep::PostFader
      : ENUM_INT_TO_VALUE (BounceStep, g_settings_get_enum (S_UI, "bounce-step"));
  bounce_with_parents_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? true
      : g_settings_get_boolean (S_UI, "bounce-with-parents");

  if (!filepath.empty ())
    {
      file_uri_ = filepath;
    }
  else
    {
      std::string  tmp_dir = io_create_tmp_dir ("zrythm_bounce_XXXXXX");
      const char * ext = format_get_ext (format_);
      std::string  filename = bounce_name + "." + ext;
      file_uri_ = Glib::build_filename (tmp_dir, filename);
    }
}

void
Exporter::prepare_tracks_for_export (AudioEngine &engine, Transport &transport)
{
  AUDIO_ENGINE->preparing_to_export_ = true;
  state_ = std::make_unique<AudioEngine::State> ();

  AUDIO_ENGINE->wait_for_pause (*state_, Z_F_NO_FORCE, true);
  z_info ("engine paused");

  TRANSPORT->play_state_ = Transport::PlayState::Rolling;

  AUDIO_ENGINE->exporting_ = true;
  AUDIO_ENGINE->preparing_to_export_ = false;
  TRANSPORT->loop_ = false;

  z_info ("deactivating and reactivating plugins");

  /* deactivate and activate all plugins to make them reset their states */
  /* TODO this doesn't reset the plugin state as expected, so sending note off
   * is needed */
  TRACKLIST->activate_all_plugins (false);
  TRACKLIST->activate_all_plugins (true);

  connections_ = std::make_unique<std::vector<PortConnection>> ();
  if (settings_.mode_ != Exporter::Mode::Full)
    {
      /* disconnect all track faders from their channel outputs so that sends
       * and custom connections will work */
      for (auto cur_tr : TRACKLIST->tracks_ | type_is<ChannelTrack> ())
        {
          if (cur_tr->bounce_ || cur_tr->out_signal_type_ != PortType::Audio)
            continue;

          auto &l_src_id = cur_tr->channel_->fader_->stereo_out_->get_l ().id_;
          auto &l_dest_id = cur_tr->channel_->stereo_out_->get_l ().id_;
          auto  l_conn =
            PORT_CONNECTIONS_MGR->find_connection (l_src_id, l_dest_id);
          z_return_if_fail (l_conn);
          connections_->push_back (*l_conn);
          PORT_CONNECTIONS_MGR->ensure_disconnect (l_src_id, l_dest_id);

          auto &r_src_id = cur_tr->channel_->fader_->stereo_out_->get_r ().id_;
          auto &r_dest_id = cur_tr->channel_->stereo_out_->get_r ().id_;
          auto  r_conn =
            PORT_CONNECTIONS_MGR->find_connection (r_src_id, r_dest_id);
          z_return_if_fail (r_conn);
          connections_->push_back (*r_conn);
          PORT_CONNECTIONS_MGR->ensure_disconnect (r_src_id, r_dest_id);
        }

      /* recalculate the graph to apply the changes */
      ROUTER->recalc_graph (false);

      /* remark all tracks for bounce */
      TRACKLIST->mark_all_tracks_for_bounce (true);
    }

  z_debug ("preparing playback snapshots...");
  TRACKLIST->set_caches (CacheType::PlaybackSnapshots);
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
          PORT_CONNECTIONS_MGR->ensure_connect_from_connection (conn);
        }
      connections_.reset ();

      /* recalculate the graph to apply the changes */
      ROUTER->recalc_graph (false);
    }

  /* reset "bounce to master" on each track */
  for (auto &track : TRACKLIST->tracks_)
    {
      track->bounce_to_master_ = false;
    }

  /* restart engine */
  AUDIO_ENGINE->exporting_ = false;
  AUDIO_ENGINE->resume (*state_);
  state_.reset ();
}

void
Exporter::Settings::print () const
{
  const std::string time_range_type_str =
    Exporter_TimeRange_to_string (time_range_);
  std::string time_range;
  if (time_range_ == Exporter::TimeRange::Custom)
    {
      time_range = fmt::format (
        "Custom: {} ~ {}", custom_start_.to_string (), custom_end_.to_string ());
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
    audio_bit_depth_enum_to_int (depth_), time_range,
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
  Track * last_track = nullptr;
  Track * track_to_disable = nullptr;
  switch (settings_.mode_)
    {
    case Mode::Regions:
      last_track = TL_SELECTIONS->get_last_track ();
      break;
    case Mode::Tracks:
      last_track = TRACKLIST_SELECTIONS->get_lowest_track ();
      if (settings_.disable_after_bounce_)
        {
          track_to_disable = last_track;
        }
      break;
    case Mode::Full:
      z_return_if_reached ();
      break;
    }
  z_return_if_fail (last_track != nullptr);

  Position tmp = TRANSPORT->playhead_pos_;
  TRANSPORT->set_playhead_pos (settings_.custom_start_);
  try
    {
      Track::create_with_action (
        Track::Type::Audio, nullptr, &descr, &pos, last_track->pos_ + 1, 1,
        track_to_disable ? track_to_disable->pos_ : -1, nullptr);
    }
  catch (const ZrythmException &ex)
    {
      ex.handle (_ ("Failed to create audio track"));
    }

  TRANSPORT->set_playhead_pos (tmp);
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
      init_pos.set_to_bar (1);
      if (
        settings_.custom_start_ >= settings_.custom_end_
        || settings_.custom_start_ < init_pos)
        {
          progress_info_->mark_completed (
            ProgressInfo::CompletionType::HAS_ERROR, _ ("Invalid time range"));
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
      ex.handle (_ ("Failed to export"));
    }

  z_debug ("done exporting");
}
