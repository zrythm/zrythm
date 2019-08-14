/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * The Processing Graph is an acyclic graph of
 * nodes, which are explained below.
 * The graph is re-created
 * every time there is a change in Port connections.
 * When the graph gets (re)created, it goes through
 * the project and finds all nodes that should be
 * added to it and adds them, then creates
 * NUM_CPU_CORES - 1 threads for processing
 * trigger nodes.
 *
 * \section ports Ports
 *
 * Track's (Channel's) and Plugin's are made up of
 * Port's. These ports can be connected with each
 * other, as long as they are the same PortType (
 * with a few exceptions) and of opposite PortFlow.
 *
 * When these ports are connected, during
 * processing, the signals from the source port are
 * summed into the destination port. What this
 * means in practice depends on the PortType. For
 * audio, the signal is simply added to the
 * existing one. For event/MIDI ports, the
 * MidiEvent's from the source port are appended
 * to the events of the destination port.
 *
 * \section nodes Nodes
 *
 * \subsection trigger_nodes Trigger Nodes
 *
 */

