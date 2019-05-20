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

import sys
import unittest

from python import parse_pybind_signature

from . import BaseTestCase

class Signature(unittest.TestCase):
    def test(self):
        self.assertEqual(parse_pybind_signature(
            'foo(a: int, a2: module.Thing) -> module.Thing3'),
            ('foo', '', [
                ('a', 'int', None),
                ('a2', 'module.Thing', None),
            ], 'module.Thing3'))

    def test_newline(self):
        self.assertEqual(parse_pybind_signature(
            'foo(a: int, a2: module.Thing) -> module.Thing3\n'),
            ('foo', '', [
                ('a', 'int', None),
                ('a2', 'module.Thing', None),
            ], 'module.Thing3'))

    def test_docs(self):
        self.assertEqual(parse_pybind_signature(
            'foo(a: int, a2: module.Thing) -> module.Thing3\n\nDocs here!!'),
            ('foo', 'Docs here!!', [
                ('a', 'int', None),
                ('a2', 'module.Thing', None),
            ], 'module.Thing3'))

    def test_no_args(self):
        self.assertEqual(parse_pybind_signature(
            'thingy() -> str'),
            ('thingy', '', [], 'str'))

    def test_no_return(self):
        self.assertEqual(parse_pybind_signature(
            '__init__(self: module.Thing)'),
            ('__init__', '', [
                ('self', 'module.Thing', None),
            ], None))

    def test_no_arg_types(self):
        self.assertEqual(parse_pybind_signature(
            'thingy(self, the_other_thing)'),
            ('thingy', '', [
                ('self', None, None),
                ('the_other_thing', None, None),
            ], None))

    def test_square_brackets(self):
        self.assertEqual(parse_pybind_signature(
            'foo(a: Tuple[int, str], no_really: str) -> List[str]'),
            ('foo', '', [
                ('a', 'Tuple[int, str]', None),
                ('no_really', 'str', None),
            ], 'List[str]'))

    def test_nested_square_brackets(self):
        self.assertEqual(parse_pybind_signature(
            'foo(a: Tuple[int, List[Tuple[int, int]]], another: float) -> Union[str, Any]'),
            ('foo', '', [
                ('a', 'Tuple[int, List[Tuple[int, int]]]', None),
                ('another', 'float', None),
            ], 'Union[str, Any]'))

    def test_kwargs(self):
        self.assertEqual(parse_pybind_signature(
            'foo(*args, **kwargs)'),
            ('foo', '', [
                ('*args', None, None),
                ('**kwargs', None, None),
            ], None))

    def test_default_values(self):
        self.assertEqual(parse_pybind_signature(
            'foo(a: float=1.0, b: str=\'hello\')'),
            ('foo', '', [
                ('a', 'float', '1.0'),
                ('b', 'str', '\'hello\''),
            ], None))

    def test_crazy_stuff(self):
        self.assertEqual(parse_pybind_signature(
            'foo(a: int, b: Math::Vector<4, UnsignedInt>)'),
            ('foo', '', [('…', None, None)], None))

    def test_crazy_stuff_docs(self):
        self.assertEqual(parse_pybind_signature(
            'foo(a: int, b: Math::Vector<4, UnsignedInt>)\n\nThis is text!!'),
            ('foo', 'This is text!!', [('…', None, None)], None))

    def test_crazy_return(self):
        self.assertEqual(parse_pybind_signature(
            'foo(a: int) -> Math::Vector<4, UnsignedInt>'),
            ('foo', '', [('…', None, None)], None))

    def test_crazy_return_docs(self):
        self.assertEqual(parse_pybind_signature(
            'foo(a: int) -> Math::Vector<4, UnsignedInt>\n\nThis returns!'),
            ('foo', 'This returns!', [('…', None, None)], None))

    def test_no_name(self):
        self.assertEqual(parse_pybind_signature(
            '(arg0: MyClass) -> float'),
            ('', '', [('arg0', 'MyClass', None)], 'float'))

class Signatures(BaseTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'signatures', *args, **kwargs)

        sys.path.append(self.path)
        import pybind_signatures

    def test_positional_args(self):
        import pybind_signatures

        # Verify that the assumptions are correct -- not using py::arg() makes
        # the parameters positional-only, while py::arg() makes them
        # positional-or-keyword
        self.assertEqual(pybind_signatures.scale(14, 0.3), 4)
        with self.assertRaises(TypeError):
            pybind_signatures.scale(arg0=1, arg1=3.0)
        self.assertEqual(pybind_signatures.scale_kwargs(14, 0.3), 4)
        self.assertEqual(pybind_signatures.scale_kwargs(a=14, argument=0.3), 4)

        # Verify the same for classes
        a = pybind_signatures.MyClass()
        self.assertEqual(pybind_signatures.MyClass.instance_function(a, 3, 'bla'), (0.5, 42))
        with self.assertRaises(TypeError):
            pybind_signatures.MyClass.instance_function(self=a, arg0=3, arg1='bla')
        self.assertEqual(pybind_signatures.MyClass.instance_function_kwargs(a, 3, 'bla'), (0.5, 42))
        self.assertEqual(pybind_signatures.MyClass.instance_function_kwargs(self=a, hey=3, what='bla'), (0.5, 42))

        # In particular, the 'self' parameter is positional-only if there are
        # no arguments to use py::arg() for
        self.assertEqual(pybind_signatures.MyClass.another(a), 42)
        with self.assertRaises(TypeError):
            pybind_signatures.MyClass.another(self=a)

    def test(self):
        import pybind_signatures
        self.run_python({
            'INPUT_MODULES': ['pybind_signatures'],
            'PYBIND11_COMPATIBILITY': True
        })
        self.assertEqual(*self.actual_expected_contents('pybind_signatures.html'))
        self.assertEqual(*self.actual_expected_contents('pybind_signatures.MyClass.html'))

class Enums(BaseTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'enums', *args, **kwargs)

    def test(self):
        self.run_python({
            'PYBIND11_COMPATIBILITY': True
        })
        self.assertEqual(*self.actual_expected_contents('pybind_enums.html'))

class Submodules(BaseTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'submodules', *args, **kwargs)

    def test(self):
        self.run_python({
            'PYBIND11_COMPATIBILITY': True
        })
        self.assertEqual(*self.actual_expected_contents('pybind_submodules.html'))
