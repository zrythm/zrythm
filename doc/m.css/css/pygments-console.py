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

from pygments.style import Style
from pygments.token import Text, Generic

class ConsoleStyle(Style):

    background_color = None
    default_style = ''

    styles = {
        Text:                       '#b2b2b2',

        # Shell session
        Generic.Prompt:             'bold #54ffff',
        Generic.Output:             '#b2b2b2',

        # ANSI highlighting
        Generic.AnsiBlack:          '#000000',
        Generic.AnsiRed:            '#b21818',
        Generic.AnsiGreen:          '#18b218',
        Generic.AnsiYellow:         '#b26818',
        Generic.AnsiBlue:           '#3f3fd1',
        Generic.AnsiMagenta:        '#b218b2',
        Generic.AnsiCyan:           '#18b2b2',
        Generic.AnsiWhite:          '#b2b2b2',
        Generic.AnsiDefault:        '#b2b2b2',
        Generic.AnsiBrightBlack:    'bold #686868',
        Generic.AnsiBrightRed:      'bold #ff5454',
        Generic.AnsiBrightGreen:    'bold #54ff54',
        Generic.AnsiBrightYellow:   'bold #ffff54',
        Generic.AnsiBrightBlue:     'bold #5454ff',
        Generic.AnsiBrightMagenta:  'bold #ff54ff',
        Generic.AnsiBrightCyan:     'bold #54ffff',
        Generic.AnsiBrightWhite:    'bold #ffffff',
        Generic.AnsiBrightDefault:  'bold #ffffff'
    }
