"""Another module"""

# This one is a module that shouldn't be exposed
import enum

# This one is a package that shouldn't be exposed
import xml

# These are variables from an external modules, shouldn't be exposed either
from re import I

# TODO: there's no way to tell where a variable of a builtin type comes from,
#   so this would be exposed. The only solution is requiring a presence of an
#   external docstring I'm afraid.
#from math import pi
