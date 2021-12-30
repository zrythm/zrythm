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

import copy
import logging
import os.path
import re

from docutils.writers.html5_polyglot import HTMLTranslator
from docutils.transforms import Transform
import docutils
from docutils import nodes, utils
from docutils.utils import smartquotes
from urllib.parse import urljoin

try:
    import pyphen
except ImportError:
    pyphen = None

logger = logging.getLogger(__name__)

default_settings = {
    # Language used for hyphenation and smart quotes
    'M_HTMLSANITY_LANGUAGE': 'en',
    # Extra docutils settings
    'M_HTMLSANITY_DOCUTILS_SETTINGS': {},
    # Whether hyphenation is enabled
    'M_HTMLSANITY_HYPHENATION': False,
    # Whether smart quotes are enabled
    'M_HTMLSANITY_SMART_QUOTES': False,
    # List of formatted document-level fields (which should get hyphenation
    # and smart quotes applied as well)
    'M_HTMLSANITY_FORMATTED_FIELDS': []
}

settings = None

# Default docutils settings. Some things added in register() / register_mcss(),
# and from the M_HTMLSANITY_DOCUTILS_SETTINGS option.
docutils_settings = {
    'initial_header_level': '2',
    'syntax_highlight': 'short',
    'input_encoding': 'utf-8',
    'halt_level': 2,
    'traceback': True,
    'embed_stylesheet': False
}

words_re = re.compile(r'\w+', re.UNICODE|re.X)

def extract_document_language(document):
    # Take the one from settings as default
    language = document.settings.language_code

    # Then try to find the :lang: metadata option
    for field in document.traverse(nodes.field):
        assert isinstance(field[0], nodes.field_name)
        assert isinstance(field[1], nodes.field_body)
        # field_body -> paragraph -> text
        if field[0][0] == 'lang': return str(field[1][0][0])

    return language

def can_apply_typography(txtnode):
    # Exclude:
    #  - literals and spans inside literals
    #  - raw code (such as SVG)
    #  - field names
    #  - bibliographic elements (author, date, ... fields)
    #  - links with title that's the same as URL (or e-mail)
    if isinstance(txtnode.parent, nodes.literal) or \
       isinstance(txtnode.parent.parent, nodes.literal) or \
       isinstance(txtnode.parent, nodes.raw) or \
       isinstance(txtnode.parent, nodes.field_name) or \
       isinstance(txtnode.parent, nodes.Bibliographic) or \
       (isinstance(txtnode.parent, nodes.reference) and
            (txtnode.astext() == txtnode.parent.get('refuri', '') or 'mailto:' + txtnode.astext() == txtnode.parent.get('refuri', ''))):
        return False

    # From fields include only the ones that are in M_HTMLSANITY_FORMATTED_FIELDS
    if isinstance(txtnode.parent.parent, nodes.field_body):
        field_name_index = txtnode.parent.parent.parent.first_child_matching_class(nodes.field_name)
        if txtnode.parent.parent.parent[field_name_index][0] in settings['M_HTMLSANITY_FORMATTED_FIELDS']:
            return True
        return False

    return True

