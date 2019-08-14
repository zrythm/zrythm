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

import math
import os
import sys
import unittest

from distutils.version import LooseVersion

from . import BaseTestCase

class String(BaseTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'string', *args, **kwargs)

    def test(self):
        self.run_python({
            'LINKS_NAVBAR1': [
                ('Modules', 'modules', []),
                ('Classes', 'classes', [])],
        })
        self.assertEqual(*self.actual_expected_contents('inspect_string.html'))
        self.assertEqual(*self.actual_expected_contents('inspect_string.another_module.html'))
        self.assertEqual(*self.actual_expected_contents('inspect_string.Foo.html'))
        self.assertEqual(*self.actual_expected_contents('inspect_string.Specials.html'))

        self.assertEqual(*self.actual_expected_contents('classes.html'))
        self.assertEqual(*self.actual_expected_contents('modules.html'))

class Object(BaseTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'object', *args, **kwargs)

    def test(self):
        # Reuse the stuff from inspect_string, but this time reference it via
        # an object and not a string
        sys.path.append(os.path.join(os.path.dirname(self.path), 'inspect_string'))
        import inspect_string
        self.run_python({
            'LINKS_NAVBAR1': [
                ('Modules', 'modules', []),
                ('Classes', 'classes', [])],
            'INPUT_MODULES': [inspect_string]
        })

        # The output should be the same as when inspecting a string
        self.assertEqual(*self.actual_expected_contents('inspect_string.html', '../inspect_string/inspect_string.html'))
        self.assertEqual(*self.actual_expected_contents('inspect_string.another_module.html', '../inspect_string/inspect_string.another_module.html'))
        self.assertEqual(*self.actual_expected_contents('inspect_string.Foo.html', '../inspect_string/inspect_string.Foo.html'))
        self.assertEqual(*self.actual_expected_contents('inspect_string.Specials.html', '../inspect_string/inspect_string.Specials.html'))

        self.assertEqual(*self.actual_expected_contents('classes.html', '../inspect_string/classes.html'))
        self.assertEqual(*self.actual_expected_contents('modules.html', '../inspect_string/modules.html'))

class AllProperty(BaseTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'all_property', *args, **kwargs)

    def test(self):
        self.run_python()
        self.assertEqual(*self.actual_expected_contents('inspect_all_property.html'))

class Annotations(BaseTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'annotations', *args, **kwargs)

    def test(self):
        self.run_python()
        self.assertEqual(*self.actual_expected_contents('inspect_annotations.html'))
        self.assertEqual(*self.actual_expected_contents('inspect_annotations.Foo.html'))

    @unittest.skipUnless(LooseVersion(sys.version) >= LooseVersion('3.7'),
        "signature with / for pow() is not present in 3.6")
    def test_math(self):
        # From math export only pow() so we have the verification easier, and
        # in addition log() because it doesn't provide any signature metadata
        assert not hasattr(math, '__all__')
        math.__all__ = ['pow', 'log']

        self.run_python({
            'INPUT_MODULES': [math]
        })

        del math.__all__
        assert not hasattr(math, '__all__')

        self.assertEqual(*self.actual_expected_contents('math.html'))

    @unittest.skipUnless(LooseVersion(sys.version) < LooseVersion('3.7'),
        "docstring for log() is different in 3.7")
    def test_math36(self):
        # From math export only pow() so we have the verification easier, and
        # in addition log() because it doesn't provide any signature metadata
        assert not hasattr(math, '__all__')
        math.__all__ = ['log']

        self.run_python({
            'INPUT_MODULES': [math]
        })

        del math.__all__
        assert not hasattr(math, '__all__')

        self.assertEqual(*self.actual_expected_contents('math.html', 'math36.html'))
