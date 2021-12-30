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

filters_pre = None
filters_post = None

def _highlight(code, language, options, *, is_block, filters=[]):
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

    # Pygments wants the underscored option
    if 'hl-lines' in options:
        options['hl_lines'] = options['hl-lines']
        del options['hl-lines']

    if isinstance(lexer, ansilexer.AnsiLexer):
        formatter = ansilexer.HtmlAnsiFormatter(**options)
    else:
        formatter = HtmlFormatter(nowrap=True, **options)

    global filters_pre
    # First apply local pre filters, if any
    for filter in filters:
        f = filters_pre.get((lexer.name, filter))
        if f: code = f(code)
    # Then a global pre filter, if any
    f = filters_pre.get(lexer.name)
    if f: code = f(code)

    highlighted = highlight(code, lexer, formatter).rstrip()
    # Strip whitespace around if inline code, strip only trailing whitespace if
    # a block
    if not is_block: highlighted = highlighted.lstrip()

    global filters_post
    # First apply local post filters, if any
    for filter in filters:
        f = filters_post.get((lexer.name, filter))
        if f: highlighted = f(highlighted)
    # Then a global post filter, if any
    f = filters_post.get(lexer.name)
    if f: highlighted = f(highlighted)

    return class_, highlighted

class Code(Directive):
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = True
    option_spec = {
        'hl-lines': directives.unchanged,
        # Legacy alias to hl-lines (I hate underscores)
        'hl_lines': directives.unchanged,
        'class': directives.class_option,
        'filters': directives.unchanged
    }
    has_content = True

    def run(self):
        self.assert_has_content()

        set_classes(self.options)
        classes = []
        if 'classes' in self.options:
            classes += self.options['classes']
            del self.options['classes']

        # Legacy alias to hl-lines
        if 'hl_lines' in self.options:
            self.options['hl-lines'] = self.options['hl_lines']
            del self.options['hl_lines']

        filters = self.options.pop('filters', '').split()

        class_, highlighted = _highlight('\n'.join(self.content), self.arguments[0], self.options, is_block=True, filters=filters)
        classes += [class_]

        content = nodes.raw('', highlighted, format='html')
        pre = nodes.literal_block('', classes=classes)
        pre.append(content)
        return [pre]