class SmartQuotes(docutils.transforms.universal.SmartQuotes):
    """Smart quote transform

    Copy-paste of docutils builtin smart_quotes feature, patched to *not*
    replace quotes etc. in inline code blocks.  Original source from
    https://github.com/docutils-mirror/docutils/blob/e88c5fb08d5cdfa8b4ac1020dd6f7177778d5990/docutils/transforms/universal.py#L207

    Contrary to the builtin, this is controlled via HTMLSANITY_SMART_QUOTES
    instead of smart_quotes in DOCUTILS_SETTINGS, so the original smart quotes
    implementation is not executed.
    """

    def apply(self):
        # We are using our own config variable instead of
        # self.document.settings.smart_quotes in order to avoid the builtin
        # SmartQuotes to be executed as well
        if not settings['M_HTMLSANITY_SMART_QUOTES']:
            return
        try:
            alternative = settings['M_HTMLSANITY_SMART_QUOTES'].startswith('alt')
        except AttributeError:
            alternative = False
        # print repr(alternative)

        document_language = extract_document_language(self.document)

        # "Educate" quotes in normal text. Handle each block of text
        # (TextElement node) as a unit to keep context around inline nodes:
        for node in self.document.traverse(nodes.TextElement):
            # skip preformatted text blocks and special elements:
            if isinstance(node, (nodes.FixedTextElement, nodes.Special)):
                continue
            # nested TextElements are not "block-level" elements:
            if isinstance(node.parent, nodes.TextElement):
                continue

            # list of text nodes in the "text block":
            # Patched here to exclude more stuff.
            txtnodes = []
            for txtnode in node.traverse(nodes.Text):
                if not can_apply_typography(txtnode): continue
                # Don't convert -- in option strings
                if isinstance(txtnode.parent, nodes.option_string): continue

                txtnodes += [txtnode]

            # language: use typographical quotes for language "lang"
            lang = node.get_language_code(document_language)
            # use alternative form if `smart-quotes` setting starts with "alt":
            if alternative:
                if '-x-altquot' in lang:
                    lang = lang.replace('-x-altquot', '')
                else:
                    lang += '-x-altquot'
            # drop subtags missing in quotes:
            for tag in utils.normalize_language_tag(lang):
                if tag in smartquotes.smartchars.quotes:
                    lang = tag
                    break
            else: # language not supported: (keep ASCII quotes)
                lang = ''

            # Iterator educating quotes in plain text:
            # '2': set all, using old school en- and em- dash shortcuts
            teacher = smartquotes.educate_tokens(self.get_tokens(txtnodes),
                                                 attr='2', language=lang)

            for txtnode, newtext in zip(txtnodes, teacher):
                txtnode.parent.replace(txtnode, nodes.Text(newtext))

            self.unsupported_languages = set() # reset

class Pyphen(Transform):
    """Hyphenation using pyphen

    Enabled or disabled using HTMLSANITY_HYPHENATION boolean option, defaulting
    to ``False``.
    """

    # Max Docutils priority is 990, be sure that this is applied at the very
    # last
    default_priority = 991

    def __init__(self, document, startnode):
        Transform.__init__(self, document, startnode=startnode)

    def apply(self):
        if not settings['M_HTMLSANITY_HYPHENATION']:
            return

        document_language = extract_document_language(self.document)

        pyphen_for_lang = {}

        # Go through all text words and hyphenate them
        for node in self.document.traverse(nodes.TextElement):
            # Skip preformatted text blocks and special elements
            if isinstance(node, (nodes.FixedTextElement, nodes.Special)):
                continue

            for txtnode in node.traverse(nodes.Text):
                if not can_apply_typography(txtnode): continue

                # Don't hyphenate document title. Not part of
                # can_apply_typography() because we *do* want smart quotes for
                # a document title.
                if isinstance(txtnode.parent, nodes.title): continue

                # Useful for debugging, don't remove ;)
                #print(repr(txtnode.parent), repr(txtnode.parent.parent), repr(txtnode.parent.parent.parent))

                # Proper language-dependent hyphenation. Can't be done for
                # `node` as a paragraph can consist of more than one language.
                lang = txtnode.parent.get_language_code(document_language)

                # Create new Pyphen object for given lang, if not yet cached.
                # I'm assuming this is faster than recreating the instance for
                # every text node
                if lang not in pyphen_for_lang:
                    if not pyphen or lang not in pyphen.LANGUAGES: continue
                    pyphen_for_lang[lang] = pyphen.Pyphen(lang=lang)

                txtnode.parent.replace(txtnode, nodes.Text(words_re.sub(lambda m: pyphen_for_lang[lang].inserted(m.group(0), '\u00AD'), txtnode.astext())))

