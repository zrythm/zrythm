#!/usr/bin/env python

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
import sys

# So it's possible to do `./plugins/m/sphinx.py somefile.inv` without it all
# blowing up on the earliest `import math` as there's a math.py file in this
# directory. Alternatively, doing `python -m m.sphinx` would work too, but who
# would remember to do that.
if __name__ == '__main__': # pragma: no cover
    sys.path.remove(os.path.realpath(os.path.dirname(__file__)))

import argparse
import logging
import re
from types import SimpleNamespace as Empty
from typing import Dict, List, Optional
from urllib.parse import urljoin
import zlib

import docutils
from docutils import nodes, utils
from docutils.parsers import rst
from docutils.parsers.rst import directives
from docutils.parsers.rst.roles import set_classes
from docutils.parsers.rst.states import Inliner

sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..'))
import m.htmlsanity

# All those initialized in register() or register_mcss()
current_referer_path = None
page_ref_prefixes = None
current_param_names = None
module_doc_output = None
class_doc_output = None
enum_value_doc_output = None
enum_doc_output = None
function_doc_output = None
property_doc_output = None
data_doc_output = None
inventory_filename = None

# List option (allowing multiple arguments). See _docutils_assemble_option_dict
# in python.py for details.
def directives_unchanged_list(argument):
    return [directives.unchanged(argument)]

class PyModule(rst.Directive):
    final_argument_whitespace = True
    has_content = True
    required_arguments = 1
    option_spec = {'summary': directives.unchanged,
                   'data': directives_unchanged_list}

    def run(self):
        # Check that data are parsed properly, turn them into a dict. This will
        # blow up if the data name is not specified.
        properties = {}
        data = {}
        for name, summary in self.options.get('data', []):
            if name in data: raise KeyError("duplicate data {}".format(name))
            data[name] = summary

        output = module_doc_output.setdefault(self.arguments[0], {})
        if self.options.get('summary'):
            output['summary'] = self.options['summary']
        if self.content:
            output['content'] = '\n'.join(self.content)

        for name, summary in data.items():
            data_doc_output.setdefault('{}.{}'.format(self.arguments[0], name), {})['summary'] = summary

        return []

class PyClass(rst.Directive):
    final_argument_whitespace = True
    has_content = True
    required_arguments = 1
    option_spec = {'summary': directives.unchanged,
                   'property': directives_unchanged_list,
                   'data': directives_unchanged_list}

    def run(self):
        # Check that properties and data are parsed properly, turn them into
        # dicts. This will blow up if the property / data name is not specified.
        properties = {}
        for name, summary in self.options.get('property', []):
            if name in properties: raise KeyError("duplicate property {}".format(name))
            properties[name] = summary
        data = {}
        for name, summary in self.options.get('data', []):
            if name in data: raise KeyError("duplicate data {}".format(name))
            data[name] = summary

        output = class_doc_output.setdefault(self.arguments[0], {})
        if self.options.get('summary'):
            output['summary'] = self.options['summary']
        if self.content:
            output['content'] = '\n'.join(self.content)

        for name, summary in properties.items():
            property_doc_output.setdefault('{}.{}'.format(self.arguments[0], name), {})['summary'] = summary
        for name, summary in data.items():
            data_doc_output.setdefault('{}.{}'.format(self.arguments[0], name), {})['summary'] = summary

        return []

class PyEnum(rst.Directive):
    final_argument_whitespace = True
    has_content = True
    required_arguments = 1
    option_spec = {'summary': directives.unchanged,
                   'value': directives_unchanged_list}

    def run(self):
        # Check that values are parsed properly, turn them into a dict. This
        # will blow up if the param name is not specified.
        values = {}
        for name, summary in self.options.get('value', []):
            if name in values: raise KeyError("duplicate param {}".format(name))
            values[name] = summary

        output = enum_doc_output.setdefault(self.arguments[0], {})
        if self.options.get('summary'):
            output['summary'] = self.options['summary']
        if self.content:
            output['content'] = '\n'.join(self.content)

        for name, content in values.items():
            enum_value_doc_output.setdefault('{}.{}'.format(self.arguments[0], name), {})['content'] = content

        return []

class PyEnumValue(rst.Directive):
    final_argument_whitespace = True
    has_content = True
    required_arguments = 1

    def run(self):
        output = enum_value_doc_output.setdefault(self.arguments[0], {})
        if self.content:
            output['content'] = '\n'.join(self.content)
        return []

