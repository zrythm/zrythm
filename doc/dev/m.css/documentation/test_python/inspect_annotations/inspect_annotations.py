from typing import List, Tuple

class Foo:
    """A class with properties"""

    @property
    def a_property(self) -> List[bool]:
        """A property with a type annotation"""
        pass

def annotation(param: List[int], another: bool, third: str = "hello") -> Foo:
    """An annotated function"""
    pass

def no_annotation(a, b, z):
    """Non-annotated function"""
    pass

def no_annotation_default_param(param, another, third = "hello"):
    """Non-annotated function"""
    pass

def partial_annotation(foo, param: Tuple[int, int], unannotated, cls: Foo):
    """Partially annotated function"""
    pass

# Only possible with native code now, https://www.python.org/dev/peps/pep-0570/
#def positionals_only(positional_only, /, positional_kw):
    #"""Function with explicitly delimited positional args"""
    #pass

def args_kwargs(a, b, *args, **kwargs):
    """Function with args and kwargs"""
    pass

def positional_keyword(positional_kw, *, kw_only):
    """Function with explicitly delimited keyword args"""
    pass

def annotated_positional_keyword(bar = False, *, foo: str, **kwargs):
    """Function with explicitly delimited keyword args and type annotations"""
    pass

UNANNOTATED_VAR = 3.45

ANNOTATED_VAR: Tuple[bool, str] = (False, 'No.')
