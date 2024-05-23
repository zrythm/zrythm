// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Graph exporter.
 */

#ifndef __AUDIO_GRAPH_EXPORT_H__
#define __AUDIO_GRAPH_EXPORT_H__

#include "zrythm-config.h"

typedef struct Graph Graph;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Export type.
 */
enum class GraphExportType
{
#ifdef HAVE_CGRAPH
  GRAPH_EXPORT_PNG,
  GRAPH_EXPORT_DOT,
  GRAPH_EXPORT_PS,
  GRAPH_EXPORT_SVG,
#endif
  NUM_GRAPH_EXPORT_TYPES,
};

void
graph_export_as_simple (GraphExportType type, const char * export_path);

/**
 * Exports the graph at the given path.
 *
 * Engine must be paused before calling this.
 */
void
graph_export_as (Graph * graph, GraphExportType type, const char * path);

/**
 * @}
 */

#endif