class PyFunction(rst.Directive):
    final_argument_whitespace = True
    has_content = True
    required_arguments = 1
    option_spec = {'summary': directives.unchanged,
                   'param': directives_unchanged_list,
                   'raise': directives_unchanged_list,
                   'return': directives.unchanged}

    def run(self):
        # Check that params are parsed properly, turn them into a dict. This
        # will blow up if the param name is not specified.
        params = {}
        for name, content in self.options.get('param', []):
            if name in params: raise KeyError("duplicate param {}".format(name))
            params[name] = content
        # Check that exceptions are parsed properly. This will blow up if the
        # exception name is not specified. Unlike function params not turning
        # these into a dict since a single type can be raised for multiple
        # reasons.
        raises = []
        for name, content in self.options.get('raise', []):
            raises += [(name, content)]

        output = function_doc_output.setdefault(self.arguments[0], {})
        if self.options.get('summary'):
            output['summary'] = self.options['summary']
        if params:
            output['params'] = params
        if raises:
            output['raise'] = raises
        if self.options.get('return'):
            output['return'] = self.options['return']
        if self.content:
            output['content'] = '\n'.join(self.content)
        return []

class PyProperty(rst.Directive):
    final_argument_whitespace = True
    has_content = True
    required_arguments = 1
    option_spec = {'summary': directives.unchanged,
                   'raise': directives_unchanged_list}

    def run(self):
        # Check that exceptions are parsed properly. This will blow up if the
        # exception name is not specified. Unlike function params not turning
        # these into a dict since a single type can be raised for multiple
        # reasons.
        raises = []
        for name, content in self.options.get('raise', []):
            raises += [(name, content)]

        output = property_doc_output.setdefault(self.arguments[0], {})
        if raises:
            output['raise'] = raises
        if self.options.get('summary'):
            output['summary'] = self.options['summary']
        if self.content:
            output['content'] = '\n'.join(self.content)
        return []

class PyData(rst.Directive):
    final_argument_whitespace = True
    has_content = True
    required_arguments = 1
    option_spec = {'summary': directives.unchanged}

    def run(self):
        output = data_doc_output.setdefault(self.arguments[0], {})
        if self.options.get('summary'):
            output['summary'] = self.options['summary']
        if self.content:
            output['content'] = '\n'.join(self.content)
        return []

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

intersphinx_inventory = {}
intersphinx_name_prefixes = []

# Basically a copy of sphinx.util.inventory.InventoryFile.load_v2. There's no
# documentation for this, it seems.
def parse_intersphinx_inventory(file, base_url, inventory, css_classes):
    # Parse the header, uncompressed
    inventory_version = file.readline().rstrip()
    if inventory_version != b'# Sphinx inventory version 2':
        raise ValueError("Unsupported inventory version header: {}".format(inventory_version)) # pragma: no cover
    # those two are not used at the moment, just for completeness
    project = file.readline().rstrip()[11:]
    version = file.readline().rstrip()[11:]
    line = file.readline()
    if b'zlib' not in line:
        raise ValueError("invalid inventory header (not compressed): {}".format(line)) # pragma: no cover

    # Decompress the rest. Again mostly a copy of the sphinx code.
    for line in zlib.decompress(file.read()).decode('utf-8').splitlines():
        m = re.match(r'(?x)(.+?)\s+(\S*:\S*)\s+(-?\d+)\s+(\S+)\s+(.*)',
                         line.rstrip())
        if not m: # pragma: no cover
            print("wait what is this line?! {}".format(line))
            continue
        # What the F is prio for
        name, type, prio, location, title = m.groups()

        # What is this?!
        if location.endswith('$'): location = location[:-1] + name

        # The original code `continue`s in this case. I'm asserting. Fix your
        # docs.
        assert not(type == 'py:module' and type in inventory and name in inventory[type]), "Well dang, we hit that bug in 1.1 that I didn't want to work around" # pragma: no cover

        # Prepend base URL and add to the inventory
        inventory.setdefault(type, {})[name] = (urljoin(base_url, location), title, css_classes)

def parse_intersphinx_inventories(input, inventories):
    global intersphinx_inventory, intersphinx_name_prefixes
    intersphinx_inventory = {}
    intersphinx_name_prefixes = ['']

    for f in inventories:
        inventory, base_url = f[:2]
        prefixes = f[2] if len(f) > 2 else []
        css_classes = f[3] if len(f) > 3 else []

        intersphinx_name_prefixes += prefixes
        with open(os.path.join(input, inventory), 'rb') as file:
            parse_intersphinx_inventory(file, base_url, intersphinx_inventory, css_classes)

