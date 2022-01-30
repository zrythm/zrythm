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
import os
from docutils.parsers import rst
from docutils.parsers.rst import Directive
from docutils.parsers.rst import directives, states
from docutils.nodes import fully_normalize_name, whitespace_normalize_name
from docutils.parsers.rst.roles import set_classes
from docutils import nodes

# If Pillow is not available, it's not an error unless one uses the image grid
# functionality (or :scale: option for Image)
try:
    import PIL.Image
    import PIL.ExifTags
except ImportError:
    PIL = None

default_settings = {
    'INPUT': None,
    'M_IMAGES_REQUIRE_ALT_TEXT': False
}

settings = None

class Image(Directive):
    """Image directive

    Copy of docutils.parsers.rst.directives.Image with:

    -   the align option removed (handled better by m.css)
    -   moved the implementation of scale here instead of it being handled in
        the HTML writer
    -   .m-image CSS class added
    -   adding a outer container for clickable image to make the clickable area
        cover only the image
    -   optionally dying when alt text is not present
    """

    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = True
    option_spec = {'alt': directives.unchanged_required,
                   'name': directives.unchanged,
                   'height': directives.length_or_unitless,
                   'width': directives.length_or_percentage_or_unitless,
                   'scale': directives.percentage,
                   'class': directives.class_option,
                   'target': directives.unchanged_required}

    # Overriden by Figure
    image_class = 'm-image'

    def run(self):
        messages = []
        reference = directives.uri(self.arguments[0])
        self.options['uri'] = reference
        reference_node = None
        if 'target' in self.options:
            block = states.escape2null(
                self.options['target']).splitlines()
            block = [line for line in block]
            target_type, data = self.state.parse_target(
                block, self.block_text, self.lineno)
            if target_type == 'refuri':
                reference_node = nodes.reference(refuri=data)
            elif target_type == 'refname':
                reference_node = nodes.reference(
                    refname=fully_normalize_name(data),
                    name=whitespace_normalize_name(data))
                reference_node.indirect_reference_name = data
                self.state.document.note_refname(reference_node)
            else:                           # malformed target
                messages.append(data)       # data is a system message
            del self.options['target']

        width = None
        height = None
        # If scaling requested, open the files and calculate the scaled size
        # Support both {filename} (3.7.1) and {static} (3.8) placeholders,
        # also prepend the absolute path in case we're not Pelican. In all
        # cases use only width and not both so the max-width can correctly
        # scale the image down on smaller screen sizes.
        # TODO: implement ratio-preserving scaling to avoid jumps on load using
        # the margin-bottom hack
        if 'scale' in self.options:
            file = os.path.join(os.getcwd(), settings['INPUT'])
            absuri = os.path.join(file, reference.format(filename=file, static=file))
            im = PIL.Image.open(absuri)
            width = "{}px".format(int(im.width*self.options['scale']/100.0))
        elif 'width' in self.options:
            width = self.options['width']
        elif 'height' in self.options:
            height = self.options['height'] # TODO: convert to width instead?

        # Remove the classes from the image element, will be added either to it
        # or to the wrapping element later
        set_classes(self.options)
        classes = self.options.get('classes', [])
        if 'classes' in self.options: del self.options['classes']
        if 'width' in self.options: del self.options['width']
        if 'height' in self.options: del self.options['height']
        image_node = nodes.image(self.block_text, width=width, height=height, **self.options)

        if not 'alt' in self.options and settings['M_IMAGES_REQUIRE_ALT_TEXT']:
            error = self.state_machine.reporter.error(
                    'Images and figures require the alt text. See the M_IMAGES_REQUIRE_ALT_TEXT option.',
                    image_node,
                    line=self.lineno)
            return [error]

        self.add_name(image_node)
        if reference_node:
            if self.image_class:
                container_node = nodes.container()
                container_node['classes'] += [self.image_class] + classes
                reference_node += image_node
                container_node += reference_node
                return messages + [container_node]
            else:
                reference_node += image_node
                return messages + [reference_node]
        else:
            if self.image_class: image_node['classes'] += [self.image_class] + classes
            return messages + [image_node]

class Figure(Image):
    """Figure directive

    Copy of docutils.parsers.rst.directives.Figure with:

    -   the align option removed (handled better by m.css)
    -   the redundant figwidth option removed (use width/scale/height from the
        Image directive instead)
    -   .m-figure CSS class added
    """

    option_spec = Image.option_spec.copy()
    option_spec['figclass'] = directives.class_option
    has_content = True

    image_class = None

    def run(self):
        figclasses = self.options.pop('figclass', None)
        (image_node,) = Image.run(self)
        if isinstance(image_node, nodes.system_message):
            return [image_node]
        figure_node = nodes.figure('', image_node)
        if figclasses:
            figure_node['classes'] += figclasses
        figure_node['classes'] += ['m-figure']

        if self.content:
            node = nodes.Element()          # anonymous container for parsing
            self.state.nested_parse(self.content, self.content_offset, node)
            first_node = node[0]
            if isinstance(first_node, nodes.paragraph):
                caption = nodes.caption(first_node.rawsource, '',
                                        *first_node.children)
                caption.source = first_node.source
                caption.line = first_node.line
                figure_node += caption
            elif not (isinstance(first_node, nodes.comment)
                      and len(first_node) == 0):
                error = self.state_machine.reporter.error(
                      'Figure caption must be a paragraph or empty comment, got %s' % type(first_node),
                      nodes.literal_block(self.block_text, self.block_text),
                      line=self.lineno)
                return [figure_node, error]
            if len(node) > 1:
                figure_node += nodes.legend('', *node[1:])
        return [figure_node]