class SaneHtmlTranslator(HTMLTranslator):
    """Sane HTML translator

    Patched version of docutils builtin HTML5 translator, improving the output
    further.
    """

    # Overrides the ones from docutils HTML5 writer to enable the soft hyphen
    # character
    special_characters = {ord('&'): '&amp;',
                          ord('<'): '&lt;',
                          ord('"'): '&quot;',
                          ord('>'): '&gt;',
                          ord('@'): '&#64;', # may thwart address harvesters
                          ord('\u00AD'): '&shy;'}

    def __init__(self, document):
        HTMLTranslator.__init__(self, document)

        # There's a minor difference between docutils 0.13 and 0.14 that breaks
        # stuff. Monkey-patch it here.
        if not hasattr(self, 'in_word_wrap_point'):
            self.in_word_wrap_point = HTMLTranslator.sollbruchstelle

    # Somehow this does the trick and removes docinfo from the body. Was
    # present in the HTML4 translator but not in the HTML5 one, so copying it
    # verbatim over
    def visit_docinfo(self, node):
        self.context.append(len(self.body))
        self.body.append(self.starttag(node, 'table',
                                       CLASS='docinfo',
                                       frame="void", rules="none"))
        self.body.append('<col class="docinfo-name" />\n'
                         '<col class="docinfo-content" />\n'
                         '<tbody valign="top">\n')
        self.in_docinfo = True

    def depart_docinfo(self, node):
        self.body.append('</tbody>\n</table>\n')
        self.in_docinfo = False
        start = self.context.pop()
        self.docinfo = self.body[start:]
        self.body = []

    # Have <abbr> properly with title
    def visit_abbreviation(self, node):
        attrs = {}
        if node.hasattr('title'):
            attrs['title'] = node['title']
        self.body.append(self.starttag(node, 'abbr', '', **attrs))

    def depart_abbreviation(self, node):
        self.body.append('</abbr>')

    # Convert outdated width/height attributes to CSS styles, handle the scale
    # directly inside m.images; don't put URI in alt text.
    def visit_image(self, node):
        atts = {}
        uri = node['uri']
        ext = os.path.splitext(uri)[1].lower()
        atts['src'] = uri
        if 'alt' in node: atts['alt'] = node['alt']
        style = []
        if node.get('width'):
            style += ['width: {}'.format(node['width'])]
        if node.get('height'):
            style += ['height: {}'.format(node['height'])]
        if style:
            atts['style'] = '; '.join(style)
        if (isinstance(node.parent, nodes.TextElement) or
            (isinstance(node.parent, nodes.reference) and
             not isinstance(node.parent.parent, nodes.TextElement))):
            # Inline context or surrounded by <a>...</a>.
            suffix = ''
        else:
            suffix = '\n'
        self.body.append(self.emptytag(node, 'img', suffix, **atts))

    def depart_image(self, node):
        pass

    # Use HTML5 <section> tag for sections (instead of <div class="section">)
    def visit_section(self, node):
        self.section_level += 1
        self.body.append(
            self.starttag(node, 'section'))

    def depart_section(self, node):
        self.section_level -= 1
        self.body.append('</section>\n')

    # Legend inside figure -- print as <span> (instead of <div class="legend">,
    # as that's not valid inside HTML5 <figure> element)
    def visit_legend(self, node):
        self.body.append(self.starttag(node, 'span'))

    def depart_legend(self, node):
        self.body.append('</span>\n')

    # Literal -- print as <code> (instead of some <span>)
    def visit_literal(self, node):
        self.body.append(self.starttag(node, 'code', ''))

    def depart_literal(self, node):
        self.body.append('</code>')

    # Literal block -- use <pre> without nested <code> and useless classes
    def visit_literal_block(self, node):
        self.body.append(self.starttag(node, 'pre', ''))

    def depart_literal_block(self, node):
        self.body.append('</pre>\n')

    # Anchor -- drop the useless classes
    def visit_reference(self, node):
        atts = {}
        if 'refuri' in node:
            atts['href'] = node['refuri']
            if ( self.settings.cloak_email_addresses
                 and atts['href'].startswith('mailto:')):
                atts['href'] = self.cloak_mailto(atts['href'])
                self.in_mailto = True
        else:
            assert 'refid' in node, \
                   'References must have "refuri" or "refid" attribute.'
            atts['href'] = '#' + node['refid']
        # why?!?!
        #if not isinstance(node.parent, nodes.TextElement):
            #assert len(node) == 1 and isinstance(node[0], nodes.image)

        # If the link is a plain URL without explicitly specified title, apply
        # m-link-wrap so it doesn't leak out of the view on narrow screens.
        # This can be disabled by explicitly providing the URL also as a title
        # --- then the node will have a name attribute and we'll skip in that
        # case.
        if len(node.children) == 1 and isinstance(node.children[0], nodes.Text) and 'name' not in node and 'refuri' in node and node.children[0] == node['refuri']:
            node['classes'] += ['m-link-wrap']

        self.body.append(self.starttag(node, 'a', '', **atts))

    def depart_reference(self, node):
        self.body.append('</a>')
        if not isinstance(node.parent, nodes.TextElement):
            self.body.append('\n')
        self.in_mailto = False

    # Use <aside> instead of a meaningless <div>
    def visit_topic(self, node):
        self.body.append(self.starttag(node, 'aside'))
        self.topic_classes = node['classes']

    def depart_topic(self, node):
        self.body.append('</aside>\n')
        self.topic_classes = []

    # Don't use <colgroup> or other shit in tables
    def visit_colspec(self, node):
        self.colspecs.append(node)
        # "stubs" list is an attribute of the tgroup element:
        node.parent.stubs.append(node.attributes.get('stub'))

    def depart_colspec(self, node):
        # write out <colgroup> when all colspecs are processed
        if isinstance(node.next_node(descend=False, siblings=True),
                      nodes.colspec):
            return
        if 'colwidths-auto' in node.parent.parent['classes'] or (
            'colwidths-auto' in self.settings.table_style and
            ('colwidths-given' not in node.parent.parent['classes'])):
            return

    # Verbatim copied, removing the 'head' class from <th>
    def visit_entry(self, node):
        atts = {'class': []}
        if node.parent.parent.parent.stubs[node.parent.column]:
            # "stubs" list is an attribute of the tgroup element
            atts['class'].append('stub')
        if atts['class'] or isinstance(node.parent.parent, nodes.thead):
            tagname = 'th'
            atts['class'] = ' '.join(atts['class'])
        else:
            tagname = 'td'
            del atts['class']
        node.parent.column += 1
        if 'morerows' in node:
            atts['rowspan'] = node['morerows'] + 1
        if 'morecols' in node:
            atts['colspan'] = node['morecols'] + 1
            node.parent.column += node['morecols']
        self.body.append(self.starttag(node, tagname, '', **atts))
        self.context.append('</%s>\n' % tagname.lower())

    # Don't put comments into the HTML output
    def visit_comment(self, node,
                      sub=re.compile('-(?=-)').sub):
        raise nodes.SkipNode

    # Containers don't need those stupid "docutils" class names
    def visit_container(self, node):
        atts = {}
        self.body.append(self.starttag(node, 'div', **atts))

    def depart_container(self, node):
        self.body.append('</div>\n')

    # Use HTML5 <figure> element for figures
    def visit_figure(self, node):
        atts = {}
        if node.get('id'):
            atts['ids'] = [node['id']]
        style = []
        if node.get('width'):
            style += ['width: {}'.format(node['width'])]
        if style:
            atts['style'] = '; '.join(style)
        self.body.append(self.starttag(node, 'figure', **atts))

    def depart_figure(self, node):
        self.body.append('</figure>\n')

    def visit_caption(self, node):
        self.body.append(self.starttag(node, 'figcaption', ''))

    def depart_caption(self, node):
        self.body.append('</figcaption>\n')

    # Line blocks are <p> with lines separated using simple <br />. No need for
    # nested <div>s.
    def visit_line(self, node):
        pass

    def depart_line(self, node):
        self.body.append('<br />\n')

    # Footnote list. Replacing the classes with just .m-footnote.
    def visit_footnote(self, node):
        if not self.in_footnote_list:
            self.body.append('<dl class="m-footnote">\n')
            self.in_footnote_list = True

    def depart_footnote(self, node):
        self.body.append('</dd>\n')
        if not isinstance(node.next_node(descend=False, siblings=True),
                          nodes.footnote):
            self.body.append('</dl>\n')
            self.in_footnote_list = False

    # Footnote reference
    def visit_footnote_reference(self, node):
        href = '#' + node['refid']
        self.body.append(self.starttag(node, 'a', '', CLASS='m-footnote', href=href))

    def depart_footnote_reference(self, node):
        self.body.append('</a>')

    # Footnote and citation labels
    def visit_label(self, node):
        self.body.append(self.starttag(node.parent, 'dt', ''))

    def depart_label(self, node):
        if self.settings.footnote_backlinks:
            backrefs = node.parent['backrefs']
            if len(backrefs) == 1:
                self.body.append('</a>')
        self.body.append('.</dt>\n<dd><span class="m-footnote">')
        if len(backrefs) == 1:
            self.body.append('<a href="#{}">^</a>'.format(backrefs[0]))
        else:
            self.body.append('^ ')
            self.body.append(format(' '.join('<a href="#{}">{}</a>'.format(ref, chr(ord('a') + i)) for i, ref in enumerate(backrefs))))
        self.body.append('</span> ')

    def visit_line_block(self, node):
        self.body.append(self.starttag(node, 'p'))

    def depart_line_block(self, node):
        self.body.append('</p>\n')

    # Copied from the HTML4 translator, somehow not present in the HTML5 one.
    # Not having this generates *a lot* of <p> tags everywhere.
    def should_be_compact_paragraph(self, node):
        """
        Determine if the <p> tags around paragraph ``node`` can be omitted.
        """
        if (isinstance(node.parent, nodes.document) or
            isinstance(node.parent, nodes.compound)):
            # Never compact paragraphs in document or compound
            return False
        for key, value in node.attlist():
            if (node.is_not_default(key) and
                not (key == 'classes' and value in
                     ([], ['first'], ['last'], ['first', 'last']))):
                # Attribute which needs to survive.
                return False
        first = isinstance(node.parent[0], nodes.label) # skip label
        for child in node.parent.children[first:]:
            # only first paragraph can be compact
            if isinstance(child, nodes.Invisible):
                continue
            if child is node:
                break
            return False
        parent_length = len([n for n in node.parent if not isinstance(
            n, (nodes.Invisible, nodes.label))])
        if ( self.compact_simple
             or self.compact_field_list
             or self.compact_p and parent_length == 1):
            return True
        return False

    def visit_paragraph(self, node):
        if self.should_be_compact_paragraph(node):
            self.context.append('')
        else:
            self.body.append(self.starttag(node, 'p', ''))
            self.context.append('</p>\n')

    def depart_paragraph(self, node):
        self.body.append(self.context.pop())

    # Titles in topics should be <h3>
    def visit_title(self, node):
        """Only 6 section levels are supported by HTML."""
        check_id = 0  # TODO: is this a bool (False) or a counter?
        close_tag = '</p>\n'
        if isinstance(node.parent, nodes.topic):
            self.body.append(
                  self.starttag(node, 'h3', ''))
            close_tag = '</h3>\n'
        elif isinstance(node.parent, nodes.sidebar):
            self.body.append(
                  self.starttag(node, 'p', '', CLASS='sidebar-title'))
        elif isinstance(node.parent, nodes.Admonition):
            self.body.append(
                  self.starttag(node, 'p', '', CLASS='admonition-title'))
        elif isinstance(node.parent, nodes.table):
            self.body.append(
                  self.starttag(node, 'caption', ''))
            close_tag = '</caption>\n'
        elif isinstance(node.parent, nodes.document):
            self.body.append(self.starttag(node, 'h1', '', CLASS='title'))
            close_tag = '</h1>\n'
            self.in_document_title = len(self.body)
        else:
            assert isinstance(node.parent, nodes.section)
            h_level = self.section_level + self.initial_header_level - 1
            atts = {}
            if (len(node.parent) >= 2 and
                isinstance(node.parent[1], nodes.subtitle)):
                atts['CLASS'] = 'with-subtitle'
            self.body.append(
                  self.starttag(node, 'h%s' % h_level, '', **atts))
            atts = {}
            if node.hasattr('refid'):
                atts['href'] = '#' + node['refid']
            if atts:
                self.body.append(self.starttag({}, 'a', '', **atts))
                close_tag = '</a></h%s>\n' % (h_level)
            else:
                close_tag = '</h%s>\n' % (h_level)
        self.context.append(close_tag)

    def depart_title(self, node):
        self.body.append(self.context.pop())
        if self.in_document_title:
            self.title = self.body[self.in_document_title:-1]
            self.in_document_title = 0
            self.body_pre_docinfo.extend(self.body)
            self.html_title.extend(self.body)
            del self.body[:]

    # <ul>, <ol>, <dl> -- verbatim copied, removing "simple" class. For <ol>
    # also removing the enumtype
    def visit_bullet_list(self, node):
        atts = {}
        old_compact_simple = self.compact_simple
        self.context.append((self.compact_simple, self.compact_p))
        self.compact_p = None
        self.compact_simple = self.is_compactable(node)
        self.body.append(self.starttag(node, 'ul', **atts))

    def depart_bullet_list(self, node):
        self.compact_simple, self.compact_p = self.context.pop()
        self.body.append('</ul>\n')

    def visit_enumerated_list(self, node):
        atts = {}
        if 'start' in node:
            atts['start'] = node['start']
        self.body.append(self.starttag(node, 'ol', **atts))

    def depart_enumerated_list(self, node):
        self.body.append('</ol>\n')

    def visit_definition_list(self, node):
        classes = node.setdefault('classes', [])
        self.body.append(self.starttag(node, 'dl'))

    def depart_definition_list(self, node):
        self.body.append('</dl>\n')

    # no class="docutils" in <hr>
    def visit_transition(self, node):
        self.body.append(self.emptytag(node, 'hr'))

    def depart_transition(self, node):
        pass

