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
from pygments.token import Keyword, Name, Comment, String, Error, \
    Literal, Number, Operator, Other, Punctuation, Text, Generic, \
    Whitespace

class DarkStyle(Style):
    background_color = None
    highlight_color = '#34424d'
    default_style = ""

    styles = {
        # C++
        Comment:                '#a5c9ea',
        Comment.Preproc:        '#3bd267',
        Comment.PreprocFile:    '#c7cf2f',
        Keyword:                'bold #ffffff',
        Name:                   '#dcdcdc',
        String:                 '#e07f7c',
        String.Char:            '#e07cdc',
        String.Escape:          '#e07cdc', # like char
        String.Interpol:        '#a5c9ea', # like comment
        Number:                 '#c7cf2f',
        Operator:               '#aaaaaa',
        Punctuation:            "#aaaaaa",

        # CMake
        Name.Builtin:           'bold #ffffff',
        Name.Variable:          '#c7cf2f',

        # reST, HTML
        Name.Tag:               'bold #dcdcdc',
        Name.Attribute:         'bold #dcdcdc',
        Name.Class:             'bold #dcdcdc',
        Operator.Word:          'bold #dcdcdc',
        Generic.Heading:        'bold #ffffff',
        Generic.Emph:           'italic #e6e6e6',
        Generic.Strong:         'bold #e6e6e6',

        # Diffs
        Generic.Subheading:     '#5b9dd9',
        Generic.Inserted:       '#3bd267',
        Generic.Deleted:        '#cd3431'
    }