# Adapter to accommodate breaking change in Pillow 7.2
# https://pillow.readthedocs.io/en/stable/releasenotes/7.2.0.html#moved-to-imagefiledirectory-v2-in-image-exif
def _to_numerator_denominator_tuple(ratio):
    if isinstance(ratio, tuple):
        return ratio
    else:
        return ratio.numerator, ratio.denominator

class ImageGrid(rst.Directive):
    has_content = True

    def run(self):
        grid_node = nodes.container()
        grid_node['classes'] += ['m-imagegrid', 'm-container-inflate']

        rows = [[]]
        total_widths = [0]
        for uri_caption in self.content:
            # New line, calculating width from 0 again
            if not uri_caption:
                rows.append([])
                total_widths.append(0)
                continue

            uri, _, caption = uri_caption.partition(' ')

            # Open the files and calculate the overall width
            # Support both {filename} (3.7.1) and {static} (3.8) placeholders,
            # also prepend the absolute path in case we're not Pelican
            file = os.path.join(os.getcwd(), settings['INPUT'])
            absuri = os.path.join(file, uri.format(filename=file, static=file))
            im = PIL.Image.open(absuri)

            # If no caption provided, get EXIF info, if it's there
            if not caption and hasattr(im, '_getexif') and im._getexif() is not None:
                exif = {
                    PIL.ExifTags.TAGS[k]: v
                    for k, v in im._getexif().items()
                    if k in PIL.ExifTags.TAGS and len(str(v)) < 256
                }

                # Not all info might be present
                caption = []
                if 'FNumber' in exif:
                    numerator, denominator = _to_numerator_denominator_tuple(exif['FNumber'])
                    caption += ["F{}".format(float(numerator)/float(denominator))]
                if 'ExposureTime' in exif:
                    numerator, denominator = _to_numerator_denominator_tuple(exif['ExposureTime'])
                    if int(numerator) > int(denominator):
                        caption += ["{} s".format(float(numerator)/float(denominator))]
                    else:
                        caption += ["{}/{} s".format(numerator, denominator)]
                if 'ISOSpeedRatings' in exif:
                    caption += ["ISO {}".format(exif['ISOSpeedRatings'])]
                caption = ', '.join(caption)

            # If the caption is `..`, it's meant to be explicitly disabled
            if caption == '..': caption = ''

            rel_width = float(im.width)/im.height
            total_widths[-1] += rel_width
            rows[-1].append((uri, rel_width, caption))

        for i, row in enumerate(rows):
            row_node = nodes.container()

            for uri, rel_width, caption in row:
                image_reference = rst.directives.uri(uri)
                image_node = nodes.image('', uri=image_reference)

                # <figurecaption> in case there's a caption
                if caption:
                    text_nodes, _ = self.state.inline_text(caption, self.lineno)
                    text_node = nodes.paragraph('', '', *text_nodes)
                    overlay_node = nodes.caption()
                    overlay_node.append(text_node)

                # Otherwise an empty <div>
                else: overlay_node = nodes.container()

                link_node = nodes.reference('', refuri=image_reference)
                link_node.append(image_node)
                link_node.append(overlay_node)
                wrapper_node = nodes.figure(width="{:.3f}%".format(rel_width*100.0/total_widths[i]))
                wrapper_node.append(link_node)
                row_node.append(wrapper_node)

            grid_node.append(row_node)

        return [grid_node]

def register_mcss(mcss_settings, **kwargs):
    global default_settings, settings
    settings = copy.deepcopy(default_settings)
    for key in settings.keys():
        if key in mcss_settings: settings[key] = mcss_settings[key]

    rst.directives.register_directive('image', Image)
    rst.directives.register_directive('figure', Figure)
    rst.directives.register_directive('image-grid', ImageGrid)

# Below is only Pelican-specific functionality. If Pelican is not found, these
# do nothing.

def _pelican_configure(pelicanobj):
    settings = {
        'INPUT': pelicanobj.settings['PATH'],
    }
    for key in 'M_IMAGES_REQUIRE_ALT_TEXT':
        if key in pelicanobj.settings: settings[key] = pelicanobj.settings[key]

    register_mcss(mcss_settings=settings)

def register(): # for Pelican
    from pelican import signals

    signals.initialized.connect(_pelican_configure)