class _SaneFieldBodyTranslator(SaneHtmlTranslator):
    """
    Copy of _FieldBodyTranslator with the only difference being inherited from
    SaneHtmlTranslator instead of HTMLTranslator
    """

    def __init__(self, document):
        SaneHtmlTranslator.__init__(self, document)

    # Overriding the function in SaneHtmlTranslator, in addition never
    # compacting paragraphs directly in field bodies (such as article summary
    # or page footer) unless explicitly told it so. The sad thing is that the
    # Pelican theme currently always expects the summaries to be wrapped in
    # <p>, while the Python docs expect exactly the other case.
    def should_be_compact_paragraph(self, node):
        if isinstance(node.parent, nodes.field_body) and not self.compact_field_list:
            return False
        return SaneHtmlTranslator.should_be_compact_paragraph(self, node)

    def astext(self):
        return ''.join(self.body)

    # If this wouldn't be here, the output would have <dd> around. Not useful.
    def visit_field_body(self, node):
        pass

    def depart_field_body(self, node):
        pass

class SaneHtmlWriter(docutils.writers.html5_polyglot.Writer):
    def __init__(self):
        docutils.writers.html5_polyglot.Writer.__init__(self)

        self.translator_class = SaneHtmlTranslator

    def get_transforms(self):
        return docutils.writers.html5_polyglot.Writer.get_transforms(self) + [SmartQuotes, Pyphen]

