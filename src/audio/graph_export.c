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

#include "audio/fader.h"
#include "audio/graph_export.h"
#include "audio/passthrough_processor.h"
#include "audio/port.h"
#include "audio/routing.h"
#include "audio/track.h"
#include "plugins/plugin.h"

#ifdef HAVE_CGRAPH
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#endif

#ifdef HAVE_CGRAPH

typedef struct ANode
{
  GraphNode * node;

  /** Subgraph the node is a part of. */
  Agraph_t *  graph;
} ANode;

static Agraph_t *
get_graph_from_node (
  ANode *    anodes,
  int         num_anodes,
  GraphNode * node)
{
  for (int i = 0; i < num_anodes; i++)
    {
      ANode * anode = &anodes[i];
      if (anode->node->id == node->id)
        return anode->graph;
    }
  g_warning (
    "%p %s", node, graph_get_node_name (node));
  g_return_val_if_reached (NULL);
}

static Agraph_t *
get_parent_graph (
  ANode *    anodes,
  int         num_anodes,
  GraphNode * node)
{
  GraphNode * parent_node = NULL;
  switch (node->type)
    {
    case ROUTE_NODE_TYPE_TRACK:
    case ROUTE_NODE_TYPE_PLUGIN:
    case ROUTE_NODE_TYPE_FADER:
    case ROUTE_NODE_TYPE_PREFADER:
      parent_node = node;
      break;
    case ROUTE_NODE_TYPE_PORT:
      {
        switch (node->port->id.owner_type)
          {
          case PORT_OWNER_TYPE_PLUGIN:
            {
              Plugin * pl =
                port_get_plugin (node->port, true);
              parent_node =
                graph_find_node_from_plugin (
                  node->graph, pl);
            }
            break;
          case PORT_OWNER_TYPE_TRACK:
            {
              Track * tr =
                port_get_track (node->port, true);
              parent_node =
                graph_find_node_from_track (
                  node->graph, tr);
            }
            break;
          case PORT_OWNER_TYPE_PREFADER:
            {
              Track * tr =
                port_get_track (node->port, true);
              parent_node =
                graph_find_node_from_prefader (
                  node->graph,
                  &tr->channel->prefader);
            }
            break;
          case PORT_OWNER_TYPE_FADER:
            {
              Track * tr =
                port_get_track (node->port, true);
              parent_node =
                graph_find_node_from_fader (
                  node->graph,
                  &tr->channel->fader);
            }
            break;
          case PORT_OWNER_TYPE_TRACK_PROCESSOR:
            {
              Track * tr =
                port_get_track (node->port, true);
              parent_node =
                graph_find_node_from_track (
                  node->graph, tr);
            }
            break;
          default:
            break;
          }
      }
      break;
    default:
      break;
    }
  if (!parent_node)
    return NULL;

  return
    get_graph_from_node (
      anodes, num_anodes, parent_node);
}

static Agnode_t *
create_anode (
  Agraph_t *  aroot_graph,
  GraphNode * node,
  ANode *    anodes,
  int         num_anodes)
{
  Agraph_t * aparent_graph =
    get_parent_graph (anodes, num_anodes, node);
  if (!aparent_graph)
    aparent_graph = aroot_graph;

  char * node_name = graph_get_node_name (node);
  Agnode_t * anode =
    agnode (aparent_graph, node_name, true);
  switch (node->type)
    {
    case ROUTE_NODE_TYPE_PORT:
      switch (node->port->id.type)
        {
        case TYPE_AUDIO:
          agsafeset (
            anode, (char *) "color",
            (char *) "crimson",
            (char *) "black");
          break;
        case TYPE_EVENT:
          agsafeset (
            anode, (char *) "color",
            (char *) "navy",
            (char *) "black");
          break;
        case TYPE_CONTROL:
          agsafeset (
            anode, (char *) "color",
            (char *) "darkviolet",
            (char *) "black");
          break;
        case TYPE_CV:
          agsafeset (
            anode, (char *) "color",
            (char *) "darkgreen",
            (char *) "black");
          break;
        default:
          break;
        }
      break;
    default:
      agsafeset (
        anode, (char *) "shape", (char *) "record",
        (char *) "ellipse");
      break;
    }
  g_free (node_name);

  return anode;
}