# Matches e.g. py:function in py:function:open
_type_prefix_re = re.compile(r'([a-z0-9]{,3}:[a-z0-9]{3,}):')
_function_types = ['py:function', 'py:classmethod', 'py:staticmethod', 'py:method', 'c:function']

def ref(name, rawtext, text, lineno, inliner: Inliner, options={}, content=[]):
    title, target, hash = parse_link(text)

    # Otherwise adding classes to the options behaves globally (uh?)
    _options = dict(options)
    set_classes(_options)
    # Avoid assert on adding to undefined member later
    if 'classes' not in _options: _options['classes'] = []

    # If we're in a page and there's no page module scope yet, look if there
    # are :py:module: page metadata we could use for a prefix
    global current_referer_path, page_ref_prefixes
    if current_referer_path[-1][0].name == 'PAGE' and page_ref_prefixes is None:
        # Since we're in the middle of parse, the nodes.docinfo is not present
        # yet (it's produced by the frontmatter.DocInfo transform that's run
        # after the parsing ends), so we look in field lists instead.
        # TODO: DocInfo picks only the first ever field list happening right
        #   after a title and we should do the same to avoid picking up options
        #   later in the page. There the transform depends on DocTitle being
        #   ran already, so it would need to be more complex here. See
        #   docutils.transforms.frontmatter.DocInfo.apply() for details.
        for docinfo in inliner.document.traverse(docutils.nodes.field_list):
            for element in docinfo.children:
                if element.tagname != 'field': continue
                name_elem, body_elem = element.children
                if name_elem.astext() == 'ref-prefix':
                    page_ref_prefixes = [line.strip() + '.' for line in body_elem.astext().splitlines() if line.strip()]

        # If we didn't find any, set it to an empty list (not None), so this is
        # not traversed again next time
        if not page_ref_prefixes: page_ref_prefixes = []

    # Add prefixes of the referer path to the global prefix list. Make an empty
    # prefix first so :ref:`open()` always reliably links to the builtin
    # instead whatever `foo.bar.open()` that's currently in scope. After that
    # the names "closest" to the referer have the biggest priority with shorter
    # referer_path prefixes after, and name prefixes from M_SPHINX_INVENTORIES
    # last.
    global intersphinx_inventory, intersphinx_name_prefixes
    referer_path = current_referer_path[-1][1] if current_referer_path else []
    prefixes = [''] + ['.'.join(referer_path[:len(referer_path) - i]) + '.' for i, _ in enumerate(referer_path)] + (page_ref_prefixes if page_ref_prefixes else []) + intersphinx_name_prefixes
    for prefix in prefixes:
        found = None

        # If the target is prefixed with a type, try looking up that type
        # directly. The implicit link title is then without the type.
        m = _type_prefix_re.match(target)
        if m:
            type = m.group(1)
            prefixed = prefix + target[len(type) + 1:]
            # ALlow trailing () on functions here as well
            if prefixed.endswith('()') and type in _function_types:
                prefixed = prefixed[:-2]
            if type in intersphinx_inventory and prefixed in intersphinx_inventory[type]:
                target = target[len(type) + 1:]
                found = type, intersphinx_inventory[m.group(1)][prefixed]

        prefixed = prefix + target

        # If the target looks like a function, look only in functions and strip
        # the trailing () as the inventory doesn't have that
        if not found and prefixed.endswith('()'):
            prefixed = prefixed[:-2]
            for type in _function_types:
                if type in intersphinx_inventory and prefixed in intersphinx_inventory[type]:
                    found = type, intersphinx_inventory[type][prefixed]
                    break

        # Iterate through whitelisted types otherwise. Skipping
        # 'std:pdbcommand', 'std:cmdoption', 'std:term', 'std:label',
        # 'std:opcode', 'std:envvar', 'std:token', 'std:doc', 'std:2to3fixer'
        # and unknown domains such as c++ for now as I'm unsure about potential
        # name clashes.
        if not found:
            for type in [
                'py:exception', 'py:attribute', 'py:method', 'py:data', 'py:module', 'py:function', 'py:class', 'py:classmethod', 'py:staticmethod',
                'c:var', 'c:type', 'c:function', 'c:member', 'c:macro',
                # TODO: those apparently don't exist:
                'py:enum', 'py:enumvalue'
            ]:
                if type in intersphinx_inventory and prefixed in intersphinx_inventory[type]:
                    found = type, intersphinx_inventory[type][prefixed]

        if found:
            url, link_title, css_classes = found[1]
            if title:
                use_title = title
            elif link_title != '-':
                use_title = link_title
            else:
                use_title = target
                # Add () to function refs
                if found[0] in _function_types and not target.endswith('()'):
                    use_title += '()'

            _options['classes'] += css_classes
            node = nodes.reference(rawtext, use_title, refuri=url + hash, **_options)
            return [node], []

    if title:
        logging.warning("Sphinx symbol `{}` not found, rendering just link title".format(target))
        node = nodes.inline(rawtext, title, **_options)
    else:
        logging.warning("Sphinx symbol `{}` not found, rendering as monospace".format(target))
        node = nodes.literal(rawtext, target, **_options)
    return [node], []

