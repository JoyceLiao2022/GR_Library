#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(gr_python, m) {
    m.doc() = "Python bindings for the GR library";
}