def render_rst(value):
    pub = docutils.core.Publisher(
        writer=SaneHtmlWriter(),
        source_class=docutils.io.StringInput,
        destination_class=docutils.io.StringOutput)
    pub.set_components('standalone', 'restructuredtext', 'html')
    pub.writer.translator_class = _SaneFieldBodyTranslator
    pub.process_programmatic_settings(None, docutils_settings, None)
    pub.set_source(source=value)
    pub.publish(enable_exit_status=True)
    return pub.writer.parts.get('body').strip()

def rtrim(value):
    return value.rstrip()

def hyphenate(value, enable=None, lang=None):
    if enable is None: enable = settings['M_HTMLSANITY_HYPHENATION']
    if lang is None: lang = settings['M_HTMLSANITY_LANGUAGE']
    if not enable or not pyphen: return value
    pyphen_ = pyphen.Pyphen(lang=lang)
    return words_re.sub(lambda m: pyphen_.inserted(m.group(0), '&shy;'), str(value))

def dehyphenate(value, enable=None):
    if enable is None: enable = settings['M_HTMLSANITY_HYPHENATION']
    if not enable: return value
    return value.replace('&shy;', '')

def register_mcss(mcss_settings, jinja_environment, **kwargs):
    global default_settings, settings
    settings = copy.deepcopy(default_settings)
    for key in settings.keys():
        if key in mcss_settings: settings[key] = mcss_settings[key]
    docutils_settings['language_code'] = settings['M_HTMLSANITY_LANGUAGE']
    docutils_settings.update(settings['M_HTMLSANITY_DOCUTILS_SETTINGS'])

    jinja_environment.filters['rtrim'] = rtrim
    jinja_environment.filters['render_rst'] = render_rst
    jinja_environment.filters['hyphenate'] = hyphenate
    jinja_environment.filters['dehyphenate'] = dehyphenate

