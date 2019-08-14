#
#   This file is part of m.css.
#
#   Copyright © 2017, 2018, 2019 Vladimír Vondruš <mosra@centrum.cz>
#
#   Permission is hereby granted, free of charge, to any person obtaining a
#   copy of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom the
#   Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.
#

import html
import os
import re

from docutils import nodes, utils
from docutils.parsers import rst
from docutils.parsers.rst import directives
from docutils.parsers.rst.roles import set_classes

import pelican.signals

import latex2svg
import latex2svgextra

render_as_code = False

def _is_math_figure(parent):
    # The parent has to be a figure, marked as m-figure
    if not isinstance(parent, nodes.figure): return False
    if 'm-figure' not in parent.get('classes', []): return False

    # And as a first visible node of such type
    for child in parent:
        if not isinstance(child, nodes.Invisible): return False

    return True

class Math(rst.Directive):
    option_spec = {'class': directives.class_option,
                   'name': directives.unchanged}
    has_content = True

    def run(self):
        set_classes(self.options)
        self.assert_has_content()

        parent = self.state.parent

        # Fallback rendering as code requested
        if render_as_code:
            # If this is a math figure, replace the figure CSS class to have a
            # matching border
            if _is_math_figure(parent):
                parent['classes'][parent['classes'].index('m-figure')] = 'm-code-figure'

            content = nodes.raw('', html.escape('\n'.join(self.content)), format='html')
            pre = nodes.literal_block('')
            pre.append(content)
            return [pre]

        content = '\n'.join(self.content)

        _, svg = latex2svgextra.fetch_cached_or_render("$$" + content + "$$")

        # If this is the first real node inside a math figure, put the SVG
        # directly inside
        if _is_math_figure(parent):
            node = nodes.raw(self.block_text, latex2svgextra.patch(content, svg, None, ' class="{}"'.format(' '.join(['m-math'] + self.options.get('classes', [])))), format='html')
            node.line = self.content_offset + 1
            self.add_name(node)
            return [node]

        # Otherwise wrap it in a <div class="m-math">
        node = nodes.raw(self.block_text, latex2svgextra.patch(content, svg, None, ''), format='html')
        node.line = self.content_offset + 1
        self.add_name(node)
        container = nodes.container(**self.options)
        container['classes'] += ['m-math']
        container.append(node)
        return [container]

def new_page(content):
    latex2svgextra.counter = 0

def math(role, rawtext, text, lineno, inliner, options={}, content=[]):
    # Otherwise the backslashes do quite a mess there
    i = rawtext.find('`')
    text = rawtext.split('`')[1]

    # Fallback rendering as code requested
    if render_as_code:
        set_classes(options)
        classes = []
        if 'classes' in options:
            classes += options['classes']
            del options['classes']

        content = nodes.raw('', html.escape(utils.unescape(text)), format='html')
        node = nodes.literal(rawtext, '', **options)
        node.append(content)
        return [node], []

    # Apply classes to the <svg> element instead of some outer <span>
    set_classes(options)
    classes = 'm-math'
    if 'classes' in options:
        classes += ' ' + ' '.join(options['classes'])
        del options['classes']

    depth, svg = latex2svgextra.fetch_cached_or_render("$" + text + "$")

    attribs = ' class="{}"'.format(classes)
    node = nodes.raw(rawtext, latex2svgextra.patch(text, svg, depth, attribs), format='html', **options)
    return [node], []

def configure_pelican(pelicanobj):
    global render_as_code
    render_as_code = pelicanobj.settings.get('M_MATH_RENDER_AS_CODE', False)
    cache_file = pelicanobj.settings.get('M_MATH_CACHE_FILE', 'm.math.cache')
    if cache_file and os.path.exists(cache_file):
        latex2svgextra.unpickle_cache(cache_file)
    else:
        latex2svgextra.unpickle_cache(None)

def save_cache(pelicanobj):
    cache_file = pelicanobj.settings.get('M_MATH_CACHE_FILE', 'm.math.cache')
    if cache_file: latex2svgextra.pickle_cache(cache_file)

def register():
    pelican.signals.initialized.connect(configure_pelican)
    pelican.signals.finalized.connect(save_cache)
    pelican.signals.content_object_init.connect(new_page)
    rst.directives.register_directive('math', Math)
    rst.roles.register_canonical_role('math', math)
