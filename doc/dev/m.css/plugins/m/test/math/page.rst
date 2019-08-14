m.math
######

:summary: no.

.. role:: math-primary(math)
    :class: m-primary

Inline colored math :math-primary:`a^2` and colored math block:

.. math::
    :class: m-success

    a^2 + b^2 = c^2

Colored parts of inline :math:`b^2 - \color{m-info}{4ac}` and block formulas:

.. math::

    \frac{-b \pm \color{m-success} \sqrt{D}}{2a}

Properly align *huge* formulas vertically on a line:
:math:`\hat q^{-1} = \frac{\hat q^*}{|\hat q|^2}`
and make sure there's enough space for all the complex :math:`W` things between
the lines :math:`W = \sum_{i=0}^{n} \frac{w_i}{h_i}` because
:math:`Y = \sum_{i=0}^{n} B`

The ``\cfrac`` thing doesn't align well: :math:`W = \sum_{i=0}^{n} \cfrac{w_i}{h_i}`

Properly escape the formula source:

.. math::

    \begin{array}{rcl}
        x & = & 1
    \end{array}

.. class:: m-text m-big

Formulas :math:`a^2` in big text are big.

.. class:: m-text m-small

Formulas :math:`a^2` in small text are small.

.. math-figure:: This is a title.

    .. math::
        :class: m-info

        a^2 + b^2 = c^2

    This is a description.

.. math-figure::

    .. math::

        a^2 + b^2 = c^2

    The math below should not be styled as a part of the figure:

    .. math::
        :class: m-danger

        a^2 + b^2 = c^2