def scope_enter(type, path, param_names=None, **kwargs):
    global current_referer_path, current_param_names
    current_referer_path += [(type, path)]
    current_param_names = param_names

    # If we're in a page, reset page_ref_prefixes so next time :ref: needs it,
    # it will look it up instead of using a stale version
    global page_ref_prefixes
    if type.name == 'PAGE': page_ref_prefixes = None

def scope_exit(type, path, **kwargs):
    global current_referer_path, current_param_names
    assert current_referer_path[-1] == (type, path), "%s %s" % (current_referer_path, path)
    current_referer_path = current_referer_path[:-1]
    current_param_names = None

def check_scope_stack_empty(**kwargs):
    global current_referer_path
    assert not current_referer_path

def p(name, rawtext, text, lineno, inliner: Inliner, options={}, content=[]):
    global current_referer_path, current_param_names
    if not current_param_names:
        logging.warning("can't reference parameter %s outside of a function scope", text)
    elif text not in current_param_names:
        logging.warning("parameter %s not found in %s(%s) function signature", text, '.'.join(current_referer_path[-1][1]), ', '.join(current_param_names))

    node = nodes.literal(rawtext, text, **options)
    return [node], []

def consume_docstring(type, path: List[str], signature: Optional[str], doc: str) -> str:
    # Create the directive header based on type
    if type.name == 'MODULE':
        source = '.. py:module:: '
        doc_output = module_doc_output
    elif type.name == 'CLASS':
        source = '.. py:class:: '
        doc_output = class_doc_output
    elif type.name == 'ENUM':
        source = '.. py:enum:: '
        doc_output = enum_doc_output
    elif type.name == 'ENUM_VALUE':
        source = '.. py:enumvalue:: '
        doc_output = enum_value_doc_output
    elif type.name in ['FUNCTION', 'OVERLOADED_FUNCTION']:
        source = '.. py:function:: '
        doc_output = function_doc_output
    elif type.name == 'PROPERTY':
        source = '.. py:property:: '
        doc_output = property_doc_output
    else: # pragma: no cover
        # Data don't have docstrings, you silly
        assert type.name != 'DATA'
        # Ignore unknown types, pass the docs through
        return doc

    # Add path and signature to the header
    path_signature_str = '.'.join(path) + (signature if signature else '')
    source += path_signature_str + '\n'

    # Everything except enum values has a separate summary. Assuming first
    # paragraph is a summary, turn it into a :summary: directive option with
    # successive lines indented.
    if type.name != 'ENUM_VALUE':
        summary, _, doc = doc.partition('\n\n')
        source += '    :summary: {}\n'.format(summary.replace('\n', '\n        '))

    # The next paragraph could be option list. If that's so, indent those as
    # well, append
    if doc.startswith(':'):
        options, _, doc = doc.partition('\n\n')
        source += '    {}\n\n'.format(options.replace('\n', '\n    '))
    else:
        source += '\n'

    # The rest (if any) is content. Indent as well.
    source += '    {}\n'.format(doc.replace('\n', '\n    '))

    # Unleash docutils on this piece. It will call into the proper directive
    # and do the thing. Ignore the output as there shouldn't be anything left.
    pub = docutils.core.Publisher(
        writer=m.htmlsanity.SaneHtmlWriter(),
        source_class=docutils.io.StringInput,
        destination_class=docutils.io.StringOutput)
    pub.set_components('standalone', 'restructuredtext', 'html')
    pub.writer.translator_class = m.htmlsanity.SaneHtmlTranslator
    pub.process_programmatic_settings(None, m.htmlsanity.docutils_settings, None)
    # Docutils uses a deprecated U mode for opening files, so instead of
    # monkey-patching docutils.io.FileInput to not do that (like Pelican does),
    # I just read the thing myself.
    # TODO it *somehow* needs to supply the original docstring filename and
    # line range to it for better error reporting, this is too awful
    pub.set_source(source=source)
    pub.publish()