# Below is only Pelican-specific functionality. If Pelican is not found, these
# do nothing.
try:
    from pelican import signals
    from pelican.readers import RstReader

    class PelicanSaneRstReader(RstReader):
        writer_class = SaneHtmlWriter
        field_body_translator_class = _SaneFieldBodyTranslator

except ImportError:
    pass

pelican_settings = {}

def pelican_expand_link(link, content):
    link_regex = r"""^
        (?P<markup>)(?P<quote>)
        (?P<path>{0}(?P<value>.*))
        $""".format(pelican_settings['INTRASITE_LINK_REGEX'])
    links = re.compile(link_regex, re.X)
    return links.sub(
        lambda m: content._link_replacer(content.get_siteurl(), m),
        link)

def pelican_expand_links(text, content):
    # TODO: fields that are in FORMATTED_FIELDS are already expanded, but that
    # requires extra work on user side. Ideal would be to handle that all on
    # template side, so keeping this one for the time when we can replace
    # FORMATTED_FIELDS with render_rst as well.
    return content._update_content(text, content.get_siteurl())

# To be consistent with both what Pelican does now with '/'.join(SITEURL, url)
# and with https://github.com/getpelican/pelican/pull/2196. Keep consistent
# with m.alias.
def pelican_format_siteurl(url):
    return urljoin(pelican_settings['SITEURL'] + ('/' if not pelican_settings['SITEURL'].endswith('/') else ''), url)

