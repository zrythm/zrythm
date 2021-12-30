#
#   This file is part of m.css.
#
#   Copyright © 2017, 2018, 2019, 2020 Vladimír Vondruš <mosra@centrum.cz>
#   Copyright © 2020 Blair Conrad <blair@blairconrad.com>
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

from pygments.lexer import RegexLexer
from pygments.formatters import HtmlFormatter
from pygments.token import *

# Support ANSI SGR codes in input, styling the output.
# Code definitions are taken from
# http://man7.org/linux/man-pages/man4/console_codes.4.html, which
# appears in part below, in case it disappears:
#
# ECMA-48 Set Graphics Rendition
#
#    The ECMA-48 SGR sequence ESC [ parameters m sets display attributes.
#    Several attributes can be set in the same sequence, separated by
#    semicolons.  An empty parameter (between semicolons or string
#    initiator or terminator) is interpreted as a zero.
#   param     result
#    0         reset all attributes to their defaults
#    1         set bold
#    2         set half-bright (simulated with color on a color display)
#    4         set underscore (simulated with color on a color display)
#              (the colors used to simulate dim or underline are set
#              using ESC ] ...)
#    5         set blink
#    7         set reverse video
#    10        reset selected mapping, display control flag, and toggle
#              meta flag (ECMA-48 says "primary font").
#    11        select null mapping, set display control flag, reset
#              toggle meta flag (ECMA-48 says "first alternate font").
#    12        select null mapping, set display control flag, set toggle
#              meta flag (ECMA-48 says "second alternate font").  The
#              toggle meta flag causes the high bit of a byte to be
#              toggled before the mapping table translation is done.
#    21        set underline; before Linux 4.17, this value set normal
#              intensity (as is done in many other terminals)
#    22        set normal intensity
#    24        underline off
#    25        blink off
#    27        reverse video off
#    30        set black foreground
#    31        set red foreground
#    32        set green foreground
#    33        set brown foreground
#    34        set blue foreground
#    35        set magenta foreground
#    36        set cyan foreground
#    37        set white foreground
#    38        256/24-bit foreground color follows, shoehorned into 16
#              basic colors (before Linux 3.16: set underscore on, set
#              default foreground color)
#    39        set default foreground color (before Linux 3.16: set
#              underscore off, set default foreground color)
#    40        set black background
#    41        set red background
#    42        set green background
#    43        set brown background
#    44        set blue background
#    45        set magenta background
#    46        set cyan background
#    47        set white background
#    48        256/24-bit background color follows, shoehorned into 8
#              basic colors
#    49        set default background color
#    90..97    set foreground to bright versions of 30..37
#    100..107  set background to bright versions of 40..47
#
#    Commands 38 and 48 require further arguments:
#      ;5;x       256 color: values 0..15 are IBGR (black, red, green,
#                 ... white), 16..231 a 6x6x6 color cube, 232..255 a
#                 grayscale ramp
#      ;2;r;g;b   24-bit color, r/g/b components are in the range 0..255
#
# For historical reasons, all "brown"s above are replaced with "yellow"
# by m.css.
#
# AnsiLexer supports commands 0, 1, 22, 30–39, 40–49, 90–97, and 100–107
# (ranges inclusive).
# All other commands will be ignored completely.
#
# Foreground colors named Bright* are not affected by the "bright" SGR
# setting—they will always appear bright, even after command 22 resets the
# normal intensity. Likewise, they will not affect the "bright" setting—a
# directive to use Red after BrightGreen will result in Red being displayed,
# not BrightRed.
#
# Palette or RGB Foreground colors from command 38 likewise do not interact
# with the "bright" setting, nor do any background colors.
class AnsiLexer(RegexLexer):
    name = 'Ansi escape lexer'

    _sgrs_and_text = '(?P<commands>(\x1b\\[(\\d*;)*\\d*m)+)(?P<text>[^\x1b]*)'
    _sgr_split = re.compile('m\x1b\\[|;|m|\x1b\\[')

    _named_colors = [
        'Black',
        'Red',
        'Green',
        'Yellow',
        'Blue',
        'Magenta',
        'Cyan',
        'White',
    ]
    _palette_start_colors = [
        '000000',
        '800000',
        '008000',
        '808000',
        '000080',
        '800080',
        '008080',
        'c0c0c0',
        '808080',
        'ff0000',
        '00ff00',
        'ffff00',
        '0000ff',
        'ff00ff',
        '00ffff',
        'ffffff',
    ]
    _palette_cube_steps = ['00', '5f', '87', 'af', 'd7', 'ff']

    def __init__(self, **options):
        RegexLexer.__init__(self, **options)

        self._bright = False
        self._foreground = ''
        self._background = ''

    def _callback(self, match):
        commands = match.group('commands')
        text = match.group('text')

        # split the commands strings into their constituent parameter codes
        parameters = self._sgr_split.split(commands)[1:-1]
        parameters = [int(p) if p else 0 for p in parameters]

        # loop over the parameters, consuming them to create commands, some
        # of which will have arguments
        while parameters:
            command = parameters.pop(0)
            if command == 0:
                self._bright = False
                self._foreground = ''
                self._background = ''
            elif command == 1:
                self._bright = True
            elif command == 22:
                self._bright = False
            elif command >= 30 and command <= 37:
                self._foreground = self._named_colors[command - 30]
            elif command == 38:
                mode = parameters.pop(0)
                if mode == 2:
                    rgb = self._read_rgb(parameters)
                else:
                    rgb = self._read_palette_color(parameters)
                self._foreground = rgb
            elif command == 39:
                self._foreground = ''
            elif command >= 40 and command <= 47:
                self._background = self._named_colors[command - 40]
            elif command == 48:
                mode = parameters.pop(0)
                if mode == 2:
                    rgb = self._read_rgb(parameters)
                else:
                    rgb = self._read_palette_color(parameters)
                self._background = rgb
            elif command == 49:
                self._background = ''
            elif command >= 90 and command <= 97:
                self._foreground = ('Bright' +
                                    self._named_colors[command - 90])
            elif command >= 100 and command <= 107:
                self._background = ('Bright' +
                                    self._named_colors[command - 100])

        if self._bright and self._foreground in self._named_colors:
            token = 'Bright' + self._foreground
        elif self._bright and not self._foreground:
            token = 'BrightDefault'
        else:
            token = self._foreground

        if (self._background):
            token += '-On' + self._background

        if token:
            token = 'Generic.Ansi' + token
            yield (match.start(), string_to_tokentype(token), text)
        else:
            yield (match.start(), Text, text)

    def _read_rgb(self, parameters):
        r = parameters.pop(0)
        g = parameters.pop(0)
        b = parameters.pop(0)
        return self._to_hex(r, g, b)

    def _read_palette_color(self, parameters):
        # the palette runs from 0–255 inclusive, consisting of
        #  - 16 specific colors intended to give a good range
        #  - 216 colors laid out on a color cube, with axes for
        #    each of red, green, and blue
        #  - 24 shades of grey, from grey3 to grey93
        offset = parameters.pop(0)
        if offset < 16:
            return self._palette_start_colors[offset]
        elif offset < 232:
            offset = offset - 16
            offset, b = divmod (offset,6)
            r, g = divmod(offset, 6)
            r = self._palette_cube_steps[r]
            g = self._palette_cube_steps[g]
            b = self._palette_cube_steps[b]
            return r + g + b
        else:
            shade = 8 + 10 * (offset - 232)
            return self._to_hex(shade, shade, shade)

    def _to_hex(self, r, g, b):
        return '{:02x}{:02x}{:02x}'.format(r, g, b)

    tokens = {
        'root': [
            ('[^\x1b]+', Text),
            (_sgrs_and_text, _callback),
        ]
    }

