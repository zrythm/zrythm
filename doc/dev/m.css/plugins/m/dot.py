#
#   This file is part of m.css.
#
#   Copyright © 2017, 2018, 2019, 2020 Vladimír Vondruš <mosra@centrum.cz>
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

import re
import subprocess

from docutils import nodes
from docutils.parsers import rst
from docutils.parsers.rst import directives
from docutils.parsers.rst.roles import set_classes

import dot2svg

def _is_graph_figure(parent):
    # The parent has to be a figure, marked as m-figure
    if not isinstance(parent, nodes.figure): return False
    if 'm-figure' not in parent.get('classes', []): return False

    # And as a first visible node of such type
    for child in parent:
        if not isinstance(child, nodes.Invisible): return False

    return True

class Dot(rst.Directive):
    has_content = True
    optional_arguments = 1
    final_argument_whitespace = True
    option_spec = {'class': directives.class_option,
                   'name': directives.unchanged}

    def run(self, source):
        set_classes(self.options)

        # If this is the first real node inside a graph figure, put the SVG
        # directly inside
        parent = self.state.parent
        if _is_graph_figure(parent):
            svg = dot2svg.dot2svg(source, attribs=' class="{}"'.format(' '.join(['m-graph'] + self.options.get('classes', []))))
            node = nodes.raw('', svg, format='html')
            return [node]

        # Otherwise wrap it in a <div class="m-graph">
        svg = dot2svg.dot2svg(source)
        container = nodes.container(**self.options)
        container['classes'] = ['m-graph'] + container['classes']
        node = nodes.raw('', svg, format='html')
        container.append(node)
        return [container]

class Digraph(Dot):
    def run(self):
        # We need to pass "" for an empty title to get rid of <title>,
        # otherwise the output contains <title>%3</title> (wtf!)
        return Dot.run(self, 'digraph "{}" {{\n{}}}'.format(
            self.arguments[0] if self.arguments else '',
            '\n'.join(self.content)))

class StrictDigraph(Dot):
    def run(self):
        # We need to pass "" for an empty title to get rid of <title>,
        # otherwise the output contains <title>%3</title> (wtf!)
        return Dot.run(self, 'strict digraph "{}" {{\n{}}}'.format(
            self.arguments[0] if self.arguments else '',
            '\n'.join(self.content)))

class Graph(Dot):
    def run(self):
        # We need to pass "" for an empty title to get rid of <title>,
        # otherwise the output contains <title>%3</title> (wtf!)
        return Dot.run(self, 'graph "{}" {{\n{}}}'.format(
            self.arguments[0] if self.arguments else '',
            '\n'.join(self.content)))

class StrictGraph(Dot):
    def run(self):
        # We need to pass "" for an empty title to get rid of <title>,
        # otherwise the output contains <title>%3</title> (wtf!)
        return Dot.run(self, 'strict graph "{}" {{\n{}}}'.format(
            self.arguments[0] if self.arguments else '',
            '\n'.join(self.content)))

def register_mcss(mcss_settings, **kwargs):
    dot2svg.configure(
        mcss_settings.get('M_DOT_FONT', 'Source Sans Pro'),
        mcss_settings.get('M_DOT_FONT_SIZE', 16.0))
    rst.directives.register_directive('digraph', Digraph)
    rst.directives.register_directive('strict-digraph', StrictDigraph)
    rst.directives.register_directive('graph', Graph)
    rst.directives.register_directive('strict-graph', StrictGraph)

# Below is only Pelican-specific functionality. If Pelican is not found, these
# do nothing.

def _pelican_configure(pelicanobj):
    register_mcss(mcss_settings=pelicanobj.settings)

def register(): # for Pelican
    from pelican import signals

    signals.initialized.connect(_pelican_configure)
