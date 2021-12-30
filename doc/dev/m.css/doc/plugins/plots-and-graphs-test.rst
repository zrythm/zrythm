..
    This file is part of m.css.

    Copyright © 2017, 2018, 2019, 2020 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
..

Test
####

:save_as: plugins/plots-and-graphs/test/index.html
:breadcrumb: {filename}/plugins.rst Plugins
             {filename}/plugins/plots-and-graphs.rst Plots and graphs

Plots
=====

.. container:: m-row

    .. container:: m-col-m-6

        .. plot:: Fastest animals
            :type: barh
            :labels:
                Cheetah
                Pronghorn
            :units: km/h
            :values: 109.4 88.5
            :colors: warning primary
            :errors: 14.32 5.5

    .. container:: m-col-m-6

        .. plot:: Fastest animals
            :type: barh
            :labels:
                Cheetah
                Pronghorn
                Springbok
                Wildebeest
            :units: km/h
            :values: 109.4 88.5 88 80.5
            :colors: warning primary danger info

Graphs
======

.. container:: m-row m-container-inflate

    .. container:: m-col-m-4

        All should be colored except the self-pointing "no" and the "?" arrow.

    .. container:: m-col-m-4

        All should be colored globally with the same color.

    .. container:: m-col-m-4

        All should be colored globally with overrides except for the
        self-pointing "no" and the "?" arrow.

.. container:: m-row m-container-inflate

    .. container:: m-col-m-4

        .. digraph:: FSM

            rankdir=LR
            yes [shape=circle, class="m-default", style=filled]
            no [shape=circle, class="m-default"]
            yes -> no [label="no", class="m-default"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"
                class="m-default"

                struct [label="{ ? | { ?! | !? }}", shape=record, class="m-default"]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM
            :class: m-default

            rankdir=LR
            yes [shape=circle, style=filled]
            no [shape=circle]
            yes -> no [label="no"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"

                struct [label="{ ? | { ?! | !? }}", shape=record]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM
            :class: m-danger

            rankdir=LR
            yes [shape=circle, class="m-default", style=filled]
            no [shape=circle, class="m-default"]
            yes -> no [label="no", class="m-default"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"
                class="m-default"

                struct [label="{ ? | { ?! | !? }}", shape=record, class="m-default"]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM

            rankdir=LR
            yes [shape=circle, class="m-primary", style=filled]
            no [shape=circle, class="m-primary"]
            yes -> no [label="no", class="m-primary"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"
                class="m-primary"

                struct [label="{ ? | { ?! | !? }}", shape=record, class="m-primary"]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM
            :class: m-primary

            rankdir=LR
            yes [shape=circle, style=filled]
            no [shape=circle]
            yes -> no [label="no"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"

                struct [label="{ ? | { ?! | !? }}", shape=record]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM
            :class: m-danger

            rankdir=LR
            yes [shape=circle, class="m-primary", style=filled]
            no [shape=circle, class="m-primary"]
            yes -> no [label="no", class="m-primary"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"
                class="m-primary"

                struct [label="{ ? | { ?! | !? }}", shape=record, class="m-primary"]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM

            rankdir=LR
            yes [shape=circle, class="m-success", style=filled]
            no [shape=circle, class="m-success"]
            yes -> no [label="no", class="m-success"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"
                class="m-success"

                struct [label="{ ? | { ?! | !? }}", shape=record, class="m-success"]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM
            :class: m-success

            rankdir=LR
            yes [shape=circle, style=filled]
            no [shape=circle]
            yes -> no [label="no"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"

                struct [label="{ ? | { ?! | !? }}", shape=record]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM
            :class: m-danger

            rankdir=LR
            yes [shape=circle, class="m-success", style=filled]
            no [shape=circle, class="m-success"]
            yes -> no [label="no", class="m-success"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"
                class="m-success"

                struct [label="{ ? | { ?! | !? }}", shape=record, class="m-success"]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM

            rankdir=LR
            yes [shape=circle, class="m-warning", style=filled]
            no [shape=circle, class="m-warning"]
            yes -> no [label="no", class="m-warning"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"
                class="m-warning"

                struct [label="{ ? | { ?! | !? }}", shape=record, class="m-warning"]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM
            :class: m-warning

            rankdir=LR
            yes [shape=circle, style=filled]
            no [shape=circle]
            yes -> no [label="no"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"

                struct [label="{ ? | { ?! | !? }}", shape=record]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM
            :class: m-danger

            rankdir=LR
            yes [shape=circle, class="m-warning", style=filled]
            no [shape=circle, class="m-warning"]
            yes -> no [label="no", class="m-warning"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"
                class="m-warning"

                struct [label="{ ? | { ?! | !? }}", shape=record, class="m-warning"]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM

            rankdir=LR
            yes [shape=circle, class="m-danger", style=filled]
            no [shape=circle, class="m-danger"]
            yes -> no [label="no", class="m-danger"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"
                class="m-danger"

                struct [label="{ ? | { ?! | !? }}", shape=record, class="m-danger"]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM
            :class: m-danger

            rankdir=LR
            yes [shape=circle, style=filled]
            no [shape=circle]
            yes -> no [label="no"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"

                struct [label="{ ? | { ?! | !? }}", shape=record]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM
            :class: m-success

            rankdir=LR
            yes [shape=circle, class="m-danger", style=filled]
            no [shape=circle, class="m-danger"]
            yes -> no [label="no", class="m-danger"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"
                class="m-danger"

                struct [label="{ ? | { ?! | !? }}", shape=record, class="m-danger"]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM

            rankdir=LR
            yes [shape=circle, class="m-info", style=filled]
            no [shape=circle, class="m-info"]
            yes -> no [label="no", class="m-info"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"
                class="m-info"

                struct [label="{ ? | { ?! | !? }}", shape=record, class="m-info"]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM
            :class: m-info

            rankdir=LR
            yes [shape=circle, style=filled]
            no [shape=circle]
            yes -> no [label="no"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"

                struct [label="{ ? | { ?! | !? }}", shape=record]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM
            :class: m-danger

            rankdir=LR
            yes [shape=circle, class="m-info", style=filled]
            no [shape=circle, class="m-info"]
            yes -> no [label="no", class="m-info"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"
                class="m-info"

                struct [label="{ ? | { ?! | !? }}", shape=record, class="m-info"]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM

            rankdir=LR
            yes [shape=circle, class="m-dim", style=filled]
            no [shape=circle, class="m-dim"]
            yes -> no [label="no", class="m-dim"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"
                class="m-dim"

                struct [label="{ ? | { ?! | !? }}", shape=record, class="m-dim"]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM
            :class: m-dim

            rankdir=LR
            yes [shape=circle, style=filled]
            no [shape=circle]
            yes -> no [label="no"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"

                struct [label="{ ? | { ?! | !? }}", shape=record]
                struct -> yes [label="?"]
            }

    .. container:: m-col-m-4

        .. digraph:: FSM
            :class: m-danger

            rankdir=LR
            yes [shape=circle, class="m-dim", style=filled]
            no [shape=circle, class="m-dim"]
            yes -> no [label="no", class="m-dim"]
            no -> no [label="no"]

            subgraph cluster_A {
                label="Maybe"
                class="m-dim"

                struct [label="{ ? | { ?! | !? }}", shape=record, class="m-dim"]
                struct -> yes [label="?"]
            }
