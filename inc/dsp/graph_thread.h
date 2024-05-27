// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
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

/**
 * \file
 *
 * Routing graph thread.
 */

#ifndef __AUDIO_GRAPH_THREAD_H__
#define __AUDIO_GRAPH_THREAD_H__

#include "zrythm-config.h"

#include "utils/types.h"

#include "gtk_wrapper.h"
#include <pthread.h>

#ifdef HAVE_LSP_DSP
#  include <lsp-plug.in/dsp/dsp.h>
#endif

typedef struct Graph Graph;

/**
 * @addtogroup dsp
 *
 * @{
 */

typedef struct GraphThread
{
  pthread_t pthread;

  /**
   * Thread index in zrythm.
   *
   * The main thread will be -1 and the rest in
   * sequence starting from 0.
   */
  int id;

  /** Pointer back to the graph. */
  Graph * graph;

#ifdef HAVE_LSP_DSP
  /** LSP DSP context. */
  lsp::dsp::context_t lsp_ctx;
#endif
} GraphThread;

/**
 * Creates a thread.
 *
 * @param id The index of the thread.
 * @param graph The graph to set to the thread.
 * @param is_main 1 if main thread.
 */
GraphThread *
graph_thread_new (const int id, const bool is_main, Graph * graph);

/**
 * @}
 */

#endif
