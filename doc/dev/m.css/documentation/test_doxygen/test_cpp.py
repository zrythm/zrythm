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

import unittest

from distutils.version import LooseVersion

from . import IntegrationTestCase, doxygen_version

class EnumClass(IntegrationTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'enum_class', *args, **kwargs)

    @unittest.skipUnless(LooseVersion(doxygen_version()) > LooseVersion("1.8.13"),
                         "https://github.com/doxygen/doxygen/pull/627")
    def test(self):
        self.run_doxygen(wildcard='File_8h.xml')
        self.assertEqual(*self.actual_expected_contents('File_8h.html'))

class TemplateAlias(IntegrationTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'template_alias', *args, **kwargs)

    @unittest.skipUnless(LooseVersion(doxygen_version()) > LooseVersion("1.8.13"),
                         "https://github.com/doxygen/doxygen/pull/626")
    def test(self):
        self.run_doxygen(wildcard='*.xml')
        self.assertEqual(*self.actual_expected_contents('File_8h.html'))
        self.assertEqual(*self.actual_expected_contents('structTemplate.html'))

class Derived(IntegrationTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'derived', *args, **kwargs)

    def test(self):
        self.run_doxygen(wildcard='*.xml')
        self.assertEqual(*self.actual_expected_contents('classNamespace_1_1A.html'))
        self.assertEqual(*self.actual_expected_contents('classNamespace_1_1PrivateBase.html'))
        self.assertEqual(*self.actual_expected_contents('classAnother_1_1ProtectedBase.html'))
        self.assertEqual(*self.actual_expected_contents('classNamespace_1_1VirtualBase.html'))
        self.assertEqual(*self.actual_expected_contents('classBaseOutsideANamespace.html'))
        self.assertEqual(*self.actual_expected_contents('classDerivedOutsideANamespace.html'))
        self.assertEqual(*self.actual_expected_contents('structAnother_1_1Final.html'))
        # For the final label in the tree
        self.assertEqual(*self.actual_expected_contents('annotated.html'))

class Friends(IntegrationTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'friends', *args, **kwargs)

    @unittest.skipUnless(LooseVersion(doxygen_version()) > LooseVersion("1.8.13"),
                         "1.8.13 produces invalid XML for friend declarations")
    def test(self):
        self.run_doxygen(wildcard='class*.xml')
        self.assertEqual(*self.actual_expected_contents('classClass.html'))
        self.assertEqual(*self.actual_expected_contents('classTemplate.html'))

class SignalsSlots(IntegrationTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'signals_slots', *args, **kwargs)

    def test(self):
        self.run_doxygen(wildcard='classClass.xml')
        self.assertEqual(*self.actual_expected_contents('classClass.html'))

class VariableTemplate(IntegrationTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'variable_template', *args, **kwargs)

    def test(self):
        self.run_doxygen(wildcard='*.xml')
        self.assertEqual(*self.actual_expected_contents('structFoo.html'))
        self.assertEqual(*self.actual_expected_contents('structBar.html'))

class FunctionAttributes(IntegrationTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'function_attributes', *args, **kwargs)

    def test(self):
        self.run_doxygen(wildcard='*.xml')
        self.assertEqual(*self.actual_expected_contents('structFoo.html'))
        self.assertEqual(*self.actual_expected_contents('classBase.html'))
        self.assertEqual(*self.actual_expected_contents('classDerived.html'))
        self.assertEqual(*self.actual_expected_contents('structFinal.html'))

class FunctionAttributesNospace(IntegrationTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'function_attributes_nospace', *args, **kwargs)

    def test(self):
        self.run_doxygen(wildcard='structFoo.xml')
        self.assertEqual(*self.actual_expected_contents('structFoo.html'))
