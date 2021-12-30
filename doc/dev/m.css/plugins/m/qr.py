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

import io
import qrcode
import qrcode.image.svg
import re

from docutils.parsers import rst
from docutils.parsers.rst import directives
from docutils.parsers.rst.roles import set_classes
from docutils import nodes

_svg_preamble_src = re.compile(r"""<\?xml version='1.0' encoding='UTF-8'\?>
<svg height="(?P<height>\d+)mm" version="1.1" viewBox="(?P<viewBox>0 0 \d+ \d+)" width="(?P<width>\d+)mm" xmlns="http://www.w3.org/2000/svg">""")
_svg_preamble_dst = '<svg{attribs} style="width: {size}; height: {size};" viewBox="{viewBox}">'

def _mm2rem(mm): return mm*9.6/2.54/16.0

class Qr(rst.Directive):
    final_argument_whitespace = True
    has_content = False
    required_arguments = 1
    option_spec = {'class': directives.class_option,
                   'name': directives.unchanged,
                   'size': directives.length_or_unitless}

    def run(self):
        set_classes(self.options)

        # FFS why so complex
        svg = qrcode.make(self.arguments[0], image_factory=qrcode.image.svg.SvgPathFillImage)
        f = io.BytesIO()
        svg.save(f)
        svg = f.getvalue().decode('utf-8')

        # Compress a bit, remove cruft
        svg = svg.replace('L ', 'L').replace('M ', 'M').replace(' z', 'z')
        svg = svg.replace(' id="qr-path"', '')

        attribs = ' class="{}"'.format(' '.join(['m-image'] + self.options.get('classes', [])))

        if 'size' in self.options:
            size = self.options['size']
        else:
            size = None

        def preamble_repl(match): return _svg_preamble_dst.format(
            attribs=attribs,
            size=size if size else
                # The original size is in mm, convert that to pixels on 96 DPI
                # and then to rem assuming 1 rem = 16px
                '{:.2f}rem'.format(float(match.group('width'))*9.6/2.54/16.0, 2),
            viewBox=match.group('viewBox'))
        svg = _svg_preamble_src.sub(preamble_repl, svg)
        # There's a pointless difference between what qrcode 6.1 generates on
        # my Python 3.7 (w/o space) and on Travis Python 3.7. I don't know
        # what's to blame, so just patch that.
        svg = svg.replace(' />', '/>')
        return [nodes.raw('', svg, format='html')]

def register_mcss(**kwargs):
    rst.directives.register_directive('qr', Qr)

# Below is only Pelican-specific functionality. If Pelican is not found, these
# do nothing.

register = register_mcss # for Pelican