def merge_inventories(name_map, **kwargs):
    global intersphinx_inventory

    # Create inventory entries from the name_map
    internal_inventory = {}
    for path_str, entry in name_map.items():
        EntryType = type(entry.type) # so we don't need to import the enum
        if entry.type == EntryType.MODULE:
            type_string = 'py:module'
        elif entry.type == EntryType.CLASS:
            type_string = 'py:class'
        elif entry.type == EntryType.FUNCTION:
            # TODO: properly distinguish between 'py:function',
            # 'py:classmethod', 'py:staticmethod', 'py:method'
            type_string = 'py:function'
        elif entry.type == EntryType.OVERLOADED_FUNCTION:
            # TODO: what about the other overloads?
            type_string = 'py:function'
        elif entry.type == EntryType.PROPERTY:
            # datetime.date.year is decorated with @property and listed as a
            # py:attribute, so that's probably it
            type_string = 'py:attribute'
        elif entry.type == EntryType.ENUM:
            type_string = 'py:enum' # this desn't exist in Sphinx
        elif entry.type == EntryType.ENUM_VALUE:
            type_string = 'py:enumvalue' # these don't exist in Sphinx
        elif entry.type == EntryType.DATA:
            type_string = 'py:data'
        elif entry.type == EntryType.PAGE:
            type_string = 'std:doc'
        elif entry.type == EntryType.SPECIAL:
            # TODO: this will cause duplicates when multiple m.css projects
            #   gets together, solve better
            type_string = 'std:special'
        else: # pragma: no cover
            assert False

        # Mark those with m-doc (as internal)
        internal_inventory.setdefault(type_string, {})[path_str] = (entry.url, '-', ['m-doc'])

    # Add class / enum / enum value inventory entries to the name map for type
    # cross-linking
    for type_, type_string in [
        # TODO: this will blow up if the above loop is never entered (which is
        # unlikely) as EntryType is defined there
        (EntryType.CLASS, 'py:class'),
        # Otherwise we can't link to standard exceptions from :raise:
        (EntryType.CLASS, 'py:exception'), # TODO: special type for these?
        (EntryType.DATA, 'py:data'), # typing.Tuple or typing.Any is data
        # Those are custom to m.css, not in Sphinx
        (EntryType.ENUM, 'py:enum'),
        (EntryType.ENUM_VALUE, 'py:enumvalue'),
    ]:
        if type_string in intersphinx_inventory:
            for path, value in intersphinx_inventory[type_string].items():
                url, _, css_classes = value
                entry = Empty()
                entry.type = type_
                entry.object = None
                entry.path = path.split('.')
                entry.css_classes = css_classes
                entry.url = url
                name_map[path] = entry

    # Add stuff from the name map to our inventory
    for type_, data_internal in internal_inventory.items():
        data = intersphinx_inventory.setdefault(type_, {})
        for path, value in data_internal.items():
            # Ignore duplicate things (such as `index` etc.)
            # TODO: solve better
            if path in data: continue
            data[path] = value

    # Save the internal inventory, if requested. Again basically a copy of
    # sphinx.util.inventory.InventoryFile.dump().
    if inventory_filename:
        with open(os.path.join(inventory_filename), 'wb') as f:
            # Header
            # TODO: user-defined project/version
            f.write(b'# Sphinx inventory version 2\n'
                    b'# Project: X\n'
                    b'# Version: 0\n'
                    b'# The remainder of this file is compressed using zlib.\n')

            # Body. Sorting so it's in a reproducible order for testing.
            compressor = zlib.compressobj(9)
            for type_, data in sorted(internal_inventory.items()):
                for path, value in data.items():
                    url, title, css_classes = value
                    # The type has to contain a colon. Wtf is the 2?
                    assert ':' in type_
                    f.write(compressor.compress('{} {} 2 {} {}\n'.format(path, type_, url, title).encode('utf-8')))
            f.write(compressor.flush())

