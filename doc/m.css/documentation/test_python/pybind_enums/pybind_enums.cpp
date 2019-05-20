#include <pybind11/pybind11.h>

namespace py = pybind11;

enum MyEnum {
    First = 0,
    Second = 1,
    Third = 74,
    Consistent = -5
};

enum SixtyfourBitFlag: std::uint64_t {
    Yes = 1000000000000ull,
    No = 18446744073709551615ull
};

PYBIND11_MODULE(pybind_enums, m) {
    m.doc() = "pybind11 enum parsing";

    py::enum_<MyEnum>(m, "MyEnum", "An enum without value docs :(")
        .value("First", MyEnum::First)
        .value("Second", MyEnum::Second)
        .value("Third", MyEnum::Third)
        .value("CONSISTANTE", MyEnum::Consistent);

    py::enum_<SixtyfourBitFlag>(m, "SixtyfourBitFlag", "64-bit flags")
        .value("Yes", SixtyfourBitFlag::Yes)
        .value("No", SixtyfourBitFlag::No);
}
