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
from docutils.parsers.rst import directives
from docutils.parsers.rst.roles import set_classes

import matplotlib as mpl
mpl.use('Agg') # otherwise it will attempt to use X11
import matplotlib.pyplot as plt
import numpy as np
import io

mpl.rcParams['font.size'] = '11'
mpl.rcParams['axes.titlesize'] = '13'

# Plot background. Replaced with .m-plot .m-background later, equivalent to
# --default-filled-background-color
mpl.rcParams['axes.facecolor'] = '#cafe01'

# All of these should match --color, replaced with .m-plot .m-text
mpl.rcParams['text.color'] = '#cafe02'
mpl.rcParams['axes.labelcolor'] = '#cafe02'
mpl.rcParams['xtick.color'] = '#cafe02'
mpl.rcParams['ytick.color'] = '#cafe02'

# no need to have a border around the plot
mpl.rcParams['axes.spines.left'] = False
mpl.rcParams['axes.spines.right'] = False
mpl.rcParams['axes.spines.top'] = False
mpl.rcParams['axes.spines.bottom'] = False

mpl.rcParams['svg.fonttype'] = 'none' # otherwise it renders text to paths
mpl.rcParams['figure.autolayout'] = True # so it relayouts everything to fit

# Gets increased for every graph on a page to (hopefully) ensure unique SVG IDs
mpl.rcParams['svg.hashsalt'] = 0

# Color codes for bars. Keep in sync with latex2svgextra.
style_mapping = {
    'default': '#cafe03',
    'primary': '#cafe04',
    'success': '#cafe05',
    'warning': '#cafe06',
    'danger': '#cafe07',
    'info': '#cafe08',
    'dim': '#cafe09'
}

# Patch to remove preamble and hardcoded sizes. Matplotlib 2.2 has a http URL
# while matplotlib 3 has a https URL, check for both. Matplotlib 3.3 has a new
# <metadata> field (which we're not interested in) and slightly different
# formatting of the global style after (which we unify to the compact version).
_patch_src = re.compile(r"""<\?xml version="1\.0" encoding="utf-8" standalone="no"\?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1\.1//EN"
  "http://www\.w3\.org/Graphics/SVG/1\.1/DTD/svg11\.dtd">
<!-- Created with matplotlib \(https?://matplotlib.org/\) -->
<svg height="\d+(\.\d+)?pt" version="1.1" (?P<viewBox>viewBox="0 0 \d+ \d+(\.\d+)?") width="\d+(\.\d+)?pt" xmlns="http://www\.w3\.org/2000/svg" xmlns:xlink="http://www\.w3\.org/1999/xlink">(
 <metadata>.+</metadata>)?
 <defs>
  <style type="text/css">
?\*{stroke-linecap:butt;stroke-linejoin:round;}(
  )?</style>
 </defs>
""", re.DOTALL)
_patch_dst = r"""<svg \g<viewBox>>
 <defs>
  <style type="text/css">*{stroke-linecap:butt;stroke-linejoin:round;}</style>
 </defs>
"""

# Remove needless newlines and trailing space in path data
_path_patch_src = re.compile('(?P<prev>[\\dz]) ?\n(?P<next>[LMz])', re.MULTILINE)
_path_patch_dst = '\\g<prev> \\g<next>'
_path_patch2_src = re.compile(' ?\n"')
_path_patch2_dst = '"'