class Include(docutils.parsers.rst.directives.misc.Include):
    option_spec = {
        'code': directives.unchanged,
        'tab-width': int,
        'start-line': int,
        'end-line': int,
        'start-after': directives.unchanged_required,
        'start-on': directives.unchanged_required,
        'end-before': directives.unchanged,
        'strip-prefix': directives.unchanged,
        'class': directives.class_option,
        'filters': directives.unchanged,
        'hl-lines': directives.unchanged
    }

    def run(self):
        """
        Verbatim copy of docutils.parsers.rst.directives.misc.Include.run()
        that just calls to our Code instead of builtin CodeBlock, is without
        the rarely useful :encoding:, :literal: and :name: options and adds
        support for :start-on:, empty :end-before: and :strip-prefix:.
        """
        source = self.state_machine.input_lines.source(
            self.lineno - self.state_machine.input_offset - 1)
        source_dir = os.path.dirname(os.path.abspath(source))
        path = directives.path(self.arguments[0])
        if path.startswith('<') and path.endswith('>'):
            path = os.path.join(self.standard_include_path, path[1:-1])
        path = os.path.normpath(os.path.join(source_dir, path))
        path = utils.relative_path(None, path)
        path = nodes.reprunicode(path)
        e_handler=self.state.document.settings.input_encoding_error_handler
        tab_width = self.options.get(
            'tab-width', self.state.document.settings.tab_width)
        try:
            self.state.document.settings.record_dependencies.add(path)
            include_file = io.FileInput(source_path=path,
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
        # Compared to start-after, this includes the matched line
        on_text = self.options.get('start-on', None)
        if on_text:
            on_index = rawtext.find('\n' + on_text)
            if on_index < 0:
                raise self.severe('Problem with "start-on" option of "%s" '
                                  'directive:\nText not found.' % self.name)
            rawtext = rawtext[on_index:]
        # Compared to builtin include directive, the end-before can be empty,
        # in which case it simply matches the first empty line (which is
        # usually end of the code block)
        before_text = self.options.get('end-before', None)
        if before_text is not None:
            # skip content in rawtext after *and incl.* a matching text
            if before_text == '':
                before_index = rawtext.find('\n\n')
            else:
                before_index = rawtext.find(before_text)
            if before_index < 0:
                raise self.severe('Problem with "end-before" option of "%s" '
                                  'directive:\nText not found.' % self.name)
            rawtext = rawtext[:before_index]

        include_lines = statemachine.string2lines(rawtext, tab_width,
                                                  convert_whitespace=True)

        # Strip a common prefix from all lines. Useful for example when
        # including a reST snippet that's embedded in a comment, or cutting
        # away excessive indentation. Can be wrapped in quotes in order to
        # avoid trailing whitespace in reST markup.
        if 'strip-prefix' in self.options and self.options['strip-prefix']:
            prefix = self.options['strip-prefix']
            if prefix[0] == prefix[-1] and prefix[0] in ['\'', '"']:
                prefix = prefix[1:-1]
            for i, line in enumerate(include_lines):
                if line.startswith(prefix): include_lines[i] = line[len(prefix):]
                # Strip the prefix also if the line is just the prefix alone,
                # with trailing whitespace removed
                elif line.rstrip() == prefix.rstrip(): include_lines[i] = ''

        if 'code' in self.options:
            self.options['source'] = path
            # Don't convert tabs to spaces, if `tab_width` is negative:
            if tab_width < 0:
                include_lines = rawtext.splitlines()
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
        self.state_machine.insert_input(include_lines, path)
        return []

def code(role, rawtext, text, lineno, inliner, options={}, content=[]):
    # In order to properly preserve backslashes (well, and backticks)
    text = rawtext[rawtext.find('`') + 1:rawtext.rfind('`')]

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

    filters = options.pop('filters', '').split()

    class_, highlighted = _highlight(utils.unescape(text), language, options, is_block=False, filters=filters)
    classes += [class_]

    content = nodes.raw('', highlighted, format='html')
    node = nodes.literal(rawtext, '', classes=classes, **options)
    node.append(content)
    return [node], []

code.options = {'class': directives.class_option,
                'language': directives.unchanged,
                'filters': directives.unchanged}

def register_mcss(mcss_settings, **kwargs):
    rst.directives.register_directive('code', Code)
    rst.directives.register_directive('include', Include)
    rst.roles.register_canonical_role('code', code)

    # These two are builtin aliases to .. code:: in docutils:
    # https://github.com/docutils-mirror/docutils/blob/e88c5fb08d5cdfa8b4ac1020dd6f7177778d5990/docutils/parsers/rst/languages/en.py#L22-L24
    # Since a lot of existing markup (especially coming from Sphinx) uses
    # .. code-block:: and since there's no reason for .. code-block:: /
    # .. sourcecode:: to behave like unpatched docutils, let's add those too:
    rst.directives.register_directive('code-block', Code)
    rst.directives.register_directive('sourcecode', Code)

    global filters_pre, filters_post
    filters_pre = mcss_settings.get('M_CODE_FILTERS_PRE', {})
    filters_post = mcss_settings.get('M_CODE_FILTERS_POST', {})

# Below is only Pelican-specific functionality. If Pelican is not found, these
# do nothing.

def _pelican_configure(pelicanobj):
    settings = {}
    for key in ['M_CODE_FILTERS_PRE', 'M_CODE_FILTERS_POST']:
        if key in pelicanobj.settings: settings[key] = pelicanobj.settings[key]

    register_mcss(mcss_settings=settings)

def register(): # for Pelican
    from pelican import signals

    signals.initialized.connect(_pelican_configure)
