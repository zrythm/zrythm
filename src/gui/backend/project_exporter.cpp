// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph_pruner.h"
#include "dsp/graph_renderer.h"
#include "engine/session/project_graph_builder.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/project_exporter.h"
#include "utils/audio_file_writer.h"

#include <QQmlEngine>
#include <QtConcurrentRun>

gui::qquick::QFutureQmlWrapper *
ProjectExporter::exportAudio (Project * project)
{
  dsp::GraphRenderer::RenderOptions options{
    .sample_rate_ = units::sample_rate (
      static_cast<int> (
        project->device_manager_->getCurrentAudioDevice ()
          ->getCurrentSampleRate ())),
    .block_length_ = units::samples (
      project->device_manager_->getCurrentAudioDevice ()
        ->getCurrentBufferSizeSamples ())
  };
  EngineState state{};
  project->engine ()->wait_for_pause (state, false, true);
  ProjectGraphBuilder builder (*project);
  dsp::graph::Graph   graph;
  builder.build_graph (graph);

  // Prune graph to master output
  {
    std::vector<std::reference_wrapper<dsp::graph::GraphNode>> terminals;
    auto * node = graph.get_nodes ().find_node_for_processable (
      *project->tracklist ()
         ->singletonTracks ()
         ->masterTrack ()
         ->channel ()
         ->audioOutPort ());
    terminals.emplace_back (*node);
    dsp::graph::GraphPruner::prune_graph_to_terminals (graph, terminals);
  }

  const auto * marker_track =
    project->tracklist ()->singletonTracks ()->markerTrack ();
  auto graph_render_future = dsp::GraphRenderer::render_async (
    options, graph.steal_nodes (),
    [context = project] (std::function<void ()> func) {
      QMetaObject::invokeMethod (context, func, Qt::BlockingQueuedConnection);
    },
    std::make_pair (
      units::samples (marker_track->get_start_marker ()->position ()->samples ()),
      units::samples (marker_track->get_end_marker ()->position ()->samples ())));

  auto combined_future =
    QtConcurrent::run (
      [title = project->getTitle (),
       sample_rate =
         project->device_manager_->getCurrentAudioDevice ()
           ->getCurrentSampleRate (),
       exports_path = project->get_path (ProjectPath::EXPORTS, false)] (
        QPromise<QStringList>           &promise,
        QFuture<juce::AudioSampleBuffer> inner_graph_render_future) {
        // Wait for task to establish its progress min/max
        while (inner_graph_render_future.progressMaximum () <= 0)
          {
            std::this_thread::sleep_for (1ms);
          }

        const auto render_task_progress_range = std::make_pair (
          inner_graph_render_future.progressMinimum (),
          inner_graph_render_future.progressMaximum ());
        const auto write_to_file_task_additional_progress = static_cast<int> (
          (render_task_progress_range.second - render_task_progress_range.first)
          * 0.1);
        promise.setProgressRange (
          inner_graph_render_future.progressMinimum (),
          inner_graph_render_future.progressMaximum ()
            + write_to_file_task_additional_progress);
        while (!inner_graph_render_future.isFinished ())
          {
            std::this_thread::sleep_for (5ms);
            promise.setProgressValueAndText (
              inner_graph_render_future.progressValue (),
              inner_graph_render_future.progressText ());
            if (promise.isCanceled ())
              {
                inner_graph_render_future.cancel ();
              }
          }

        if (
          inner_graph_render_future.isValid ()
          && inner_graph_render_future.isResultReadyAt (0))
          {
            std::unordered_map<juce::String, juce::String> metadata;
            metadata.emplace (
              juce::String ("title"),
              utils::Utf8String::from_qstring (title).to_juce_string ());
            metadata.emplace (
              juce::String ("software"), juce::String (PROGRAM_NAME));

            juce::AudioFormatWriterOptions juce_writer_options;
            juce_writer_options =
              juce_writer_options.withSampleRate (sample_rate)
                .withNumChannels (2)
                .withBitsPerSample (
                  utils::audio::bit_depth_enum_to_int (
                    zrythm::utils::audio::BitDepth::BIT_DEPTH_16))
                .withMetadataValues (metadata)
                .withQualityOptionIndex (0);
            utils::AudioFileWriter::WriteOptions write_options{
              .writer_options_ = juce_writer_options,
              .block_length_ = units::samples (4096)
            };
            const auto path =
              exports_path
              / (utils::Utf8String::from_qstring (title) + u8"- Mixdown.wav")
                  .to_path ();
            auto file_write_future = utils::AudioFileWriter::write_async (
              write_options, path, inner_graph_render_future.takeResult ());

            // Wait for task to establish its progress min/max
            while (file_write_future.progressMaximum () <= 0)
              {
                std::this_thread::sleep_for (1ms);
              }

            const auto file_write_progress_range = std::make_pair (
              file_write_future.progressMinimum (),
              file_write_future.progressMaximum ());
            while (!file_write_future.isFinished ())
              {
                std::this_thread::sleep_for (5ms);
                const auto progress_in_file_write =
                  file_write_future.progressValue ();
                const auto current_progress_ratio_in_file_write =
                  static_cast<double>(progress_in_file_write - file_write_progress_range.first) / (file_write_progress_range.second - file_write_progress_range.first);
                promise.setProgressValueAndText (
                  render_task_progress_range.second
                    + static_cast<int> (
                      write_to_file_task_additional_progress
                      * current_progress_ratio_in_file_write),
                  file_write_future.progressText ());
                if (promise.isCanceled ())
                  {
                    file_write_future.cancel ();
                    return;
                  }
              }
            promise.addResult (
              QStringList{ utils::Utf8String::from_path (path).to_qstring () });
          }
        else
          {
            z_debug ("cancelled or failed");
            promise.future ().cancel ();
          }
      },
      graph_render_future);

  const auto resume_engine = [engine = project->engine (), state] () {
    // FIXME: this is needed because node caches are not per-graph and
    // they were destroyed via the renderer.
    engine->graph_dispatcher ().recalc_graph (false);
    engine->resume (state);
  };

  // No matter what happens, we must resume the engine
  combined_future
    .then (
      project->engine (),
      [resume_engine] (QFuture<QStringList> result) { resume_engine (); })
    .onCanceled (
      project->engine (),
      [resume_engine] () {
        z_debug ("Audio export canceled");
        resume_engine ();
      })
    .onFailed (project->engine (), [resume_engine] () {
      z_warning ("Audio export failed");
      resume_engine ();
    });

  auto * future_qml_wrapper =
    new gui::qquick::QFutureQmlWrapperT (combined_future);

  // Transfer ownership to QML JavaScript engine for proper cleanup
  QQmlEngine::setObjectOwnership (
    future_qml_wrapper, QQmlEngine::JavaScriptOwnership);

  return future_qml_wrapper;
}
