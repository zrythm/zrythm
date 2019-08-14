m.dot
#####

Note: the test uses DejaVu Sans instead of Source Sans Pro in order to have
predictable rendering on the CIs.

Different shapes, fills etc. All default colors, filled only the first node
and the arrowheads, nothing else. Non-default font size should be preserved.

.. digraph:: Basics

    rankdir=LR

    a [style=filled shape=rect]
    b [peripheries=2 shape=circle]
    c [shape=ellipse]
    a -> b
    b -> c [label="0" fontsize=40]
    c -> c [label="1"]

.. digraph:: More shapes

    d [shape=plaintext label="outsider"]

Colors:

.. digraph:: Colors

    a [class="m-success"]
    b [style=filled shape=circle class="m-dim"]
    a -> b [class="m-warning" label="yes"]
    b -> b [class="m-primary" label="no"]

Unoriented graph without a title:

.. graph::
    :class: m-success

    a -- b
    a -- b

Strict graphs:

.. strict-digraph:: A to B

    a -> b
    a -> b

.. strict-graph:: A to B

    a -- b
    a -- b

Structs:

.. digraph:: Structs

    struct [label="{ a | b | { c | d | e }}" shape=record class="m-info"]

    another [label="a | { b | c } | d | e" shape=record]

.. graph-figure:: This is a title.

    .. digraph:: A to B
        :class: m-info

        a -> b

    This is a description.

.. graph-figure::

    .. digraph::

        a -> b

    The graph below should not be styled as a part of the figure:

    .. digraph:: A to B
        :class: m-danger

        a -> b