def _pelican_configure(pelicanobj):
    pelicanobj.settings['JINJA_FILTERS']['rtrim'] = rtrim
    pelicanobj.settings['JINJA_FILTERS']['render_rst'] = render_rst
    pelicanobj.settings['JINJA_FILTERS']['expand_link'] = pelican_expand_link
    pelicanobj.settings['JINJA_FILTERS']['expand_links'] = pelican_expand_links
    pelicanobj.settings['JINJA_FILTERS']['format_siteurl'] = pelican_format_siteurl
    pelicanobj.settings['JINJA_FILTERS']['hyphenate'] = hyphenate
    pelicanobj.settings['JINJA_FILTERS']['dehyphenate'] = dehyphenate

    # Map the setting key names from Pelican's own
    global default_settings, settings
    settings = copy.deepcopy(default_settings)
    for key, pelicankey in [
        ('M_HTMLSANITY_LANGUAGE', 'DEFAULT_LANG'),
        ('M_HTMLSANITY_DOCUTILS_SETTINGS', 'DOCUTILS_SETTINGS'),
        ('M_HTMLSANITY_FORMATTED_FIELDS', 'FORMATTED_FIELDS')]:
        settings[key] = pelicanobj.settings[pelicankey]

    # Settings with the same name both here and in Pelican
    for key in 'M_HTMLSANITY_HYPHENATION', 'M_HTMLSANITY_SMART_QUOTES':
        if key in pelicanobj.settings: settings[key] = pelicanobj.settings[key]

    # Save settings needed only for Pelican-specific functionality
    global pelican_settings
    for key in 'INTRASITE_LINK_REGEX', 'SITEURL':
        pelican_settings[key] = pelicanobj.settings[key]

    # Update the docutils settings using the above
    docutils_settings['language_code'] = settings['M_HTMLSANITY_LANGUAGE']
    docutils_settings.update(settings['M_HTMLSANITY_DOCUTILS_SETTINGS'])

def _pelican_add_reader(readers):
    readers.reader_classes['rst'] = PelicanSaneRstReader

def register(): # for Pelican
    signals.initialized.connect(_pelican_configure)
    signals.readers_init.connect(_pelican_add_reader)