# Mapping from color codes to CSS classes
_class_mapping = [
    # Graph background
    ('style="fill:#cafe01;"', 'class="m-background"'),

    # Tick <path> definition in <defs>
    ('style="stroke:#cafe02;stroke-width:0.8;"', 'class="m-line"'),
    # <use>, everything is defined in <defs>, no need to repeat
    ('<use style="fill:#cafe02;stroke:#cafe02;stroke-width:0.8;"', '<use'),

    # Text styles have `font-stretch:normal;` added in matplotlib 3.3, so
    # all of them are duplicated to handle this

    # Label text on left
    ('style="fill:#cafe02;font-family:{font};font-size:11px;font-style:normal;font-weight:normal;"', 'class="m-label"'),
    ('style="fill:#cafe02;font-family:{font};font-size:11px;font-stretch:normal;font-style:normal;font-weight:normal;"', 'class="m-label"'),
    # Label text on bottom (has extra style params)
    ('style="fill:#cafe02;font-family:{font};font-size:11px;font-style:normal;font-weight:normal;', 'class="m-label" style="'),
    ('style="fill:#cafe02;font-family:{font};font-size:11px;font-stretch:normal;font-style:normal;font-weight:normal;', 'class="m-label" style="'),
    # Secondary label text
    ('style="fill:#cafe0b;font-family:{font};font-size:11px;font-style:normal;font-weight:normal;"', 'class="m-label m-dim"'),
    ('style="fill:#cafe0b;font-family:{font};font-size:11px;font-stretch:normal;font-style:normal;font-weight:normal;"', 'class="m-label m-dim"'),
    # Title text
    ('style="fill:#cafe02;font-family:{font};font-size:13px;font-style:normal;font-weight:normal;', 'class="m-title" style="'),
    ('style="fill:#cafe02;font-family:{font};font-size:13px;font-stretch:normal;font-style:normal;font-weight:normal;', 'class="m-title" style="'),

    # Bar colors. Keep in sync with latex2svgextra.
    ('style="fill:#cafe03;"', 'class="m-bar m-default"'),
    ('style="fill:#cafe04;"', 'class="m-bar m-primary"'),
    ('style="fill:#cafe05;"', 'class="m-bar m-success"'),
    ('style="fill:#cafe06;"', 'class="m-bar m-warning"'),
    ('style="fill:#cafe07;"', 'class="m-bar m-danger"'),
    ('style="fill:#cafe08;"', 'class="m-bar m-info"'),
    ('style="fill:#cafe09;"', 'class="m-bar m-dim"'),

    # Error bar line
    ('style="fill:none;stroke:#cafe0a;stroke-width:1.5;"', 'class="m-error"'),
    # Error bar <path> definition in <defs>
    ('style="stroke:#cafe0a;"', 'class="m-error"'),
    # <use>, everything is defined in <defs>, no need to repeat
    ('<use style="fill:#cafe0a;stroke:#cafe0a;"', '<use'),
]

# Titles for bars
_bar_titles_src = '<g id="plot{}-value{}-{}">'
_bar_titles_dst = '<g id="plot{}-value{}-{}"><title>{} {}</title>'
_bar_titles_dst_error = '<g id="plot{}-value{}-{}"><title>{} ± {} {}</title>'