static void
fill_anodes (
  Graph *    graph,
  Agraph_t * aroot_graph,
  ANode *    anodes)
{
  char cluster_name[600];

  /* fill nodes */
  for (int i = 0; i < graph->n_graph_nodes; i++)
    {
      GraphNode * node = graph->graph_nodes[i];
      ANode * anode = &anodes[i];
      anode->node = node;
      get_graph_from_node (
        anodes, graph->n_graph_nodes, node);
    }

  /* create top clusters (tracks, sample processor,
   * monitor fader) */
  for (int i = 0; i < graph->n_graph_nodes; i++)
    {
      GraphNode * node = graph->graph_nodes[i];
      ANode * anode = &anodes[i];

      if (node->type != ROUTE_NODE_TYPE_TRACK &&
          node->type !=
            ROUTE_NODE_TYPE_SAMPLE_PROCESSOR &&
          node->type != ROUTE_NODE_TYPE_MONITOR_FADER)
        continue;

      char * node_name = graph_get_node_name (node);
      sprintf (
        cluster_name, "cluster_%s", node_name);
      anode->graph =
        agsubg (aroot_graph, cluster_name, true);
      agsafeset (
        anode->graph, (char *) "label", node_name,
        (char *) "");
    }

  /* create track subclusters */
  for (int i = 0; i < graph->n_graph_nodes; i++)
    {
      GraphNode * node = graph->graph_nodes[i];
      ANode * anode = &anodes[i];

      if (node->type != ROUTE_NODE_TYPE_PLUGIN &&
          node->type != ROUTE_NODE_TYPE_FADER &&
          node->type != ROUTE_NODE_TYPE_PREFADER)
        continue;

      GraphNode * parent_node;
      switch (node->type)
        {
        case ROUTE_NODE_TYPE_PLUGIN:
          {
            Plugin * pl = node->pl;
            Track * tr =  plugin_get_track (pl);
            parent_node =
              graph_find_node_from_track (
                node->graph, tr);
          }
          break;
        case ROUTE_NODE_TYPE_FADER:
          {
            Fader * fader = node->fader;
            Track * tr =  fader_get_track (fader);
            parent_node =
              graph_find_node_from_track (
                node->graph, tr);
          }
          break;
        case ROUTE_NODE_TYPE_PREFADER:
          {
            PassthroughProcessor * prefader =
              node->prefader;
            Track * tr =
              passthrough_processor_get_track (
                prefader);
            parent_node =
              graph_find_node_from_track (
                node->graph, tr);
          }
          break;
        default:
          continue;
        }

      Agraph_t * aparent_graph =
        get_parent_graph (
          anodes, graph->n_graph_nodes, parent_node);
      g_warn_if_fail (aparent_graph);
      char * node_name =
        graph_get_node_name (node);
      sprintf (
        cluster_name, "cluster_%s", node_name);
      anode->graph =
        agsubg (aparent_graph, cluster_name, true);
      agsafeset (
        anode->graph, (char *) "label", node_name,
        (char *) "");
    }
}

static void
export_as_png (
  Graph *         graph,
  const char *    export_path)
{
  GVC_t * gvc = gvContext();
  Agraph_t * agraph =
    agopen (
      (char *) "routing_graph", Agstrictdirected,
      NULL);

  /* fill anodes with subgraphs */
  ANode * anodes =
    calloc (
      (size_t) graph->n_graph_nodes,
      sizeof (ANode));
  fill_anodes (graph, agraph, anodes);

  /* create graph */
  for (int i = 0; i < graph->n_graph_nodes; i++)
    {
      GraphNode * node = graph->graph_nodes[i];
      Agnode_t * anode =
        create_anode (
          agraph, node, anodes, graph->n_graph_nodes);
      for (int j = 0; j < node->n_childnodes; j++)
        {
          GraphNode * child = node->childnodes[j];
          Agnode_t * achildnode =
            create_anode (
              agraph, child, anodes,
              graph->n_graph_nodes);

          /* create edge */
          Agedge_t * edge =
            agedge (
              agraph, anode, achildnode, NULL, true);
          if (node->type == ROUTE_NODE_TYPE_PORT)
            {
              char * color =
                agget (anode, (char *) "color");
              agsafeset (
                edge, (char *) "color", color,
                (char *) "black");
            }
          else if (child->type ==
                     ROUTE_NODE_TYPE_PORT)
            {
              char * color =
                agget (achildnode, (char *) "color");
              agsafeset (
                edge, (char *) "color",
                (char *) color,
                (char *) "black");
            }
        }
    }

  gvLayout (gvc, agraph, "dot");
  gvRenderFilename (gvc, agraph, "png", export_path);
  gvFreeLayout(gvc, agraph);
  agclose (agraph);
  gvFreeContext(gvc);
}
#endif

/**
 * Exports the graph at the given path.
 */
void
graph_export_as (
  Graph *         graph,
  GraphExportType type,
  const char *    export_path)
{
  switch (type)
    {
#ifdef HAVE_CGRAPH
    case GRAPH_EXPORT_PNG:
      export_as_png (graph, export_path);
      break;
#endif
    default:
      break;
    }
}
