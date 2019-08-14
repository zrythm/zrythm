#include <pybind11/pybind11.h>

PYBIND11_MODULE(pybind_submodules, m) {
    m.doc() = "pybind11 submodules";

    m.def_submodule("sub", "Yay a submodule");
    m.def_submodule("another", "Yay another");
}
