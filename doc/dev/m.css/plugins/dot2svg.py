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

_patch_src = re.compile(r"""<\?xml version="1\.0" encoding="UTF-8" standalone="no"\?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1\.1//EN"
 "http://www\.w3\.org/Graphics/SVG/1\.1/DTD/svg11\.dtd">
<svg width="(?P<width>\d+)pt" height="(?P<height>\d+)pt"
 viewBox="(?P<viewBox>[^"]+)" xmlns="http://www\.w3\.org/2000/svg" xmlns:xlink="http://www\.w3\.org/1999/xlink">
<g id="graph0" class="graph" """)

_patch_dst = r"""<svg{attribs} style="width: {width:.3f}rem; height: {height:.3f}rem;" viewBox="{viewBox}">
<g """
_patch_custom_size_dst = r"""<svg{attribs} style="{size}" viewBox="\g<viewBox>">
<g """

_comment_src = re.compile(r"""<!--[^-]+-->\n""")

# Graphviz < 2.40 (Ubuntu 16.04 and older) doesn't have a linebreak between <g>
# and <title>
_class_src = re.compile(r"""<g id="(edge|node|clust)\d+" class="(?P<type>edge|node|cluster)(?P<classes>[^"]*)">[\n]?<title>(?P<title>[^<]*)</title>
<(?P<element>ellipse|polygon|path|text)( fill="(?P<fill>[^"]+)" stroke="[^"]+")? """)

_class_dst = r"""<g class="{classes}">
<title>{title}</title>
<{element} """

_attributes_src = re.compile(r"""<(?P<element>ellipse|polygon|polyline) fill="[^"]+" stroke="[^"]+" """)

_attributes_dst = r"""<\g<element> """

# re.compile() is called after replacing {font} in configure(). Graphviz < 2.40
# doesn't put the fill="" attribute there
_text_src_src = ' font-family="{font}" font-size="(?P<size>[^"]+)"( fill="[^"]+")?'

_text_dst = ' style="font-size: {size}px;"'

_font = ''
_font_size = 0.0

# The pt are actually px (16pt font is the same size as 16px), so just
# converting to rem here
def _pt2em(pt): return pt/_font_size

def dot2svg(source, size=None, attribs=''):
    try:
        ret = subprocess.run(['dot', '-Tsvg',
            '-Gfontname={}'.format(_font),
            '-Nfontname={}'.format(_font),
            '-Efontname={}'.format(_font),
            '-Gfontsize={}'.format(_font_size),
            '-Nfontsize={}'.format(_font_size),
            '-Efontsize={}'.format(_font_size),
            '-Gbgcolor=transparent',
            ], input=source.encode('utf-8'), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if ret.returncode: print(ret.stderr.decode('utf-8'))
        ret.check_returncode()
    except FileNotFoundError: # pragma: no cover
        raise RuntimeError("dot not found")

    # First remove comments
    svg = _comment_src.sub('', ret.stdout.decode('utf-8'))

    # Remove preamble and fixed size
    if size:
        svg = _patch_src.sub(_patch_custom_size_dst.format(attribs=attribs, size=size), svg)
    else:
        def patch_repl(match): return _patch_dst.format(
            attribs=attribs,
            width=_pt2em(float(match.group('width'))),
            height=_pt2em(float(match.group('height'))),
            viewBox=match.group('viewBox'))
        svg = _patch_src.sub(patch_repl, svg)

    # Remove unnecessary IDs and attributes, replace classes for elements
    def element_repl(match):
        classes = ['m-' + match.group('type')] + match.group('classes').replace('&#45;', '-').split()
        # distinguish between solid and filled nodes
        if ((match.group('type') == 'node' and match.group('fill') == 'none') or
            # a plaintext node is also flat
            match.group('element') == 'text'
        ):
            classes += ['m-flat']

        return _class_dst.format(
            classes=' '.join(classes),
            title=match.group('title'),
            element=match.group('element'))
    svg = _class_src.sub(element_repl, svg)

    # Remove unnecessary fill and stroke attributes
    svg = _attributes_src.sub(_attributes_dst, svg)

    # Remove unnecessary text attributes. Keep font size only if nondefault
    def text_repl(match):
        if float(match.group('size')) != _font_size:
            return _text_dst.format(size=float(match.group('size')))
        return ''
    svg = _text_src.sub(text_repl, svg)

    return svg

def configure(font, font_size):
    global _font, _font_size, _text_src
    _font = font
    _font_size = font_size
    _text_src = re.compile(_text_src_src.format(font=_font))
