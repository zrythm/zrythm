// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef ZRYTHM_DSP_GRAPH_THREAD_H
#define ZRYTHM_DSP_GRAPH_THREAD_H

#include "zrythm-config.h"

#include "utils/rt_thread_id.h"

#include "juce_wrapper.h"

namespace zrythm::dsp::graph
{

class GraphScheduler;

/**
 * @brief Processing graph thread.
 */
class GraphThread final : public juce::Thread
{
public:
  /**
   * Creates and starts a realtime thread.
   *
   * @param id The index of the thread.
   * @param is_main Whether main thread.
   * @param scheduler The scheduler that owns the thread.
   * @throw ZrythmException if the thread could not be started.
   */
  explicit GraphThread (
    int                                 id,
    bool                                is_main,
    GraphScheduler                     &scheduler,
    std::optional<juce::AudioWorkgroup> workgroup = std::nullopt);

public:
  /**
   * Called to indicate a terminal node has completed processing.
   *
   * The thread of the last terminal node that reaches here will
   * inform the main-thread, wait, and kick off the next
   * process cycle.
   */
  [[gnu::hot]] void on_reached_terminal_node ();

private:
  void run () override;

  /**
   * @brief The actual worker thread.
   */
  void run_worker () noexcept;

public:
  /**
   * Thread index in zrythm.
   *
   * The main thread will be -1 and the rest in sequence starting from 0.
   */
  int id_{ -1 };

  bool is_main_{ false };

  /**
   * @brief Realtime thread ID.
   */
  RTThreadId::IdType rt_thread_id_{};

  /** Pointer back to the owner scheduler. */
  GraphScheduler &scheduler_;

  /**
   * @brief Non-owning pointer (optional) to a workgroup to join.
   *
   * @see juce::AudioDeviceManager::getDeviceAudioWorkgroup().
   */
  std::optional<juce::AudioWorkgroup> audio_workgroup_;
};

} // namespace zrythm::dsp::graph

#endif
