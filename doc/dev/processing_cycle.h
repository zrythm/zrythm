// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \page processing_cycle The Processing Cycle
 *
 * \section introduction_processing_cycle Introduction
 *
 * Thanks to Robin Gareus' advice and help, Zrythm
 * uses a processing algorithm that depends on an
 * acyclic graph of ports. This algorithm splits
 * processing jobs into threads and processes
 * each node until the terminal nodes are reached
 * and there are no more "trigger nodes". The
 * algorithm is explained in detail below.
 *
 * \section graph The Graph
 *
 * The processing Graph is an acyclic graph of
 * GraphNode's.
 * The graph is re-created
 * every time there is a change in connections.
 * When the graph gets (re)created, it goes through
 * the project and finds all nodes that should be
 * added to it and adds them, then creates
 * NUM_CPU_CORES - 1 threads for processing
 * trigger nodes (see below).
 *
 * \section nodes Nodes
 *
 * GraphNode's can be either \ref processors or
 * \ref ports.
 *
 * \subsection ports Ports
 *
 * Track's (Channel's) and Plugin's are made up of
 * Port's. These ports can be connected with each
 * other, as long as they are the same \ref PortType
 * (with a few exceptions) and of opposite
 * \ref PortFlow.
 *
 * When these ports are connected, during
 * processing, the signals from the source port are
 * summed into the destination port. What this
 * means in practice depends on the \ref PortType.
 * For
 * audio, the signal is simply added to the
 * existing one. For event/MIDI ports, the
 * MidiEvent's from the source port are appended
 * to the events of the destination port.
 *
 * \dot
 *  digraph port_connections {
 *    a [ label="out port A" ];
 *    b [ label="out port B" ];
 *    c [ label="in port C" ];
 *    a -> c [ label="signal" ];
 *    b -> c [ label="signal" ];
 *  }
 * \enddot
 *
 * \subsection processors Processors
 *
 * Processors are the other type of node in the
 * graph and have input ports routing to them and
 * output ports being routed from them. Unlike Port
 * nodes, which simply sum their inputs, processors
 * apply some processing to their inputs and update
 * their outputs. Examples are TrackProcessor, Fader
 * and Plugin.
 *
 * \dot
 *  digraph processor {
 *    a [ label="in port 1" ];
 *    b [ label="in port 2" ];
 *    c [ label="processor", shape=record ];
 *    d [ label="out port 1" ];
 *    e [ label="out port 2" ];
 *    a -> c;
 *    b -> c;
 *    c -> d;
 *    c -> e;
 *  }
 * \enddot
 *
 * Input ports routed to processors are processed
 * in the same way as any other Port, in that they
 * simply sum their inputs. When the processor does its
 * processing, its input ports are already filled
 * in and ready to use.
 *
 * When the processor finishes processing, it writes
 * the output into its output ports. A processor
 * routes to output Port's in the graph.
 *
 * @note As a technical sidenote, processor input
 * ports are indeed routed to
 * the processor *on the graph*, however those input Port's
 * have no outgoing connection to other
 * Port's. Similarly, processor output ports are
 * connected to the processor *on the graph*,
 * however
 * the Port's themselves have no incoming connection
 * from other Port's (so they have no signals to
 * sum when it is their turn to be processed - at
 * that stage, they are already filled in by their
 * processors).
 *
 * \subsection trigger_nodes Trigger and Terminal Nodes
 *
 * Terminal nodes are nodes that don't route to
 * any other node.
 *
 * Trigger nodes are nodes that should be processed
 * first. When they are processed, they are marked as
 * processed and the reference count of the nodes
 * they point to is decremented. When the reference
 * count of a node becomes 0 (meaning it has
 * no incoming connections), it becomes a trigger
 * node. When the graph is constructed, it must
 * have at least 1 trigger node.
 *
 * \dot
 *  digraph trigger_nodes {
 *    a [ label="port A (refcount: 0 - trigger)" ];
 *    b [ label="port B (refcount: 0 - trigger)" ];
 *    c [ label="port C (refcount: 2)" ];
 *    d [ label="port D (refcount: 1 - terminal)" ];
 *    a -> c;
 *    b -> c;
 *    c -> d;
 *  }
 * \enddot
 *
 * When there are no more trigger nodes left, the
 * processing cycle is completed.
 *
 * \section latency_compensation Latency Compensation
 *
 */
