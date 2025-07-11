// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph.h"
#include "utils/string.h"

namespace zrythm::dsp::graph
{

/**
 * @brief Utility class for exporting DSP graphs.
 *
 * Provides methods to generate DOT graph descriptions from DSP graphs
 * for visualization and debugging purposes.
 */
class GraphExport final
{
public:
  /**
   * @brief Export a DSP graph to DOT format.
   *
   * @param graph Graph to export
   * @param include_class_names Whether to include class names in node labels
   * @return DOT representation of the graph as UTF-8 string
   *
   * The generated DOT graph includes:
   * - Node names
   * - Class names (when requested)
   * - Connections between nodes
   *
   * Example usage of the output:
   * @code{bash}
   * dot -Tpng graph.dot -o graph.png
   * @endcode
   */
  static utils::Utf8String
  export_to_dot (const Graph &graph, bool include_class_names = false);
};

} // namespace zrythm::dsp::graph
