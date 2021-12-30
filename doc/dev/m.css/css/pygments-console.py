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

from pygments.style import Style
from pygments.token import Text, Generic

class ConsoleStyle(Style):

    background_color = None
    default_style = ''

    styles = {
        Text:                                 '#fcfcfc',

        # Shell session
        Generic.Prompt:                       'bold #16a085',  # same as BrightCyan
        Generic.Output:                       '#fcfcfc',

        # ANSI highlighting. Same order as in Konsole style dialog, following
        # the Breeze theme.
        Generic.AnsiBlack:                    '#232627',
        Generic.AnsiRed:                      '#ed1515',
        Generic.AnsiGreen:                    '#11d116',
        Generic.AnsiYellow:                   '#f67400',
        Generic.AnsiBlue:                     '#1d99f3',
        Generic.AnsiMagenta:                  '#9b59b6',
        Generic.AnsiCyan:                     '#1abc9c',
        Generic.AnsiWhite:                    '#fcfcfc',
        Generic.AnsiBrightBlack:              'bold #7f8c8d',
        Generic.AnsiBrightRed:                'bold #c0392b',
        Generic.AnsiBrightGreen:              'bold #1cdc9a',
        Generic.AnsiBrightYellow:             'bold #fdbc4b',
        Generic.AnsiBrightBlue:               'bold #3daee9',
        Generic.AnsiBrightMagenta:            'bold #8e44ad',
        Generic.AnsiBrightCyan:               'bold #16a085',
        Generic.AnsiBrightWhite:              'bold #ffffff',
        Generic.AnsiBrightDefault:            'bold #ffffff',
        Generic.AnsiBackgroundBlack:          'bg:#232627',
        Generic.AnsiBackgroundRed:            'bg:#ed1515',
        Generic.AnsiBackgroundGreen:          'bg:#11d116',
        Generic.AnsiBackgroundYellow:         'bg:#f67400',
        Generic.AnsiBackgroundBlue:           'bg:#1d99f3',
        Generic.AnsiBackgroundMagenta:        'bg:#9b59b6',
        Generic.AnsiBackgroundCyan:           'bg:#1abc9c',
        Generic.AnsiBackgroundWhite:          'bg:#fcfcfc',
        Generic.AnsiBackgroundBrightBlack:    'bg:#7f8c8d',
        Generic.AnsiBackgroundBrightRed:      'bg:#c0392b',
        Generic.AnsiBackgroundBrightGreen:    'bg:#1cdc9a',
        Generic.AnsiBackgroundBrightYellow:   'bg:#fdbc4b',
        Generic.AnsiBackgroundBrightBlue:     'bg:#3daee9',
        Generic.AnsiBackgroundBrightMagenta:  'bg:#8e44ad',
        Generic.AnsiBackgroundBrightCyan:     'bg:#16a085',
        Generic.AnsiBackgroundBrightWhite:    'bg:#ffffff',
    }
