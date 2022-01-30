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

m.css math
##########

.. role:: em-strong(strong)
    :class: m-text m-em
.. role:: text-primary
    :class: m-text m-primary

.. |o| replace:: ·

:url: admire/math/
:cover: {static}/static/cover-math.jpg
:summary: The fastest possible math rendering for the modern web
:footer:
    .. note-dim::

        *Wondering what m.css is all about?* Visit the `main page <{filename}/index.rst>`_
        to see what else it can offer to you.
:landing:
    .. container:: m-row

        .. container:: m-col-l-6 m-push-l-1 m-col-m-7 m-nopadb

            .. raw:: html

                <h1><span class="m-thin">m.css</span> math</h1>

    .. container:: m-row

        .. container:: m-col-l-6 m-push-l-1 m-col-m-7 m-nopadt

            *The* :em-strong:`fastest possible` *math rendering for the modern web.*

            With latest advancements in web tech, the browser should no longer
            *struggle* to render math. Yet it still does. Why *jump around*
            several times until the math settles down? Why *stutter* for
            *seconds* in a heavy JavaScript renderer from a third-party CDN
            that gets shut down every once a while? Why *saturate* the network
            with extra requests for every small snippet? You don't need
            *any of that*.

        .. container:: m-col-l-3 m-push-l-2 m-col-m-4 m-push-m-1 m-col-s-6 m-push-s-3 m-col-t-8 m-push-t-2

            .. button-primary:: #demo
                :class: m-fullwidth

                See the demo

                | don't worry,
                | it's already loaded

    .. container:: m-row

        .. container:: m-col-m-12

            .. a copy of the following is below

            .. math::

                {\color{m-primary} L_o (\boldsymbol{x}, \omega_o)} = {\color{m-danger}\int\limits_{\Omega}} {\color{m-warning} f_r(\boldsymbol{x},\omega_i,\omega_o)} {\color{m-success} L_i(\boldsymbol{x},\omega_i)} {\color{m-info} ( \omega_i \cdot \boldsymbol{n})} {\color{m-danger} \operatorname d \omega_i}

.. container:: m-row m-container-inflate

    .. container:: m-col-m-4

        .. block-warning:: *Self-contained* inline SVG

            Embed math directly in HTML5 markup as pure paths, cutting away all
            external dependencies or extra fonts. And having the SVG inline
            means you can make use of all the CSS goodies on it.

            .. button-warning:: {filename}/css/components.rst#math
                :class: m-fullwidth

                See the possibilities

    .. container:: m-col-m-4

        .. block-success:: *Fastest possible* rendering

            As you unleash the full power of a static site generator to convert
            LaTeX formulas to vector graphics locally, your viewers are saved
            from needless HTTP requests and heavy JavaScript processing.

            .. button-success:: {filename}/themes/pelican.rst
                :class: m-fullwidth

                Start using Pelican

    .. container:: m-col-m-4

        .. block-info:: You are *completely* in charge

            Since the math is rendered by your local TeX installation, you have
            the full control over its appearance and you can be sure that it
            will also render the same on all browsers. Now or two years later.

            .. button-info:: {filename}/plugins/math-and-code.rst#math
                :class: m-fullwidth

                Get the plugin

.. _demo:

Demo time!
==========

Be sure to refresh your browser a couple of times to see how the rendering
performs and that there is no blank space or jumping until the math appears.
View the page source to verify that there is nothing extra being loaded to make
this happen.

Try to resize the window --- everything is possible, so why not have a
different layout of long equations optimized for smaller screens?

    .. math::
        :class: m-show-m

        \pi = \cfrac{4} {1+\cfrac{1^2} {2+\cfrac{3^2} {2+\cfrac{5^2} {2+\ddots}}}}
            = \sum_{n=0}^\infty \frac{4(-1)^n}{2n+1}
            = \frac{4}{1} - \frac{4}{3} + \frac{4}{5} - \frac{4}{7} +- \cdots

    .. math::
        :class: m-hide-m

        \begin{array}{rcl}
        \pi &=& \cfrac{4} {1+\cfrac{1^2} {2+\cfrac{3^2} {2+\cfrac{5^2} {2+\ddots}}}}
             =  \sum_{n=0}^\infty \frac{4(-1)^n}{2n+1} \\
            &=& \frac{4}{1} - \frac{4}{3} + \frac{4}{5} - \frac{4}{7} +- \cdots
        \end{array}

    .. class:: m-text m-text-right m-dim m-em

    --- `Generalized continued fraction <https://en.wikipedia.org/wiki/Generalized_continued_fraction#.CF.80>`_,
    Wikipedia

