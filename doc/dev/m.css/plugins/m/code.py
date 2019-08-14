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

import os.path

import docutils
from docutils.parsers import rst
from docutils.parsers.rst.roles import set_classes
from docutils.utils.error_reporting import SafeString, ErrorString, locale_encoding
from docutils.parsers.rst import Directive, directives
import docutils.parsers.rst.directives.misc
from docutils import io, nodes, utils, statemachine

from pygments import highlight
from pygments.formatters import HtmlFormatter
from pygments.lexers import TextLexer, BashSessionLexer, get_lexer_by_name

import logging

logger = logging.getLogger(__name__)

import ansilexer

def _highlight(code, language, options, is_block):
    # Use our own lexer for ANSI
    if language == 'ansi':
        lexer = ansilexer.AnsiLexer()
    else:
        try:
            lexer = get_lexer_by_name(language)
        except ValueError:
            logger.warning("No lexer found for language '{}', code highlighting disabled".format(language))
            lexer = TextLexer()

    if (isinstance(lexer, BashSessionLexer) or
        isinstance(lexer, ansilexer.AnsiLexer)):
        class_ = 'm-console'
    else:
        class_ = 'm-code'

    if isinstance(lexer, ansilexer.AnsiLexer):
        formatter = ansilexer.HtmlAnsiFormatter(**options)
    else:
        formatter = HtmlFormatter(nowrap=True, **options)
    parsed = highlight(code, lexer, formatter).rstrip()
    if not is_block: parsed.lstrip()

    return class_, parsed

class Code(Directive):
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = True
    option_spec = {
        'hl_lines': directives.unchanged,
        'class': directives.class_option
    }
    has_content = True

    def run(self):
        self.assert_has_content()

        set_classes(self.options)
        classes = []
        if 'classes' in self.options:
            classes += self.options['classes']
            del self.options['classes']

        class_, highlighted = _highlight('\n'.join(self.content), self.arguments[0], self.options, is_block=True)
        classes += [class_]

        content = nodes.raw('', highlighted, format='html')
        pre = nodes.literal_block('', classes=classes)
        pre.append(content)
        return [pre]

class Include(docutils.parsers.rst.directives.misc.Include):
    def run(self):
        """
        Verbatim copy of docutils.parsers.rst.directives.misc.Include.run()
        that just calls to our Code instead of builtin CodeBlock but otherwise
        just passes it back to the parent implementation.
        """
        if not 'code' in self.options:
            return docutils.parsers.rst.directives.misc.Include.run(self)

        source = self.state_machine.input_lines.source(
            self.lineno - self.state_machine.input_offset - 1)
        source_dir = os.path.dirname(os.path.abspath(source))
        path = directives.path(self.arguments[0])
        if path.startswith('<') and path.endswith('>'):
            path = os.path.join(self.standard_include_path, path[1:-1])
        path = os.path.normpath(os.path.join(source_dir, path))
        path = utils.relative_path(None, path)
        path = nodes.reprunicode(path)
        encoding = self.options.get(
            'encoding', self.state.document.settings.input_encoding)
        e_handler=self.state.document.settings.input_encoding_error_handler
        tab_width = self.options.get(
            'tab-width', self.state.document.settings.tab_width)
        try:
            self.state.document.settings.record_dependencies.add(path)
            include_file = io.FileInput(source_path=path,
                                        encoding=encoding,
                                        error_handler=e_handler)
        except UnicodeEncodeError as error:
            raise self.severe('Problems with "%s" directive path:\n'
                              'Cannot encode input file path "%s" '
                              '(wrong locale?).' %
                              (self.name, SafeString(path)))
        except IOError as error:
            raise self.severe('Problems with "%s" directive path:\n%s.' %
                      (self.name, ErrorString(error)))
        startline = self.options.get('start-line', None)
        endline = self.options.get('end-line', None)
        try:
            if startline or (endline is not None):
                lines = include_file.readlines()
                rawtext = ''.join(lines[startline:endline])
            else:
                rawtext = include_file.read()
        except UnicodeError as error:
            raise self.severe('Problem with "%s" directive:\n%s' %
                              (self.name, ErrorString(error)))
        # start-after/end-before: no restrictions on newlines in match-text,
        # and no restrictions on matching inside lines vs. line boundaries
        after_text = self.options.get('start-after', None)
        if after_text:
            # skip content in rawtext before *and incl.* a matching text
            after_index = rawtext.find(after_text)
            if after_index < 0:
                raise self.severe('Problem with "start-after" option of "%s" '
                                  'directive:\nText not found.' % self.name)
            rawtext = rawtext[after_index + len(after_text):]
        before_text = self.options.get('end-before', None)
        if before_text:
            # skip content in rawtext after *and incl.* a matching text
            before_index = rawtext.find(before_text)
            if before_index < 0:
                raise self.severe('Problem with "end-before" option of "%s" '
                                  'directive:\nText not found.' % self.name)
            rawtext = rawtext[:before_index]

        include_lines = statemachine.string2lines(rawtext, tab_width,
                                                  convert_whitespace=True)

        self.options['source'] = path
        codeblock = Code(self.name,
                            [self.options.pop('code')], # arguments
                            self.options,
                            include_lines, # content
                            self.lineno,
                            self.content_offset,
                            self.block_text,
                            self.state,
                            self.state_machine)
        return codeblock.run()

def code(role, rawtext, text, lineno, inliner, options={}, content=[]):
    # In order to properly preserve backslashes
    i = rawtext.find('`')
    text = rawtext.split('`')[1]

    set_classes(options)
    classes = []
    if 'classes' in options:
        classes += options['classes']
        del options['classes']

    # If language is not specified, render a simple literal
    if not 'language' in options:
        content = nodes.raw('', utils.unescape(text), format='html')
        node = nodes.literal(rawtext, '', **options)
        node.append(content)
        return [node], []

    language = options['language']
    del options['language']
    # Not sure why language is duplicated in classes?
    if language in classes: classes.remove(language)

    class_, highlighted = _highlight(utils.unescape(text), language, options, is_block=False)
    classes += [class_]

    content = nodes.raw('', highlighted, format='html')
    node = nodes.literal(rawtext, '', classes=classes, **options)
    node.append(content)
    return [node], []

code.options = {'class': directives.class_option,
                'language': directives.unchanged}

def register():
    rst.directives.register_directive('code', Code)
    rst.directives.register_directive('include', Include)
    rst.roles.register_canonical_role('code', code)
