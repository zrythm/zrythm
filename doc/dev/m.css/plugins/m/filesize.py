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

import os
import gzip
from docutils import nodes
from docutils.parsers import rst
from docutils.parsers.rst.roles import set_classes

settings = {}

def filesize(name, rawtext, text, lineno, inliner, options={}, content=[]):
    # Support both {filename} (3.7.1) and {static} (3.8) placeholders
    size = os.path.getsize(text.format(filename=settings['INPUT'], static=settings['INPUT']))

    for unit in ['','k','M','G','T']:
        if abs(size) < 1024.0:
            size_string = "%3.1f %sB" % (size, unit)
            break
        size /= 1024.0
    else: size_string = "%.1f PB" % size

    set_classes(options)
    return [nodes.inline(size_string, size_string, **options)], []

def filesize_gz(name, rawtext, text, lineno, inliner, options={}, content=[]):
    # Support both {filename} (3.7.1) and {static} (3.8) placeholders
    with open(text.format(filename=settings['INPUT'], static=settings['INPUT']), mode='rb') as f:
        size = len(gzip.compress(f.read()))

    for unit in ['','k','M','G','T']:
        if abs(size) < 1024.0:
            size_string = "%3.1f %sB" % (size, unit)
            break
        size /= 1024.0
    else: size_string = "%.1f PB" % size

    set_classes(options)
    return [nodes.inline(size_string, size_string, **options)], []

def register_mcss(mcss_settings, **kwargs):
    global settings
    settings['INPUT'] = mcss_settings['INPUT']
    rst.roles.register_local_role('filesize', filesize)
    rst.roles.register_local_role('filesize-gz', filesize_gz)

# Below is only Pelican-specific functionality. If Pelican is not found, these
# do nothing.

def _pelican_configure(pelicanobj):
    settings = {
        'INPUT': os.path.join(os.getcwd(), pelicanobj.settings['PATH'])
    }
    register_mcss(mcss_settings=settings)

def register(): # for Pelican
    from pelican import signals

    signals.initialized.connect(_pelican_configure)
