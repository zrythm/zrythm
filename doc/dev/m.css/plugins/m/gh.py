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

def gh_internal(account, ref, title, link):
    base_url = "https://github.com/{}/{}/{}/{}"
    if '#' in ref:
        project, _, issue = ref.partition('#')
        url = base_url.format(account, project, "issues", issue)
        if not title: title = link
    elif '@' in ref:
        project, _, commit = ref.partition('@')
        url = base_url.format(account, project, "commit", commit)
        if not title: title = account + "/" + project + "@" + commit[0:7]
    elif '$' in ref:
        project, _, branch = ref.partition('$')
        url = base_url.format(account, project, "tree", branch)
        if not title: title = url
    elif '^' in ref:
        project, _, branch = ref.partition('^')
        url = base_url.format(account, project, "releases/tag", branch)
        if not title: title = url
    else:
        url = "https://github.com/{}/{}".format(account, ref)
        if not title:
            # if simple profile link, no need to expand to full URL
            title = link if not '/' in ref else url

    return title, url

def gh(name, rawtext, text, lineno, inliner, options={}, content=[]):
    title, link = parse_link(text)
    account, _, ref = link.partition('/')
    if not ref:
        url = "https://github.com/{}".format(account)
        if not title: title = "@{}".format(account)
    else:
        title, url = gh_internal(account, ref, title, link)

    set_classes(options)
    node = nodes.reference(rawtext, title, refuri=url, **options)
    return [node], []

def register_mcss(**kwargs):
    rst.roles.register_local_role('gh', gh)

# Below is only Pelican-specific functionality. If Pelican is not found, these
# do nothing.

register = register_mcss # for Pelican
