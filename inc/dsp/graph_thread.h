// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2017, 2019 Robin Gareus <robin@gareus.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---
 */

#ifndef __AUDIO_GRAPH_THREAD_H__
#define __AUDIO_GRAPH_THREAD_H__

#include "zrythm-config.h"

#include "utils/types.h"

#ifdef HAVE_LSP_DSP
#  include <lsp-plug.in/dsp/dsp.h>
#endif

#include "ext/juce/juce.h"

class Graph;

/**
 * @addtogroup dsp
 *
 * @{
 */

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
   * @param graph The graph to set to the thread.
   * @param is_main Whether main thread.
   * @throw ZrythmException if the thread could not be started.
   */
  explicit GraphThread (const int id, const bool is_main, Graph &graph);

public:
  /**
   * Called from a terminal node (from the Graph worker-thread)
   * to indicate it has completed processing.
   *
   * The thread of the last terminal node that reaches here will
   * inform the main-thread, wait, and kick off the next
   * process cycle.
   */
  ATTR_HOT void on_reached_terminal_node ();

private:
  void run () override;

  /**
   * @brief The actual worker thread.
   */
  void run_worker ();

public:
  /**
   * Thread index in zrythm.
   *
   * The main thread will be -1 and the rest in sequence starting from 0.
   */
  int id_ = -1;

  bool is_main_ = false;

  /**
   * @brief Realtime thread ID.
   *
   * @see @ref RTThreadId.
   */
  unsigned int rt_thread_id_ = 0;

  /** Pointer back to the graph. */
  Graph &graph_;

#ifdef HAVE_LSP_DSP
  /** LSP DSP context. */
  lsp::dsp::context_t lsp_ctx_ = {};
#endif
};

/**
 * @}
 */

#endif