Matrices render pretty well also:

    .. math::

        R = \begin{pmatrix}
        \langle\mathbf{e}_1,\mathbf{a}_1\rangle & \langle\mathbf{e}_1,\mathbf{a}_2\rangle &  \langle\mathbf{e}_1,\mathbf{a}_3\rangle  & \ldots \\
        0                & \langle\mathbf{e}_2,\mathbf{a}_2\rangle                        &  \langle\mathbf{e}_2,\mathbf{a}_3\rangle  & \ldots \\
        0                & 0                                       & \langle\mathbf{e}_3,\mathbf{a}_3\rangle                          & \ldots \\
        \vdots           & \vdots                                  & \vdots                                    & \ddots \end{pmatrix}.

    .. class:: m-text m-text-right m-dim m-em

    --- `QR decomposition <https://en.wikipedia.org/wiki/QR_decomposition>`_,
    Wikipedia

Now, some inline math --- note the vertical alignment, consistent line spacing
and that nothing gets relayouted during page load:

    Multiplying :math:`x_n` by a *linear phase* :math:`e^{\frac{2\pi i}{N}n m}`
    for some integer :math:`m` corresponds to a *circular shift* of the output
    :math:`X_k`: :math:`X_k` is replaced by :math:`X_{k-m}`, where the
    subscript is interpreted `modulo <https://en.wikipedia.org/wiki/Modular_arithmetic>`_
    :math:`N` (i.e., periodically).  Similarly, a circular shift of the input
    :math:`x_n` corresponds to multiplying the output :math:`X_k` by a linear
    phase. Mathematically, if :math:`\{x_n\}` represents the vector
    :math:`\boldsymbol{x}` then

    if :math:`\mathcal{F}(\{x_n\})_k=X_k`

    then :math:`\mathcal{F}(\{ x_n\cdot e^{\frac{2\pi i}{N}n m} \})_k=X_{k-m}`

    and :math:`\mathcal{F}(\{x_{n-m}\})_k=X_k\cdot e^{-\frac{2\pi i}{N}k m}`

    .. class:: m-text m-text-right m-dim m-em

    ---  `Discrete Fourier transform § Shift theorem <https://en.wikipedia.org/wiki/Discrete_Fourier_transform#Shift_theorem>`_, Wikipedia

The inline SVG follows surrounding text size, so you can use it easily in more
places than just the main copy:

    .. button-default:: https://tauday.com/

        The :math:`\tau` manifesto

        they say :math:`\pi` is wrong

:text-primary:`Everything can be colored` just by putting CSS classes around:

    .. math::
        :class: m-primary m-show-m

        X_{k+N} \ \stackrel{\mathrm{def}}{=} \ \sum_{n=0}^{N-1} x_n e^{-\frac{2\pi i}{N} (k+N) n} = \sum_{n=0}^{N-1} x_n e^{-\frac{2\pi i}{N} k n}  \underbrace{e^{-2 \pi i n}}_{1} = \sum_{n=0}^{N-1} x_n e^{-\frac{2\pi i}{N} k n} = X_k.

    .. math::
        :class: m-primary m-hide-m

        \begin{array}{rcl}
            X_{k+N} & \ \stackrel{\mathrm{def}}{=} \ & \sum_{n=0}^{N-1} x_n e^{-\frac{2\pi i}{N} (k+N) n} \\
             & = & \sum_{n=0}^{N-1} x_n e^{-\frac{2\pi i}{N} k n}  \underbrace{e^{-2 \pi i n}}_{1} \\
             & = & \sum_{n=0}^{N-1} x_n e^{-\frac{2\pi i}{N} k n} = X_k.
        \end{array}

    .. class:: m-text m-text-right m-dim m-em

    --- `Discrete Fourier transform § Periodicity <https://en.wikipedia.org/wiki/Discrete_Fourier_transform#Periodicity>`_, Wikipedia

But it's also possible to color only parts of the equation --- with a color
that matches page theme.

    .. math::
        :class: m-show-s

        {\color{m-primary} L_o (\boldsymbol{x}, \omega_o)} = {\color{m-danger}\int\limits_{\Omega}} {\color{m-warning} f_r(\boldsymbol{x},\omega_i,\omega_o)} {\color{m-success} L_i(\boldsymbol{x},\omega_i)} {\color{m-info} ( \omega_i \cdot \boldsymbol{n})} {\color{m-danger} \operatorname d \omega_i}

    .. math::
        :class: m-hide-s m-text m-small

        {\color{m-primary} L_o (\boldsymbol{x}, \omega_o)} = {\color{m-danger}\int\limits_{\Omega}} {\color{m-warning} f_r(\boldsymbol{x},\omega_i,\omega_o)} {\color{m-success} L_i(\boldsymbol{x},\omega_i)} {\color{m-info} ( \omega_i \cdot \boldsymbol{n})} {\color{m-danger} \operatorname d \omega_i}

    .. class:: m-text-center m-noindent

        :label-primary:`outgoing light` |o| :label-danger:`integral`
        |o| :label-warning:`BRDF` |o| :label-success:`incoming light`
        |o| :label-info:`normal attenuation`

    .. class:: m-text m-text-right m-dim m-em

    --- `Lighting: The Rendering Equation <http://www.rorydriscoll.com/2008/08/24/lighting-the-rendering-equation/>`_, rorydriscoll.com

.. combined with https://en.wikipedia.org/wiki/Rendering_equation for the nice
    Greek letters
