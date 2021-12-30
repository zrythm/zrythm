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
from docutils import nodes, utils
from docutils.parsers import rst
from docutils.parsers.rst.roles import set_classes

# to avoid dependencies, link_regexp and parse_link() is common for m.abbr,
# m.gh, m.gl, m.link and m.vk

link_regexp = re.compile(r'(?P<title>.*) <(?P<link>.+)>')

def parse_link(text):
    link = utils.unescape(text)
    m = link_regexp.match(link)
    if m: return m.group('title', 'link')
    return None, link

def glext(name, rawtext, text, lineno, inliner, options={}, content=[]):
    title, extension = parse_link(text)
    if not title: title = extension
    prefix = extension.partition('_')[0]
    url = "https://www.khronos.org/registry/OpenGL/extensions/{}/{}.txt".format(prefix, extension)
    set_classes(options)
    node = nodes.reference(rawtext, title, refuri=url, **options)
    return [node], []

def webglext(name, rawtext, text, lineno, inliner, options={}, content=[]):
    title, extension = parse_link(text)
    if not title: title = extension
    url = "https://www.khronos.org/registry/webgl/extensions/{}/".format(extension)
    set_classes(options)
    node = nodes.reference(rawtext, title, refuri=url, **options)
    return [node], []

def glfn(name, rawtext, text, lineno, inliner, options={}, content=[]):
    title, fn = parse_link(text)
    if not title: title = "gl{}()".format(fn)
    url = "https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/gl{}.xhtml".format(fn)
    set_classes(options)
    node = nodes.reference(rawtext, title, refuri=url, **options)
    return [node], []

def glfnext(name, rawtext, text, lineno, inliner, options={}, content=[]):
    title, extension = parse_link(text)
    prefix = extension.partition('_')[0]
    url = "https://www.khronos.org/registry/OpenGL/extensions/{}/{}.txt".format(prefix, extension)
    set_classes(options)
    node = nodes.reference(rawtext, "gl" + title + prefix + "()", refuri=url, **options)
    return [node], []

def register_mcss(**kwargs):
    rst.roles.register_local_role('glext', glext)
    rst.roles.register_local_role('webglext', webglext)
    rst.roles.register_local_role('glfn', glfn)
    rst.roles.register_local_role('glfnext', glfnext)

# Below is only Pelican-specific functionality. If Pelican is not found, these
# do nothing.

register = register_mcss # for Pelican
