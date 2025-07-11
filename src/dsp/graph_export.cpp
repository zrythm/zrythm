// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <sstream>

#include "dsp/graph_export.h"

#include <QObject>
#include <QString>

namespace zrythm::dsp::graph
{

utils::Utf8String
GraphExport::export_to_dot (const graph::Graph &graph, bool include_class_names)
{
  std::stringstream ss;
  ss << "digraph G {\n";

  for (const auto &node : graph.setup_nodes_.graph_nodes_)
    {
      const auto &processable = node->get_processable ();

      // Node definition
      ss << "  " << node->get_id () << " [label=\""
         << processable.get_node_name ().view ();

      // Add class name if requested
      if (include_class_names)
        {
          try
            {
              auto   &qobj = dynamic_cast<QObject &> (node->get_processable ());
              QString className =
                QString (qobj.metaObject ()->className ()).section ("::", -1);
              ss << "\\n(" << className.toStdString () << ")";
            }
          catch (const std::bad_cast &)
            {
              // Fallback to typeid for non-QObject classes
              ss << "\\n(" << typeid (processable).name () << ")";
            }
        }

      ss << "\"];\n";

      // Connections
      for (const auto &child : node->childnodes_)
        {
          ss << "  " << node->get_id () << " -> " << child.get ().get_id ()
             << ";\n";
        }
    }

  ss << "}\n";

  return utils::Utf8String::from_utf8_encoded_string (ss.view ());
}
} // namespace zrythm::dsp::graph
