/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

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
typedef enum GraphExportType
{
#ifdef HAVE_CGRAPH
  GRAPH_EXPORT_PNG,
  GRAPH_EXPORT_DOT,
  GRAPH_EXPORT_PS,
  GRAPH_EXPORT_SVG,
#endif
  NUM_GRAPH_EXPORT_TYPES,
} GraphExportType;

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
