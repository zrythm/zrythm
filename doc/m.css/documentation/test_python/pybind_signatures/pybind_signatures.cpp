#include <pybind11/pybind11.h>
#include <pybind11/stl.h> /* needed for std::vector! */

namespace py = pybind11;

int scale(int a, float argument) {
    return int(a*argument);
}

void voidFunction(int) {}

std::tuple<int, int, int> takingAListReturningATuple(const std::vector<float>&) {
    return {};
}

template<std::size_t, class> struct Crazy {};

void crazySignature(const Crazy<3, int>&) {}

std::string overloaded(int) { return {}; }
bool overloaded(float) { return {}; }

struct MyClass {
    static MyClass staticFunction(int, float) { return {}; }

    std::pair<float, int> instanceFunction(int, const std::string&) { return {0.5f, 42}; }

    int another() { return 42; }

    float foo() const { return _foo; }
    void setFoo(float foo) { _foo = foo; }

    private: float _foo = 0.0f;
};

PYBIND11_MODULE(pybind_signatures, m) {
    m.doc() = "pybind11 function signature extraction";

    m
        .def("scale", &scale, "Scale an integer")
        .def("scale_kwargs", &scale, "Scale an integer, kwargs", py::arg("a"), py::arg("argument"))
        .def("void_function", &voidFunction, "Returns nothing")
        .def("taking_a_list_returning_a_tuple", &takingAListReturningATuple, "Takes a list, returns a tuple")
        .def("crazy_signature", &crazySignature, "Function that failed to get parsed")
        .def("overloaded", static_cast<std::string(*)(int)>(&overloaded), "Overloaded for ints")
        .def("overloaded", static_cast<bool(*)(float)>(&overloaded), "Overloaded for floats");

    py::class_<MyClass>(m, "MyClass", "My fun class!")
        .def_static("static_function", &MyClass::staticFunction, "Static method with positional-only args")
        .def(py::init(), "Constructor")
        .def("instance_function", &MyClass::instanceFunction, "Instance method with positional-only args")
        .def("instance_function_kwargs", &MyClass::instanceFunction, "Instance method with position or keyword args", py::arg("hey"), py::arg("what") = "eh?")
        .def("another", &MyClass::another, "Instance method with no args, 'self' is thus position-only")
        .def_property("foo", &MyClass::foo, &MyClass::setFoo, "A read/write property");
}
