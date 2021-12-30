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

from docutils.parsers.rst.states import Inliner
from docutils import nodes, utils
from docutils.parsers import rst
from docutils.parsers.rst.roles import set_classes

import xml.etree.ElementTree as ET
import os
import re

import logging

logger = logging.getLogger(__name__)

# Modified from abbr / gh / gl / ... to add support for queries and hashes
link_regexp = re.compile(r'(?P<title>.*) <(?P<link>[^?#]+)(?P<hash>[?#].+)?>')

def parse_link(text):
    link = utils.unescape(text)
    m = link_regexp.match(link)
    if m:
        title, link, hash = m.group('title', 'link', 'hash')
        if not hash: hash = '' # it's None otherwise
    else:
        title, hash = '', ''

    return title, link, hash

def init(tagfiles, input):
    global symbol_mapping, symbol_prefixes, tagfile_basenames

    # Pre-round to populate subclasses. Clear everything in case we init'd
    # before already.
    tagfile_basenames = []
    symbol_mapping = {}
    symbol_prefixes = ['']

    for f in tagfiles:
        tagfile, path = f[:2]
        prefixes = f[2] if len(f) > 2 else []
        css_classes = f[3] if len(f) > 3 else []

        tagfile_basenames += [(os.path.splitext(os.path.basename(tagfile))[0], path, css_classes)]
        symbol_prefixes += prefixes

        tree = ET.parse(os.path.join(input, tagfile))
        root = tree.getroot()
        for child in root:
            if child.tag == 'compound' and 'kind' in child.attrib:
                # Linking to pages
                if child.attrib['kind'] == 'page':
                    link = path + child.find('filename').text + '.html'
                    symbol_mapping[child.find('name').text] = (child.find('title').text, link, css_classes)

                # Linking to files
                if child.attrib['kind'] == 'file':
                    file_path = child.find('path')
                    link = path + child.find('filename').text + ".html"
                    symbol_mapping[(file_path.text if file_path is not None else '') + child.find('name').text] = (None, link, css_classes)

                    for member in child.findall('member'):
                        if not 'kind' in member.attrib: continue

                        # Preprocessor defines and macros
                        if member.attrib['kind'] == 'define':
                            symbol_mapping[member.find('name').text + ('()' if member.find('arglist').text else '')] = (None, link + '#' + member.find('anchor').text, css_classes)

                # Linking to namespaces, structs and classes
                if child.attrib['kind'] in ['class', 'struct', 'namespace']:
                    name = child.find('name').text
                    link = path + child.findtext('filename') # <filename> can be empty (cppreference tag file)
                    symbol_mapping[name] = (None, link, css_classes)
                    for member in child.findall('member'):
                        if not 'kind' in member.attrib: continue

                        # Typedefs, constants
                        if member.attrib['kind'] == 'typedef' or member.attrib['kind'] == 'enumvalue':
                            symbol_mapping[name + '::' + member.find('name').text] = (None, link + '#' + member.find('anchor').text, css_classes)

                        # Functions
                        if member.attrib['kind'] == 'function':
                            # <filename> can be empty (cppreference tag file)
                            symbol_mapping[name + '::' + member.find('name').text + "()"] = (None, link + '#' + member.findtext('anchor'), css_classes)

                        # Enums with values
                        if member.attrib['kind'] == 'enumeration':
                            enumeration = name + '::' + member.find('name').text
                            symbol_mapping[enumeration] = (None, link + '#' + member.find('anchor').text, css_classes)

                            for value in member.findall('enumvalue'):
                                symbol_mapping[enumeration + '::' + value.text] = (None, link + '#' + value.attrib['anchor'], css_classes)

                # Sections
                for section in child.findall('docanchor'):
                    symbol_mapping[section.text] = (section.attrib.get('title', ''), link + '#' + section.text, css_classes)

def dox(name, rawtext, text, lineno, inliner: Inliner, options={}, content=[]):
    title, target, hash = parse_link(text)

    # Otherwise adding classes to the options behaves globally (uh?)
    _options = dict(options)
    set_classes(_options)
    # Avoid assert on adding to undefined member later
    if 'classes' not in _options: _options['classes'] = []

    # Try linking to the whole docs first
    for basename, url, css_classes in tagfile_basenames:
        if basename == target:
            if not title:
                # TODO: extract title from index page in the tagfile
                logger.warning("Link to main page `{}` requires a title".format(target))
                title = target

            _options['classes'] += css_classes
            node = nodes.reference(rawtext, title, refuri=url + hash, **_options)
            return [node], []

    for prefix in symbol_prefixes:
        if prefix + target in symbol_mapping:
            link_title, url, css_classes = symbol_mapping[prefix + target]
            if title:
                use_title = title
            elif link_title:
                use_title = link_title
            else:
                if link_title is not None:
                    logger.warning("Doxygen anchor `{}` has no title, using its ID as link title".format(target))

                use_title = target

            _options['classes'] += css_classes
            node = nodes.reference(rawtext, use_title, refuri=url + hash, **_options)
            return [node], []

    # TODO: print file and line
    #msg = inliner.reporter.warning(
        #'Doxygen symbol %s not found' % target, line=lineno)
    #prb = inliner.problematic(rawtext, rawtext, msg)
    if title:
        logger.warning("Doxygen symbol `{}` not found, rendering just link title".format(target))
        node = nodes.inline(rawtext, title, **_options)
    else:
        logger.warning("Doxygen symbol `{}` not found, rendering as monospace".format(target))
        node = nodes.literal(rawtext, target, **_options)
    return [node], []

def register_mcss(mcss_settings, **kwargs):
    rst.roles.register_local_role('dox', dox)

    init(input=mcss_settings['INPUT'],
         tagfiles=mcss_settings.get('M_DOX_TAGFILES', []))

# Below is only Pelican-specific functionality. If Pelican is not found, these
# do nothing.

def _pelican_configure(pelicanobj):
    settings = {
        # For backwards compatibility, the input directory is pelican's CWD
        'INPUT': os.getcwd(),
    }
    for key in ['M_DOX_TAGFILES']:
        if key in pelicanobj.settings: settings[key] = pelicanobj.settings[key]

    register_mcss(mcss_settings=settings)

def register(): # for Pelican
    from pelican import signals

    signals.initialized.connect(_pelican_configure)