def register_mcss(mcss_settings, module_doc_contents, class_doc_contents, enum_doc_contents, enum_value_doc_contents, function_doc_contents, property_doc_contents, data_doc_contents, hooks_post_crawl, hooks_pre_scope, hooks_post_scope, hooks_docstring, hooks_post_run, **kwargs):
    global current_referer_path, module_doc_output, class_doc_output, enum_doc_output, enum_value_doc_output, function_doc_output, property_doc_output, data_doc_output, inventory_filename
    current_referer_path = []
    module_doc_output = module_doc_contents
    class_doc_output = class_doc_contents
    enum_doc_output = enum_doc_contents
    enum_value_doc_output = enum_value_doc_contents
    function_doc_output = function_doc_contents
    property_doc_output = property_doc_contents
    data_doc_output = data_doc_contents
    inventory_filename = os.path.join(mcss_settings['OUTPUT'], mcss_settings['M_SPHINX_INVENTORY_OUTPUT']) if 'M_SPHINX_INVENTORY_OUTPUT' in mcss_settings else None

    parse_intersphinx_inventories(input=mcss_settings['INPUT'],
         inventories=mcss_settings.get('M_SPHINX_INVENTORIES', []))

    rst.directives.register_directive('py:module', PyModule)
    rst.directives.register_directive('py:class', PyClass)
    rst.directives.register_directive('py:enum', PyEnum)
    rst.directives.register_directive('py:enumvalue', PyEnumValue)
    rst.directives.register_directive('py:function', PyFunction)
    rst.directives.register_directive('py:property', PyProperty)
    rst.directives.register_directive('py:data', PyData)

    rst.roles.register_local_role('ref', ref)
    rst.roles.register_local_role('p', p)

    hooks_pre_scope += [scope_enter]
    hooks_post_scope += [scope_exit]
    if mcss_settings.get('M_SPHINX_PARSE_DOCSTRINGS', False):
        hooks_docstring += [consume_docstring]
    hooks_post_crawl += [merge_inventories]
    # Just a sanity check
    hooks_post_run += [check_scope_stack_empty]

def _pelican_new_page(generator):
    # Set a dummy page referrer path so :ref-prefixes: works in Pelican as well
    # TODO: any chance this could be made non-crappy?
    global current_referer_path
    assert not current_referer_path or len(current_referer_path) == 1 and current_referer_path[0][0].name == 'PAGE'
    type = Empty() # We don't have the EntryType enum, so fake it
    type.name = 'PAGE'
    current_referer_path = [(type, '')]

def _pelican_configure(pelicanobj):

    # For backwards compatibility, the input directory is pelican's CWD
    parse_intersphinx_inventories(input=os.getcwd(),
         inventories=pelicanobj.settings.get('M_SPHINX_INVENTORIES', []))

def register(): # for Pelican
    from pelican import signals

    rst.roles.register_local_role('ref', ref)

    signals.initialized.connect(_pelican_configure)
    signals.article_generator_preread.connect(_pelican_new_page)
    signals.page_generator_preread.connect(_pelican_new_page)

def pretty_print_intersphinx_inventory(file):
    return ''.join([
        # Sphinx inventory version 2
        file.readline().decode('utf-8'),
        # Project and version
        file.readline().decode('utf-8'),
        file.readline().decode('utf-8'),
        # Zlib compression line
        file.readline().decode('utf-8'),
        # The rest, zlib-compressed
        zlib.decompress(file.read()).decode('utf-8')
    ])

if __name__ == '__main__': # pragma: no cover
    parser = argparse.ArgumentParser()
    parser.add_argument('file', help="inventory file to print")
    parser.add_argument('--raw', help="show raw content", action='store_true')
    parser.add_argument('--types', help="list all type keys", action='store_true')
    parser.add_argument('--repack', help="repack raw content back into a compressed file", action='store_true')
    args = parser.parse_args()

    if args.types:
        with open(args.file, 'rb') as f:
            inventory = {}
            parse_intersphinx_inventory(f, '', inventory, [])
            print(inventory.keys())
    elif args.repack:
        with open(args.file, 'rb') as f:
            # Sphinx inventory version 2
            sys.stdout.buffer.write(f.readline())
            # Project and version
            sys.stdout.buffer.write(f.readline())
            sys.stdout.buffer.write(f.readline())
            # Zlib compression line
            sys.stdout.buffer.write(f.readline())
            # The rest, zlib-compressed
            sys.stdout.buffer.write(zlib.compress(f.read()))
    else:
        with open(args.file, 'rb') as f:
            print(pretty_print_intersphinx_inventory(f))
