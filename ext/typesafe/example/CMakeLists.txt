# Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

function(_type_safe_example name)
    add_executable(type_safe_example_${name} ${name}.cpp)
    target_link_libraries(type_safe_example_${name} PUBLIC type_safe)
    set_property(TARGET type_safe_example_${name} PROPERTY CXX_STANDARD 11)
endfunction()

_type_safe_example(constrained)
_type_safe_example(optional)
_type_safe_example(output_parameter)
_type_safe_example(strong_typedef)
_type_safe_example(types)