class HtmlAnsiFormatter(HtmlFormatter):
    _ansi_color_re = re.compile(
        '(?P<Prefix>class="g g-Ansi)'
        '((?P<RGBForeground>[0-9a-f]{6})|(?P<NamedForeground>[A-Za-z]+))?'
        '(-On((?P<RGBBackground>[0-9a-f]{6})|(?P<NamedBackground>[A-Za-z]+)))?'
        '(?P<Suffix>")'
    )

    def wrap(self, source, outfile):
        return self._wrap_code(source)

    def _wrap_code(self, source):
        for i, t in source:
            if i == 1: # it's a line of formatted code
                t = self._ansi_color_re.sub(self._replace_ansi_class, t)
            yield i, t

    def _replace_ansi_class(self, match):
        html_classes = ['g']
        html_styles = []

        named_foreground = match.group('NamedForeground')
        if named_foreground:
            html_classes.append("g-Ansi" + named_foreground)
        else:
            rgb_foreground = match.group('RGBForeground')
            if rgb_foreground:
                html_styles.append('color: #' + rgb_foreground)

        named_background = match.group('NamedBackground')
        if named_background:
            html_classes.append("g-AnsiBackground" + named_background)
        else:
            rgb_background = match.group('RGBBackground')
            if rgb_background:
                html_styles.append('background-color: #' + rgb_background)

        result = ''
        if len(html_classes) > 1: # we don't want to emit just g
            result = 'class="' + ' '.join(html_classes) + '"'

        if html_styles:
            if result:
                result += ' '
            result += 'style="' + '; '.join(html_styles) + '"'

        return result
