#
#   This file is part of m.css.
#
#   Copyright © 2017, 2018, 2019, 2020 Vladimír Vondruš <mosra@centrum.cz>
#
#   Loosely based on https://github.com/Nitron/pelican-alias,
#   copyright © 2013 Christopher Williams
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
import logging
from urllib.parse import urljoin

logger = logging.getLogger(__name__)

class AliasGenerator:
    def __init__(self, context, settings, path, theme, output_path, *args):
        self.output_path = output_path
        self.context = context
        self.siteurl = settings['SITEURL']

    # Keep consistent with m.htmlsanity
    def format_siteurl(self, url):
        return urljoin(self.siteurl + ('/' if not self.siteurl.endswith('/') else ''), url)

    def generate_output(self, writer):
        for page in self.context['pages'] + self.context['articles']:
            aliases = page.metadata.get('alias', '').strip()
            if not aliases: continue
            for alias in aliases.split('\n'):
                alias = alias.strip()

                # Currently only supporting paths starting with /
                if not alias.startswith('/'): assert False
                path = os.path.join(self.output_path, alias[1:])

                # If path ends with /, write it into index.html
                directory, filename = os.path.split(path)
                if not filename: filename = 'index.html'

                alias_file = os.path.join(directory, filename)
                alias_target = self.format_siteurl(page.url)
                logger.info('m.alias: creating alias {} -> {}'.format(alias_file[len(self.output_path):], alias_target))

                # Write the redirect file
                os.makedirs(directory, exist_ok=True)
                with open(alias_file, 'w') as f:
                    f.write("""<!DOCTYPE html><html><head><meta http-equiv="refresh" content="0;url={}" /></head></html>\n""".format(alias_target))

def register_mcss(**kwargs):
    assert not "This plugin is Pelican-only" # pragma: no cover

# Below is only Pelican-specific functionality. If Pelican is not found, these
# do nothing.

def _pelican_get_generators(generators): return AliasGenerator

def register(): # for Pelican
    from pelican import signals

    signals.get_generators.connect(_pelican_get_generators)