class Plot(rst.Directive):
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = True
    option_spec = {'class': directives.class_option,
                   'name': directives.unchanged,
                   'type': directives.unchanged_required,
                   'labels': directives.unchanged_required,
                   'labels-extra': directives.unchanged,
                   'units': directives.unchanged_required,
                   'values': directives.unchanged_required,
                   'errors': directives.unchanged,
                   'colors': directives.unchanged,
                   'plot-width': directives.unchanged,
                   'bar-height': directives.unchanged,
                   # Legacy options with ugly underscores instead of dashes
                   'labels_extra': directives.unchanged,
                   'bar_height': directives.unchanged}
    has_content = False

    def run(self):
        set_classes(self.options)

        # Type
        assert self.options['type'] == 'barh'

        # Graph title and axis labels. Value labels are one per line.
        title = self.arguments[0]
        units = self.options['units']
        labels = self.options['labels'].split('\n')

        # Legacy options, convert underscores to dashes
        if 'labels_extra' in self.options:
            self.options['labels-extra'] = self.options['labels_extra']
            del self.options['labels_extra']
        if 'bar_height' in self.options:
            self.options['bar-height'] = self.options['bar_height']
            del self.options['bar_height']

        # Optional extra labels
        if 'labels-extra' in self.options:
            labels_extra = self.options['labels-extra'].split('\n')
            assert len(labels_extra) == len(labels)
        else:
            labels_extra = None

        # Values. Should be one for each label, if there are multiple lines
        # then the values get stacked.
        value_sets = []
        for row in self.options['values'].split('\n'):
            values = [float(v) for v in row.split()]
            assert len(values) == len(labels)
            value_sets += [values]

        # Optional errors
        if 'errors' in self.options:
            error_sets = []
            for row in self.options['errors'].split('\n'):
                errors = [float(e) for e in row.split()]
                assert len(errors) == len(values)
                error_sets += [errors]
            assert len(error_sets) == len(value_sets)
        else:
            error_sets = [None]*len(value_sets)

        # Colors. Should be either one for all or one for every value
        if 'colors' in self.options:
            color_sets = []
            for row in self.options['colors'].split('\n'):
                colors = [style_mapping[c] for c in row.split()]
                if len(colors) == 1: colors = colors[0]
                else: assert len(colors) == len(labels)
                color_sets += [colors]
            assert len(color_sets) == len(value_sets)
        else:
            color_sets = [style_mapping['default']]*len(value_sets)

        # Bar height
        bar_height = float(self.options.get('bar-height', '0.4'))

        # Increase hashsalt for every plot to ensure (hopefully) unique SVG IDs
        mpl.rcParams['svg.hashsalt'] = int(mpl.rcParams['svg.hashsalt']) + 1

        # Setup the graph
        fig, ax = plt.subplots()
        # TODO: let matplotlib calculate the height somehow
        fig.set_size_inches(float(self.options.get('plot-width', 8)), 0.78 + len(labels)*bar_height)
        yticks = np.arange(len(labels))
        left = np.array([0.0]*len(labels))
        for i in range(len(value_sets)):
            plot = ax.barh(yticks, value_sets[i], xerr=error_sets[i],
                           align='center', color=color_sets[i], ecolor='#cafe0a', capsize=5*bar_height/0.4, left=left)
            left += np.array(value_sets[i])
            for j, v in enumerate(plot):
                v.set_gid('plot{}-value{}-{}'.format(mpl.rcParams['svg.hashsalt'], i, j))
        ax.set_yticks(yticks)
        ax.invert_yaxis() # top-to-bottom
        ax.set_xlabel(units)
        ax.set_title(title)

        # Value labels. If extra label is specified, create two multiline texts
        # with first having the second line empty and second having the first
        # line empty.
        if labels_extra:
            ax.set_yticklabels([y + ('' if labels_extra[i] == '..' else '\n') for i, y in enumerate(labels)])
            for i, label in enumerate(ax.get_yticklabels()):
                if labels_extra[i] == '..': continue
                ax.text(0, i + 0.05, '\n' + labels_extra[i],
                        va='center', ha='right',
                        transform=label.get_transform(), color='#cafe0b')
        else: ax.set_yticklabels(labels)

        # Export to SVG
        fig.patch.set_visible(False) # hide the white background
        imgdata = io.StringIO()
        fig.savefig(imgdata, format='svg')
        plt.close() # otherwise it consumes a lot of memory in autoreload mode

        # Patch the rendered output: remove preable and hardcoded size
        imgdata = _patch_src.sub(_patch_dst, imgdata.getvalue())
        # Remove needless newlines and trailing whitespace in path data
        imgdata = _path_patch2_src.sub(_path_patch2_dst, _path_patch_src.sub(_path_patch_dst, imgdata))
        # Replace color codes with CSS classes
        for src, dst in _class_mapping: imgdata = imgdata.replace(src, dst)
        # Add titles for bars
        for i in range(len(value_sets)):
            for j in range(len(labels)):
                id = i*len(labels) + j
                if error_sets[i]: imgdata = imgdata.replace(
                    _bar_titles_src.format(mpl.rcParams['svg.hashsalt'], i, j),
                    _bar_titles_dst_error.format(mpl.rcParams['svg.hashsalt'], i, j, value_sets[i][j], error_sets[i][j], units))
                else: imgdata = imgdata.replace(
                    _bar_titles_src.format(mpl.rcParams['svg.hashsalt'], i, j),
                    _bar_titles_dst.format(mpl.rcParams['svg.hashsalt'], i, j, value_sets[i][j], units))

        container = nodes.container(**self.options)
        container['classes'] += ['m-plot']
        node = nodes.raw('', imgdata, format='html')
        container.append(node)
        return [container]

def new_page(*args, **kwargs):
    mpl.rcParams['svg.hashsalt'] = 0

def register_mcss(mcss_settings, hooks_pre_page, **kwargs):
    font = mcss_settings.get('M_PLOTS_FONT', 'Source Sans Pro')
    for i in range(len(_class_mapping)):
        src, dst = _class_mapping[i]
        _class_mapping[i] = (src.format(font=font), dst)
    mpl.rcParams['font.family'] = font

    hooks_pre_page += [new_page]

    rst.directives.register_directive('plot', Plot)

# Below is only Pelican-specific functionality. If Pelican is not found, these
# do nothing.

def _pelican_configure(pelicanobj):
    register_mcss(mcss_settings=pelicanobj.settings, hooks_pre_page=[])

def register(): # for Pelican
    from pelican import signals

    signals.initialized.connect(_pelican_configure)
    signals.content_object_init.connect(new_page)
